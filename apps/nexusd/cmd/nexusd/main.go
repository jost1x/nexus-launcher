package main

import (
	"context"
	"errors"
	"flag"
	"fmt"
	"log/slog"
	"net"
	"net/http"
	"os"
	"os/signal"
	"path/filepath"
	"syscall"
	"time"

	"connectrpc.com/connect/v2"
	"connectrpc.com/connect/v2/connecthttp"
	"connectrpc.com/grpchealth/v2"
	nexusv1connect "nexusd/gen/nexus/v1/nexusv1connect"
	"nexusd/internal/platform"
	"nexusd/internal/services"
	"nexusd/internal/socketpath"
)

const socketMode = 0o600

func main() {
	if err := run(); err != nil {
		slog.Error("nexusd stopped", "error", err)
		os.Exit(1)
	}
}

func run() error {
	defaultSocket, err := socketpath.Default()
	if err != nil {
		return err
	}

	flags := flag.NewFlagSet("nexusd", flag.ContinueOnError)
	socket := flags.String("socket", defaultSocket, "Unix domain socket path")
	if err := flags.Parse(os.Args[1:]); err != nil {
		return err
	}
	if flags.NArg() != 0 {
		return fmt.Errorf("unexpected arguments: %v", flags.Args())
	}

	listener, err := listenUnix(*socket)
	if err != nil {
		return err
	}
	defer removeSocket(*socket)

	checker := grpchealth.NewStaticChecker()
	setHealthStatus(checker, grpchealth.StatusServing)
	connectServer := connect.NewServer()
	grpchealth.Register(connectServer, checker)
	adapter := platform.NewLinux()
	nexusv1connect.RegisterSystemServiceHandler(connectServer, services.System{Adapter: adapter})
	nexusv1connect.RegisterWifiServiceHandler(connectServer, &services.Wifi{Adapter: adapter})
	nexusv1connect.RegisterHardwareServiceHandler(connectServer, services.Hardware{Adapter: adapter})

	mux := http.NewServeMux()
	connecthttp.Mount(mux, connectServer)
	protocols := new(http.Protocols)
	protocols.SetUnencryptedHTTP2(true)

	server := &http.Server{
		Handler:           mux,
		Protocols:         protocols,
		ReadHeaderTimeout: 5 * time.Second,
	}

	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer stop()

	serveErr := make(chan error, 1)
	go func() {
		slog.Info("serving gRPC health over Unix domain socket", "socket", *socket)
		serveErr <- server.Serve(listener)
	}()

	select {
	case err := <-serveErr:
		if !errors.Is(err, http.ErrServerClosed) {
			return fmt.Errorf("serve RPC: %w", err)
		}
		return nil
	case <-ctx.Done():
	}

	setHealthStatus(checker, grpchealth.StatusNotServing)
	shutdownCtx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()
	if err := server.Shutdown(shutdownCtx); err != nil {
		_ = server.Close()
		return fmt.Errorf("shut down RPC server: %w", err)
	}

	if err := <-serveErr; !errors.Is(err, http.ErrServerClosed) {
		return fmt.Errorf("serve RPC during shutdown: %w", err)
	}
	slog.Info("shutdown complete")
	return nil
}

func setHealthStatus(checker *grpchealth.StaticChecker, status grpchealth.Status) {
	checker.SetStatus("", status)
	checker.SetStatus(nexusv1connect.SystemServiceName, status)
	checker.SetStatus(nexusv1connect.WifiServiceName, status)
	checker.SetStatus(nexusv1connect.HardwareServiceName, status)
}

func listenUnix(path string) (net.Listener, error) {
	if path == "" {
		return nil, errors.New("socket path cannot be empty")
	}
	if len(path) > 100 {
		return nil, fmt.Errorf("socket path is too long (%d bytes): %s", len(path), path)
	}
	if err := os.MkdirAll(filepath.Dir(path), 0o700); err != nil {
		return nil, fmt.Errorf("create socket directory: %w", err)
	}

	if info, err := os.Lstat(path); err == nil {
		if info.Mode()&os.ModeSocket == 0 {
			return nil, fmt.Errorf("refusing to replace non-socket path %s", path)
		}
		conn, dialErr := net.DialTimeout("unix", path, 200*time.Millisecond)
		if dialErr == nil {
			_ = conn.Close()
			return nil, fmt.Errorf("another nexusd instance is listening on %s", path)
		}
		if err := os.Remove(path); err != nil {
			return nil, fmt.Errorf("remove stale socket %s: %w", path, err)
		}
	} else if !errors.Is(err, os.ErrNotExist) {
		return nil, fmt.Errorf("inspect socket path: %w", err)
	}

	listener, err := net.Listen("unix", path)
	if err != nil {
		return nil, fmt.Errorf("listen on %s: %w", path, err)
	}
	if err := os.Chmod(path, socketMode); err != nil {
		_ = listener.Close()
		_ = os.Remove(path)
		return nil, fmt.Errorf("restrict socket permissions: %w", err)
	}
	return listener, nil
}

func removeSocket(path string) {
	if err := os.Remove(path); err != nil && !errors.Is(err, os.ErrNotExist) {
		slog.Warn("could not remove Unix socket", "path", path, "error", err)
	}
}

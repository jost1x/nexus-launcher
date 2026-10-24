package main

import (
	"context"
	"flag"
	"fmt"
	"net"
	"net/http"
	"os"
	"time"

	"connectrpc.com/connect/v2"
	"connectrpc.com/connect/v2/connecthttp"
	"connectrpc.com/grpchealth/v2"
	"nexusd/internal/socketpath"
)

func main() {
	if err := run(); err != nil {
		fmt.Fprintln(os.Stderr, "nexusctl:", err)
		os.Exit(1)
	}
}

func run() error {
	defaultSocket, err := socketpath.Default()
	if err != nil {
		return err
	}

	flags := flag.NewFlagSet("nexusctl", flag.ContinueOnError)
	flags.SetOutput(os.Stderr)
	socket := flags.String("socket", defaultSocket, "Unix domain socket path")
	timeout := flags.Duration("timeout", 3*time.Second, "RPC timeout")
	if err := flags.Parse(os.Args[1:]); err != nil {
		return err
	}
	if *timeout <= 0 {
		return fmt.Errorf("timeout must be greater than zero")
	}
	if flags.NArg() == 0 {
		return usageError()
	}
	if flags.Arg(0) != "health" || flags.NArg() > 2 {
		return usageError()
	}

	service := ""
	if flags.NArg() == 2 {
		service = flags.Arg(1)
	}

	protocols := new(http.Protocols)
	protocols.SetUnencryptedHTTP2(true)
	transport := &http.Transport{
		Protocols: protocols,
		DialContext: func(ctx context.Context, _, _ string) (net.Conn, error) {
			return (&net.Dialer{}).DialContext(ctx, "unix", *socket)
		},
	}
	defer transport.CloseIdleConnections()

	client := grpchealth.NewClient(connect.NewClient(
		connecthttp.NewTransport(&http.Client{Transport: transport}, "http://nexusd", connecthttp.WithGRPC()),
	))
	ctx, cancel := context.WithTimeout(context.Background(), *timeout)
	defer cancel()
	response, err := client.Check(ctx, &grpchealth.CheckRequest{Service: service})
	if err != nil {
		return fmt.Errorf("health check failed: %w", err)
	}
	fmt.Println(response.Status)
	if response.Status != grpchealth.StatusServing {
		return fmt.Errorf("service is %s", response.Status)
	}
	return nil
}

func usageError() error {
	return fmt.Errorf("usage: nexusctl [--socket PATH] [--timeout DURATION] health [SERVICE]\n       health with no SERVICE checks the whole daemon")
}

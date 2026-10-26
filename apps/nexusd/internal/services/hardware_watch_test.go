package services

import (
	"context"
	"net/http"
	"net/http/httptest"
	"os"
	"os/exec"
	"path/filepath"
	"testing"
	"time"

	"connectrpc.com/connect/v2"
	"connectrpc.com/connect/v2/connecthttp"
	nexusv1 "nexusd/gen/nexus/v1"
	nexusv1connect "nexusd/gen/nexus/v1/nexusv1connect"
	"nexusd/internal/platform"
)

func TestHardwareStatusHubCoalescesAndUnsubscribes(t *testing.T) {
	hub := NewHardwareStatusHub()
	updates, unsubscribe := hub.subscribe()

	hub.Notify()
	hub.Notify()
	if got := len(updates); got != 1 {
		t.Fatalf("queued updates = %d, want coalesced single notification", got)
	}
	<-updates
	unsubscribe()
	hub.Notify()
	if got := len(updates); got != 0 {
		t.Fatalf("queued updates after unsubscribe = %d, want 0", got)
	}
}

func TestHardwareStatusStreamSendsInitialAndChangedStateOverGRPC(t *testing.T) {
	sysRoot := t.TempDir()
	backlight := filepath.Join(sysRoot, "class", "backlight", "test")
	if err := os.MkdirAll(backlight, 0o755); err != nil {
		t.Fatal(err)
	}
	brightnessPath := filepath.Join(backlight, "brightness")
	if err := os.WriteFile(brightnessPath, []byte("20\n"), 0o644); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Join(backlight, "max_brightness"), []byte("100\n"), 0o644); err != nil {
		t.Fatal(err)
	}
	adapter := &platform.Linux{
		SysRoot: sysRoot,
		LookPath: func(string) (string, error) {
			return "", exec.ErrNotFound
		},
	}
	hub := NewHardwareStatusHub()

	mux := http.NewServeMux()
	rpcServer := connect.NewServer()
	nexusv1connect.RegisterHardwareServiceHandler(rpcServer, Hardware{Adapter: adapter, Watcher: hub})
	connecthttp.Mount(mux, rpcServer)
	server := httptest.NewUnstartedServer(mux)
	server.EnableHTTP2 = true
	server.StartTLS()
	defer server.Close()

	transport := connecthttp.NewTransport(server.Client(), server.URL, connecthttp.WithGRPC())
	client := nexusv1connect.NewHardwareServiceClient(connect.NewClient(transport))
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	stream, err := client.WatchStatus(ctx, &nexusv1.HardwareServiceGetStatusRequest{})
	if err != nil {
		t.Fatal(err)
	}
	defer stream.Close()

	initial, err := stream.Receive()
	if err != nil {
		t.Fatal(err)
	}
	if got := initial.GetBrightness(); got != 20 {
		t.Fatalf("initial brightness = %d, want 20", got)
	}
	monitorCtx, stopMonitor := context.WithCancel(context.Background())
	defer stopMonitor()
	go hub.MonitorHardwareStatus(monitorCtx, adapter, 10*time.Millisecond)
	time.Sleep(20 * time.Millisecond)
	if err := os.WriteFile(brightnessPath, []byte("35\n"), 0o644); err != nil {
		t.Fatal(err)
	}

	changed := make(chan int64, 1)
	go func() {
		response, err := stream.Receive()
		if err != nil {
			changed <- -1
			return
		}
		changed <- response.GetBrightness()
	}()
	select {
	case got := <-changed:
		if got != 35 {
			t.Fatalf("stream brightness = %d, want 35", got)
		}
	case <-time.After(2 * time.Second):
		t.Fatal("timed out waiting for hardware state update")
	}
}

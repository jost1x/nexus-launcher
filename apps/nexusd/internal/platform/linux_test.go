package platform

import (
	"context"
	"os"
	"path/filepath"
	"strings"
	"testing"

	"connectrpc.com/connect/v2"
)

func TestParseScan(t *testing.T) {
	response := parseScan([]byte("BSS 00:11:22:33:44:55(on wlan0)\n\tfreq: 2412\n\tsignal: -47.00 dBm\n\tSSID: home\n\tRSN: *\nBSS aa:bb:cc:dd:ee:ff(on wlan0)\n\tSSID: \n"))
	if len(response.Networks) != 1 {
		t.Fatalf("got %d networks, want 1", len(response.Networks))
	}
	if response.Networks[0].Ssid != "home" || response.Networks[0].Security != "secured" || response.Networks[0].SignalPercent != 100 {
		t.Fatalf("unexpected parsed network: %+v", response.Networks[0])
	}
}

func TestWifiStatusUsesRoutableNetworkdState(t *testing.T) {
	sys := t.TempDir()
	iface := filepath.Join(sys, "class/net/wlan0")
	if err := os.MkdirAll(iface, 0o755); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Join(iface, "operstate"), []byte("up\n"), 0o644); err != nil {
		t.Fatal(err)
	}
	adapter := &Linux{SysRoot: sys, LookPath: func(name string) (string, error) {
		if name == "networkctl" {
			return "/usr/bin/networkctl", nil
		}
		return "", os.ErrNotExist
	}, Run: func(_ context.Context, _ string, _ ...string) ([]byte, error) {
		return []byte("State: routable (configured)\n"), nil
	}}
	if err := os.MkdirAll(filepath.Join(iface, "wireless"), 0o755); err != nil {
		t.Fatal(err)
	}
	status, err := adapter.WifiStatus(context.Background())
	if err != nil {
		t.Fatal(err)
	}
	if status.Interface != "wlan0" || !status.Connected {
		t.Fatalf("unexpected Wi-Fi status: %+v", status)
	}
}

func TestWifiStatusDoesNotTreatInterfaceUpAsConnected(t *testing.T) {
	sys := t.TempDir()
	iface := filepath.Join(sys, "class/net/wlan0")
	if err := os.MkdirAll(filepath.Join(iface, "wireless"), 0o755); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Join(iface, "operstate"), []byte("up\n"), 0o644); err != nil {
		t.Fatal(err)
	}
	adapter := &Linux{SysRoot: sys, LookPath: func(string) (string, error) { return "", os.ErrNotExist }}
	status, err := adapter.WifiStatus(context.Background())
	if err != nil {
		t.Fatal(err)
	}
	if status.Connected {
		t.Fatalf("interface operstate alone must not confirm association: %+v", status)
	}
}

func TestWifiScanHonorsCancellation(t *testing.T) {
	sys := t.TempDir()
	iface := filepath.Join(sys, "class/net/wlan0")
	if err := os.MkdirAll(filepath.Join(iface, "wireless"), 0o755); err != nil {
		t.Fatal(err)
	}
	adapter := &Linux{
		SysRoot: sys,
		LookPath: func(name string) (string, error) {
			if name == "iw" {
				return "/usr/sbin/iw", nil
			}
			return "", os.ErrNotExist
		},
		Run: func(ctx context.Context, _ string, _ ...string) ([]byte, error) { return nil, ctx.Err() },
	}
	ctx, cancel := context.WithCancel(context.Background())
	cancel()
	_, err := adapter.ScanWifi(ctx, "")
	if rpcErr, ok := err.(*connect.Error); !ok || rpcErr.Code() != connect.CodeCanceled {
		t.Fatalf("unexpected error: %v", err)
	}
}

func TestWifiScanWithoutUtilityIsUnsupported(t *testing.T) {
	sys := t.TempDir()
	iface := filepath.Join(sys, "class/net/wlan0")
	if err := os.MkdirAll(filepath.Join(iface, "wireless"), 0o755); err != nil {
		t.Fatal(err)
	}
	adapter := &Linux{SysRoot: sys, LookPath: func(string) (string, error) { return "", os.ErrNotExist }}
	_, err := adapter.ScanWifi(context.Background(), "")
	if rpcErr, ok := err.(*connect.Error); !ok || rpcErr.Code() != connect.CodeFailedPrecondition {
		t.Fatalf("unexpected error: %v", err)
	}
}

func TestWifiInterfaceRejectsPathInput(t *testing.T) {
	adapter := &Linux{SysRoot: t.TempDir()}
	if _, err := adapter.wifiInterface("wlan0/../../etc"); err == nil {
		t.Fatal("expected invalid interface error")
	}
}

func TestFindCommandInSearchesSystemDirectories(t *testing.T) {
	dir := t.TempDir()
	tool := filepath.Join(dir, "iw")
	if err := os.WriteFile(tool, []byte("binary"), 0o755); err != nil {
		t.Fatal(err)
	}
	path, err := findCommandIn("iw", func(string) (string, error) { return "", os.ErrNotExist }, []string{dir})
	if err != nil {
		t.Fatal(err)
	}
	if path != tool {
		t.Fatalf("got %q, want %q", path, tool)
	}
}

func TestSetBrightnessRequiresConfirmationAndScalesValue(t *testing.T) {
	sys := t.TempDir()
	path := filepath.Join(sys, "class/backlight/panel")
	if err := os.MkdirAll(path, 0o755); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Join(path, "brightness"), []byte("50"), 0o644); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Join(path, "max_brightness"), []byte("200"), 0o644); err != nil {
		t.Fatal(err)
	}
	adapter := &Linux{SysRoot: sys}
	if _, err := adapter.SetBrightness(50, false); err == nil {
		t.Fatal("expected confirmation error")
	} else if rpcErr, ok := err.(*connect.Error); !ok || rpcErr.Code() != connect.CodeFailedPrecondition {
		t.Fatalf("unexpected error: %v", err)
	}
	value, err := adapter.SetBrightness(25, true)
	if err != nil {
		t.Fatal(err)
	}
	if value != 50 {
		t.Fatalf("got value %d, want 50", value)
	}
	data, err := os.ReadFile(filepath.Join(path, "brightness"))
	if err != nil {
		t.Fatal(err)
	}
	if strings.TrimSpace(string(data)) != "50" {
		t.Fatalf("unexpected brightness file: %q", data)
	}
}

func TestSetVolumeRequiresConfirmation(t *testing.T) {
	adapter := &Linux{}
	err := adapter.SetVolume(context.Background(), 50, false)
	if rpcErr, ok := err.(*connect.Error); !ok || rpcErr.Code() != connect.CodeFailedPrecondition {
		t.Fatalf("unexpected error: %v", err)
	}
}

func TestSetVolumeUsesOnlySupportedMixerControl(t *testing.T) {
	var gotArgs []string
	adapter := &Linux{
		LookPath: func(name string) (string, error) {
			if name == "amixer" {
				return "/usr/bin/amixer", nil
			}
			return "", os.ErrNotExist
		},
		Run: func(_ context.Context, _ string, args ...string) ([]byte, error) {
			if len(args) == 1 && args[0] == "scontrols" {
				return []byte("Simple mixer control 'Master',0\n"), nil
			}
			gotArgs = append([]string(nil), args...)
			return nil, nil
		},
	}
	if err := adapter.SetVolume(context.Background(), 42, true); err != nil {
		t.Fatal(err)
	}
	want := []string{"sset", "Master", "42%"}
	if strings.Join(gotArgs, " ") != strings.Join(want, " ") {
		t.Fatalf("got args %q, want %q", gotArgs, want)
	}
}

func TestPowerActionRequiresConfirmation(t *testing.T) {
	adapter := &Linux{}
	err := adapter.PowerAction(context.Background(), 1, false)
	if rpcErr, ok := err.(*connect.Error); !ok || rpcErr.Code() != connect.CodeFailedPrecondition {
		t.Fatalf("unexpected error: %v", err)
	}
}

func TestPowerActionUsesAllowlistedSystemctlAction(t *testing.T) {
	var gotArgs []string
	adapter := &Linux{
		LookPath: func(name string) (string, error) {
			if name == "systemctl" {
				return "/usr/bin/systemctl", nil
			}
			return "", os.ErrNotExist
		},
		Run: func(_ context.Context, _ string, args ...string) ([]byte, error) {
			gotArgs = append([]string(nil), args...)
			return nil, nil
		},
	}
	if err := adapter.PowerAction(context.Background(), 1, true); err != nil {
		t.Fatal(err)
	}
	if strings.Join(gotArgs, " ") != "reboot" {
		t.Fatalf("got args %q, want reboot", gotArgs)
	}
}

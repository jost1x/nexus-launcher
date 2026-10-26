package platform

import (
	"context"
	"crypto/rand"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"net"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"time"

	"connectrpc.com/connect/v2"
	nexusv1 "nexusd/gen/nexus/v1"
)

const wifiProfilesFile = "wifi-profiles.json"

type WifiProfile struct {
	ID        string `json:"id"`
	SSID      string `json:"ssid"`
	Password  string `json:"password,omitempty"`
	Security  string `json:"security"`
	Preferred bool   `json:"preferred"`
	Priority  int    `json:"priority"`
}

type wifiProfileStore struct {
	Version  int           `json:"version"`
	Profiles []WifiProfile `json:"profiles"`
}

func (p *Linux) wifiStateDirectory() (string, error) {
	if p.WifiStateDir != "" {
		return p.WifiStateDir, nil
	}
	cache, err := os.UserCacheDir()
	if err != nil {
		return "", fmt.Errorf("resolve private Wi-Fi profile directory: %w", err)
	}
	return filepath.Join(cache, "nexusd"), nil
}

func (p *Linux) LoadWifiProfiles() ([]WifiProfile, error) {
	directory, err := p.wifiStateDirectory()
	if err != nil {
		return nil, err
	}
	path := filepath.Join(directory, wifiProfilesFile)
	data, err := os.ReadFile(path)
	if errors.Is(err, os.ErrNotExist) {
		return nil, nil
	}
	if err != nil {
		return nil, fmt.Errorf("read saved Wi-Fi networks: %w", err)
	}
	info, err := os.Stat(path)
	if err != nil {
		return nil, fmt.Errorf("inspect saved Wi-Fi networks: %w", err)
	}
	if !info.Mode().IsRegular() || info.Mode().Perm()&0o077 != 0 {
		return nil, errors.New("saved Wi-Fi networks must be a regular file accessible only to the daemon user")
	}
	var store wifiProfileStore
	if err := json.Unmarshal(data, &store); err != nil {
		return nil, fmt.Errorf("decode saved Wi-Fi networks: %w", err)
	}
	if store.Version != 1 {
		return nil, fmt.Errorf("unsupported saved Wi-Fi network format version %d", store.Version)
	}
	return store.Profiles, nil
}

func (p *Linux) SaveWifiProfiles(profiles []WifiProfile) error {
	directory, err := p.wifiStateDirectory()
	if err != nil {
		return err
	}
	if err := os.MkdirAll(directory, 0o700); err != nil {
		return fmt.Errorf("create private Wi-Fi profile directory: %w", err)
	}
	if err := os.Chmod(directory, 0o700); err != nil {
		return fmt.Errorf("restrict Wi-Fi profile directory: %w", err)
	}
	data, err := json.MarshalIndent(wifiProfileStore{Version: 1, Profiles: profiles}, "", "  ")
	if err != nil {
		return fmt.Errorf("encode saved Wi-Fi networks: %w", err)
	}
	file, err := os.CreateTemp(directory, ".wifi-profiles-*")
	if err != nil {
		return fmt.Errorf("create saved Wi-Fi network file: %w", err)
	}
	temporary := file.Name()
	defer os.Remove(temporary)
	if err := file.Chmod(0o600); err != nil {
		_ = file.Close()
		return fmt.Errorf("restrict saved Wi-Fi network file: %w", err)
	}
	if _, err := file.Write(data); err != nil {
		_ = file.Close()
		return fmt.Errorf("write saved Wi-Fi networks: %w", err)
	}
	if err := file.Sync(); err != nil {
		_ = file.Close()
		return fmt.Errorf("sync saved Wi-Fi networks: %w", err)
	}
	if err := file.Close(); err != nil {
		return fmt.Errorf("close saved Wi-Fi networks: %w", err)
	}
	if err := os.Rename(temporary, filepath.Join(directory, wifiProfilesFile)); err != nil {
		return fmt.Errorf("publish saved Wi-Fi networks: %w", err)
	}
	return nil
}

func (p *Linux) rfkillState(ctx context.Context) (bool, bool) {
	if p.LookPath == nil || p.Run == nil {
		return false, false
	}
	rfkill, err := p.LookPath("rfkill")
	if err != nil {
		return false, false
	}
	output, err := p.Run(ctx, rfkill, "list", "wlan")
	if err != nil {
		return false, false
	}
	text := strings.ToLower(string(output))
	if !strings.Contains(text, "wireless lan") && !strings.Contains(text, "wlan") {
		return false, false
	}
	if strings.Contains(text, "soft blocked: yes") || strings.Contains(text, "hard blocked: yes") {
		return false, true
	}
	if strings.Contains(text, "soft blocked: no") && strings.Contains(text, "hard blocked: no") {
		return true, true
	}
	return false, false
}

func (p *Linux) SetWifiEnabled(ctx context.Context, enabled bool) error {
	if p.LookPath == nil || p.Run == nil {
		return connect.NewError(connect.CodeFailedPrecondition, "Wi-Fi radio control is unavailable")
	}
	rfkill, err := p.LookPath("rfkill")
	if err != nil {
		return connect.NewError(connect.CodeFailedPrecondition, "Wi-Fi radio control requires rfkill")
	}
	action := "block"
	if enabled {
		action = "unblock"
	}
	output, err := p.Run(ctx, rfkill, action, "wlan")
	if err != nil {
		return connect.NewError(connect.CodePermissionDenied, fmt.Sprintf("could not change Wi-Fi radio state: %s", strings.TrimSpace(string(output))))
	}
	return nil
}

func (p *Linux) wifiControlPath(iface string) (string, error) {
	for _, directory := range []string{"/run/wpa_supplicant", "/var/run/wpa_supplicant"} {
		path := filepath.Join(directory, iface)
		if _, err := os.Stat(path); err == nil {
			return path, nil
		}
	}
	return "", connect.NewError(connect.CodeFailedPrecondition, "wpa_supplicant control socket is unavailable for this interface")
}

// wpaRequest talks to the per-interface wpa_supplicant control socket directly.
// This keeps passwords out of command-line arguments and process listings.
func (p *Linux) WPARequest(ctx context.Context, iface, command string) (string, error) {
	remotePath, err := p.wifiControlPath(iface)
	if err != nil {
		return "", err
	}
	directory, err := os.MkdirTemp("", "nexus-wpa-")
	if err != nil {
		return "", fmt.Errorf("create temporary wpa_supplicant control socket: %w", err)
	}
	defer os.RemoveAll(directory)
	localPath := filepath.Join(directory, "c")
	local, err := net.ListenUnixgram("unixgram", &net.UnixAddr{Name: localPath, Net: "unixgram"})
	if err != nil {
		return "", fmt.Errorf("open temporary wpa_supplicant control socket: %w", err)
	}
	defer local.Close()
	if err := local.SetDeadline(time.Now().Add(4 * time.Second)); err != nil {
		return "", err
	}
	if deadline, ok := ctx.Deadline(); ok {
		_ = local.SetDeadline(deadline)
	}
	if _, err := local.WriteToUnix([]byte(command), &net.UnixAddr{Name: remotePath, Net: "unixgram"}); err != nil {
		return "", fmt.Errorf("send wpa_supplicant request: %w", err)
	}
	buffer := make([]byte, 4096)
	n, _, err := local.ReadFromUnix(buffer)
	if err != nil {
		if ctx.Err() != nil {
			return "", ctx.Err()
		}
		return "", fmt.Errorf("receive wpa_supplicant response: %w", err)
	}
	response := strings.TrimSpace(string(buffer[:n]))
	if response == "FAIL" || strings.HasPrefix(response, "FAIL-") {
		return response, errors.New("wpa_supplicant rejected the request")
	}
	return response, nil
}

func (p *Linux) listWpaNetworks(ctx context.Context, iface string) (map[string]int, error) {
	response, err := p.WPARequest(ctx, iface, "LIST_NETWORKS")
	if err != nil {
		return nil, err
	}
	networks := make(map[string]int)
	for _, line := range strings.Split(response, "\n")[1:] {
		fields := strings.Split(line, "\t")
		if len(fields) < 1 {
			continue
		}
		id, err := strconv.Atoi(fields[0])
		if err == nil {
			networks[fields[0]] = id
		}
	}
	return networks, nil
}

func (p *Linux) FindWpaProfile(ctx context.Context, iface, profileID string) (int, bool, error) {
	networks, err := p.listWpaNetworks(ctx, iface)
	if err != nil {
		return 0, false, err
	}
	for id, number := range networks {
		response, err := p.WPARequest(ctx, iface, "GET_NETWORK "+id+" id_str")
		if err == nil && strings.Trim(response, "\" ") == "nexus:"+profileID {
			return number, true, nil
		}
	}
	return 0, false, nil
}

func (p *Linux) ApplyWpaProfile(ctx context.Context, iface string, profile WifiProfile) error {
	p.wifiControlMu.Lock()
	defer p.wifiControlMu.Unlock()
	networkID, found, err := p.FindWpaProfile(ctx, iface, profile.ID)
	if err != nil {
		return err
	}
	if !found {
		response, err := p.WPARequest(ctx, iface, "ADD_NETWORK")
		if err != nil {
			return fmt.Errorf("add network to wpa_supplicant: %w", err)
		}
		networkID, err = strconv.Atoi(response)
		if err != nil {
			return fmt.Errorf("invalid wpa_supplicant network identifier %q", response)
		}
	}
	command := func(field, value string) error {
		response, err := p.WPARequest(ctx, iface, fmt.Sprintf("SET_NETWORK %d %s %s", networkID, field, value))
		if err != nil || response != "OK" {
			return fmt.Errorf("wpa_supplicant rejected network setting %s", field)
		}
		return nil
	}
	if err := command("ssid", hex.EncodeToString([]byte(profile.SSID))); err != nil {
		return err
	}
	if err := command("id_str", `"nexus:`+profile.ID+`"`); err != nil {
		return err
	}
	priority := profile.Priority
	if profile.Preferred {
		priority = 1000
	}
	if err := command("priority", strconv.Itoa(priority)); err != nil {
		return err
	}
	if profile.Security == "open" {
		if err := command("key_mgmt", "NONE"); err != nil {
			return err
		}
	} else {
		if err := command("key_mgmt", "WPA-PSK"); err != nil {
			return err
		}
		password := profile.Password
		if !validHexWifiPSK(password) {
			password = `"` + strings.NewReplacer(`\`, `\\`, `"`, `\"`).Replace(password) + `"`
		}
		if err := command("psk", password); err != nil {
			return err
		}
	}
	if response, err := p.WPARequest(ctx, iface, fmt.Sprintf("ENABLE_NETWORK %d", networkID)); err != nil || response != "OK" {
		return errors.New("could not enable saved Wi-Fi network")
	}
	return nil
}

func validHexWifiPSK(value string) bool {
	if len(value) != 64 {
		return false
	}
	_, err := hex.DecodeString(value)
	return err == nil
}

func (p *Linux) connectWifiProfile(ctx context.Context, iface string, profile WifiProfile) error {
	if err := p.ApplyWpaProfile(ctx, iface, profile); err != nil {
		return connect.NewError(connect.CodeUnavailable, "could not configure the Wi-Fi network; verify wpa_supplicant control permissions")
	}
	if _, err := p.WPARequest(ctx, iface, "REASSOCIATE"); err != nil {
		return connect.NewError(connect.CodeUnavailable, "could not start the Wi-Fi connection")
	}
	return nil
}

func (p *Linux) WaitWifiConnected(ctx context.Context, iface, ssid string, timeout time.Duration) error {
	if timeout <= 0 {
		timeout = 12 * time.Second
	}
	deadline := time.NewTimer(timeout)
	defer deadline.Stop()
	ticker := time.NewTicker(500 * time.Millisecond)
	defer ticker.Stop()
	for {
		response, err := p.WPARequest(ctx, iface, "STATUS")
		if err == nil {
			state := ""
			connectedSSID := ""
			for _, line := range strings.Split(response, "\n") {
				key, value, ok := strings.Cut(line, "=")
				if !ok {
					continue
				}
				switch key {
				case "wpa_state":
					state = value
				case "ssid":
					connectedSSID = value
				}
			}
			if state == "COMPLETED" && connectedSSID == ssid {
				return nil
			}
		}
		select {
		case <-ctx.Done():
			return ctx.Err()
		case <-deadline.C:
			return connect.NewError(connect.CodeDeadlineExceeded, "Wi-Fi connection did not complete in time")
		case <-ticker.C:
		}
	}
}

// RestoreWifiProfiles reinstalls daemon-owned profiles into the active
// wpa_supplicant instance. The saved profile file remains the persistent source
// of truth; credentials are never written to logs or passed as argv.
func (p *Linux) RestoreWifiProfiles(ctx context.Context) error {
	profiles, err := p.LoadWifiProfiles()
	if err != nil || len(profiles) == 0 {
		return err
	}
	status, err := p.WifiStatus(ctx)
	if err != nil {
		return err
	}
	if status.Interface == "" || (status.RadioStateAvailable && !status.RadioEnabled) {
		return nil
	}
	for _, profile := range profiles {
		if err := p.ApplyWpaProfile(ctx, status.Interface, profile); err != nil {
			return connect.NewError(connect.CodeUnavailable, "saved Wi-Fi profiles could not be restored through wpa_supplicant")
		}
	}
	_, err = p.WPARequest(ctx, status.Interface, "REASSOCIATE")
	return err
}

func (p *Linux) MonitorWifiProfiles(ctx context.Context, retryInterval time.Duration) {
	if retryInterval <= 0 {
		retryInterval = 5 * time.Second
	}
	ticker := time.NewTicker(retryInterval)
	defer ticker.Stop()
	for {
		if err := p.RestoreWifiProfiles(ctx); err == nil {
			return
		}
		select {
		case <-ctx.Done():
			return
		case <-ticker.C:
		}
	}
}

func NewWifiProfileID() (string, error) {
	var id [12]byte
	if _, err := rand.Read(id[:]); err != nil {
		return "", fmt.Errorf("generate Wi-Fi network identifier: %w", err)
	}
	return hex.EncodeToString(id[:]), nil
}

func WifiProfileResponse(profiles []WifiProfile, status *nexusv1.WifiServiceGetStatusResponse) *nexusv1.WifiServiceListSavedNetworksResponse {
	response := &nexusv1.WifiServiceListSavedNetworksResponse{}
	for _, profile := range profiles {
		response.Networks = append(response.Networks, &nexusv1.SavedWifiNetwork{
			Id: profile.ID, Ssid: profile.SSID, Security: profile.Security,
			Connected: status != nil && status.Connected && profile.SSID == status.Ssid,
			Preferred: profile.Preferred, Priority: int32(profile.Priority),
		})
	}
	return response
}

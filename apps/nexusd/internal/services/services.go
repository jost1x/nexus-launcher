package services

import (
	"context"
	"encoding/hex"
	"errors"
	"fmt"
	"sort"
	"strings"
	"sync"
	"time"

	"connectrpc.com/connect/v2"
	"google.golang.org/protobuf/proto"
	nexusv1 "nexusd/gen/nexus/v1"
	nexusv1connect "nexusd/gen/nexus/v1/nexusv1connect"
	"nexusd/internal/platform"
)

type System struct{ Adapter *platform.Linux }

func (s System) GetInfo(ctx context.Context, _ *nexusv1.GetInfoRequest) (*nexusv1.GetInfoResponse, error) {
	return s.Adapter.SystemInfo(ctx)
}
func (s System) ListCapabilities(ctx context.Context, _ *nexusv1.ListCapabilitiesRequest) (*nexusv1.ListCapabilitiesResponse, error) {
	return s.Adapter.Capabilities(ctx), nil
}

const wifiScanCacheTTL = 15 * time.Second

type wifiScanCacheEntry struct {
	response  *nexusv1.ScanResponse
	scannedAt time.Time
}

type wifiScanCall struct {
	done     chan struct{}
	response *nexusv1.ScanResponse
	err      error
}

type Wifi struct {
	Adapter  *platform.Linux
	Watcher  *WifiStatusHub
	mu       sync.Mutex
	cache    map[string]wifiScanCacheEntry
	inFlight map[string]*wifiScanCall
}

func (s *Wifi) GetStatus(ctx context.Context, _ *nexusv1.WifiServiceGetStatusRequest) (*nexusv1.WifiServiceGetStatusResponse, error) {
	return s.Adapter.WifiStatus(ctx)
}

func (s *Wifi) SetEnabled(ctx context.Context, req *nexusv1.WifiServiceSetEnabledRequest) (*nexusv1.WifiServiceSetEnabledResponse, error) {
	if err := s.Adapter.SetWifiEnabled(ctx, req.GetEnabled()); err != nil {
		return nil, err
	}
	if req.GetEnabled() {
		if err := s.Adapter.RestoreWifiProfiles(ctx); err != nil {
			return nil, err
		}
	}
	status, err := s.Adapter.WifiStatus(ctx)
	if err != nil {
		return nil, err
	}
	return &nexusv1.WifiServiceSetEnabledResponse{Enabled: status.RadioEnabled, RadioStateAvailable: status.RadioStateAvailable}, nil
}

func (s *Wifi) ListSavedNetworks(ctx context.Context, _ *nexusv1.WifiServiceListSavedNetworksRequest) (*nexusv1.WifiServiceListSavedNetworksResponse, error) {
	profiles, err := s.Adapter.LoadWifiProfiles()
	if err != nil {
		return nil, connectError("saved Wi-Fi networks are unavailable", err)
	}
	status, _ := s.Adapter.WifiStatus(ctx)
	sort.SliceStable(profiles, func(i, j int) bool { return profiles[i].Priority > profiles[j].Priority })
	return platform.WifiProfileResponse(profiles, status), nil
}

func (s *Wifi) Connect(ctx context.Context, req *nexusv1.WifiServiceConnectRequest) (*nexusv1.WifiServiceConnectResponse, error) {
	ssid := req.GetSsid()
	if ssid == "" || len([]byte(ssid)) > 32 {
		return nil, connect.NewError(connect.CodeInvalidArgument, "SSID must contain between 1 and 32 bytes")
	}
	security := strings.ToLower(strings.TrimSpace(req.GetSecurity()))
	if security != "open" && security != "secured" && security != "wpa-psk" {
		return nil, connect.NewError(connect.CodeInvalidArgument, "unsupported Wi-Fi security type")
	}
	password := req.GetPassword()
	if security == "open" {
		password = ""
	} else if len(password) < 8 || (len(password) > 63 && !isHexPSK(password)) {
		return nil, connect.NewError(connect.CodeInvalidArgument, "Wi-Fi password must be 8–63 characters or a 64-character hexadecimal PSK")
	}
	status, err := s.Adapter.WifiStatus(ctx)
	if err != nil {
		return nil, err
	}
	if status.RadioStateAvailable && !status.RadioEnabled {
		return nil, connect.NewError(connect.CodeFailedPrecondition, "Wi-Fi is disabled")
	}
	if status.Interface == "" {
		return nil, connect.NewError(connect.CodeFailedPrecondition, "no Wi-Fi interface is available")
	}

	s.mu.Lock()
	defer s.mu.Unlock()
	profiles, err := s.Adapter.LoadWifiProfiles()
	if err != nil {
		return nil, connectError("saved Wi-Fi networks are unavailable", err)
	}
	previousProfiles := append([]platform.WifiProfile(nil), profiles...)
	profileIndex := -1
	for index := range profiles {
		if profiles[index].SSID == ssid {
			profileIndex = index
			break
		}
	}
	profile := platform.WifiProfile{SSID: ssid, Password: password, Security: security, Preferred: true}
	if profileIndex >= 0 {
		profile.ID = profiles[profileIndex].ID
		profiles = append(profiles[:profileIndex], profiles[profileIndex+1:]...)
	} else {
		profile.ID, err = platform.NewWifiProfileID()
		if err != nil {
			return nil, err
		}
	}
	profiles = append([]platform.WifiProfile{profile}, profiles...)
	normalizeWifiProfiles(profiles)
	if err := s.Adapter.SaveWifiProfiles(profiles); err != nil {
		return nil, connectError("could not save the Wi-Fi network", err)
	}
	if err := s.applyProfiles(ctx, status.Interface, profiles); err != nil {
		_ = s.Adapter.SaveWifiProfiles(previousProfiles)
		return nil, err
	}
	if err := s.Adapter.WaitWifiConnected(ctx, status.Interface, ssid, 11*time.Second); err != nil {
		_ = s.Adapter.SaveWifiProfiles(previousProfiles)
		if networkID, found, findErr := s.Adapter.FindWpaProfile(ctx, status.Interface, profile.ID); findErr == nil && found {
			_, _ = s.Adapter.WPARequest(ctx, status.Interface, fmt.Sprintf("REMOVE_NETWORK %d", networkID))
		}
		_ = s.applyProfiles(ctx, status.Interface, previousProfiles)
		return nil, connect.NewError(connect.CodeUnavailable, "Wi-Fi connection failed; check the password, signal, and network type")
	}
	current, err := s.Adapter.WifiStatus(ctx)
	if err != nil {
		return nil, err
	}
	return &nexusv1.WifiServiceConnectResponse{State: current.State, Connected: current.Connected, Ssid: current.Ssid}, nil
}

func (s *Wifi) ConnectSavedNetwork(ctx context.Context, req *nexusv1.WifiServiceConnectSavedNetworkRequest) (*nexusv1.WifiServiceConnectSavedNetworkResponse, error) {
	status, err := s.Adapter.WifiStatus(ctx)
	if err != nil {
		return nil, err
	}
	if status.RadioStateAvailable && !status.RadioEnabled {
		return nil, connect.NewError(connect.CodeFailedPrecondition, "Wi-Fi is disabled")
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	profiles, err := s.Adapter.LoadWifiProfiles()
	if err != nil {
		return nil, connectError("saved Wi-Fi networks are unavailable", err)
	}
	index := findProfile(profiles, req.GetId())
	if index < 0 {
		return nil, connect.NewError(connect.CodeNotFound, "saved Wi-Fi network was not found")
	}
	profile := profiles[index]
	profiles = append([]platform.WifiProfile{profile}, append(profiles[:index], profiles[index+1:]...)...)
	normalizeWifiProfiles(profiles)
	if err := s.Adapter.SaveWifiProfiles(profiles); err != nil {
		return nil, connectError("could not update the preferred Wi-Fi network", err)
	}
	if status.Interface == "" {
		return nil, connect.NewError(connect.CodeFailedPrecondition, "no Wi-Fi interface is available")
	}
	if err := s.applyProfiles(ctx, status.Interface, profiles); err != nil {
		return nil, err
	}
	if err := s.Adapter.WaitWifiConnected(ctx, status.Interface, profile.SSID, 11*time.Second); err != nil {
		return nil, connect.NewError(connect.CodeUnavailable, "could not connect to the saved Wi-Fi network")
	}
	current, err := s.Adapter.WifiStatus(ctx)
	if err != nil {
		return nil, err
	}
	return &nexusv1.WifiServiceConnectSavedNetworkResponse{State: current.State, Connected: current.Connected, Ssid: current.Ssid}, nil
}

func (s *Wifi) SetPreferredNetwork(ctx context.Context, req *nexusv1.WifiServiceSetPreferredNetworkRequest) (*nexusv1.WifiServiceSetPreferredNetworkResponse, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	profiles, err := s.Adapter.LoadWifiProfiles()
	if err != nil {
		return nil, connectError("saved Wi-Fi networks are unavailable", err)
	}
	index := findProfile(profiles, req.GetId())
	if index < 0 {
		return nil, connect.NewError(connect.CodeNotFound, "saved Wi-Fi network was not found")
	}
	profile := profiles[index]
	profiles = append([]platform.WifiProfile{profile}, append(profiles[:index], profiles[index+1:]...)...)
	normalizeWifiProfiles(profiles)
	if err := s.Adapter.SaveWifiProfiles(profiles); err != nil {
		return nil, connectError("could not save the preferred Wi-Fi network", err)
	}
	status, _ := s.Adapter.WifiStatus(ctx)
	if status != nil && status.Interface != "" {
		if err := s.applyProfiles(ctx, status.Interface, profiles); err != nil {
			return nil, err
		}
	}
	return &nexusv1.WifiServiceSetPreferredNetworkResponse{Networks: platform.WifiProfileResponse(profiles, status).GetNetworks()}, nil
}

func (s *Wifi) ForgetNetwork(ctx context.Context, req *nexusv1.WifiServiceForgetNetworkRequest) (*nexusv1.WifiServiceForgetNetworkResponse, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	profiles, err := s.Adapter.LoadWifiProfiles()
	if err != nil {
		return nil, connectError("saved Wi-Fi networks are unavailable", err)
	}
	index := findProfile(profiles, req.GetId())
	if index < 0 {
		return nil, connect.NewError(connect.CodeNotFound, "saved Wi-Fi network was not found")
	}
	profile := profiles[index]
	profiles = append(profiles[:index], profiles[index+1:]...)
	if len(profiles) > 0 {
		normalizeWifiProfiles(profiles)
	}
	if err := s.Adapter.SaveWifiProfiles(profiles); err != nil {
		return nil, connectError("could not forget the Wi-Fi network", err)
	}
	status, _ := s.Adapter.WifiStatus(ctx)
	if status != nil && status.Interface != "" {
		if networkID, found, findErr := s.Adapter.FindWpaProfile(ctx, status.Interface, profile.ID); findErr == nil && found {
			_, _ = s.Adapter.WPARequest(ctx, status.Interface, fmt.Sprintf("REMOVE_NETWORK %d", networkID))
		}
	}
	return &nexusv1.WifiServiceForgetNetworkResponse{Networks: platform.WifiProfileResponse(profiles, status).GetNetworks()}, nil
}

func (s *Wifi) applyProfiles(ctx context.Context, iface string, profiles []platform.WifiProfile) error {
	for _, profile := range profiles {
		if err := s.Adapter.ApplyWpaProfile(ctx, iface, profile); err != nil {
			return connect.NewError(connect.CodeUnavailable, "saved Wi-Fi profiles could not be configured through wpa_supplicant")
		}
	}
	if len(profiles) > 0 {
		if _, err := s.Adapter.WPARequest(ctx, iface, "REASSOCIATE"); err != nil {
			return connect.NewError(connect.CodeUnavailable, "could not start the Wi-Fi connection")
		}
	}
	return nil
}

func findProfile(profiles []platform.WifiProfile, id string) int {
	for index := range profiles {
		if profiles[index].ID == id {
			return index
		}
	}
	return -1
}

func normalizeWifiProfiles(profiles []platform.WifiProfile) {
	for index := range profiles {
		profiles[index].Preferred = index == 0
		profiles[index].Priority = len(profiles) - index
	}
}

func isHexPSK(value string) bool {
	if len(value) != 64 {
		return false
	}
	_, err := hex.DecodeString(value)
	return err == nil
}

func connectError(message string, cause error) error {
	if cause == nil {
		return errors.New(message)
	}
	return connect.NewError(connect.CodeUnavailable, fmt.Sprintf("%s: %v", message, cause))
}
func (s *Wifi) WatchStatus(ctx context.Context, _ *nexusv1.WifiServiceWatchStatusRequest, stream nexusv1connect.WifiServiceWatchStatusServerStream) error {
	if s.Watcher == nil {
		return fmt.Errorf("Wi-Fi status notifications are unavailable")
	}
	updates, unsubscribe := s.Watcher.subscribe()
	defer unsubscribe()

	initial, err := s.Adapter.WifiStatus(ctx)
	if err != nil {
		return err
	}
	previous := wifiWatchStatus(initial)
	if err := stream.Send(previous); err != nil {
		return err
	}
	for {
		select {
		case <-ctx.Done():
			return nil
		case <-updates:
			currentStatus, err := s.Adapter.WifiStatus(ctx)
			if err != nil {
				return err
			}
			current := wifiWatchStatus(currentStatus)
			if proto.Equal(previous, current) {
				continue
			}
			if err := stream.Send(current); err != nil {
				return err
			}
			previous = current
		}
	}
}
func (s *Wifi) Scan(ctx context.Context, req *nexusv1.ScanRequest) (*nexusv1.ScanResponse, error) {
	iface := req.GetInterface()
	s.mu.Lock()
	if !req.GetRefresh() {
		if entry, ok := s.cache[iface]; ok && time.Since(entry.scannedAt) < wifiScanCacheTTL {
			response := cloneScanResponse(entry.response)
			response.FromCache = true
			s.mu.Unlock()
			s.markCurrentConnection(ctx, iface, response)
			return response, nil
		}
	}
	if call, ok := s.inFlight[iface]; ok {
		s.mu.Unlock()
		select {
		case <-ctx.Done():
			return nil, ctx.Err()
		case <-call.done:
			if call.err != nil {
				return nil, call.err
			}
			response := cloneScanResponse(call.response)
			response.FromCache = true
			s.markCurrentConnection(ctx, iface, response)
			return response, nil
		}
	}

	call := &wifiScanCall{done: make(chan struct{})}
	if s.inFlight == nil {
		s.inFlight = make(map[string]*wifiScanCall)
	}
	s.inFlight[iface] = call
	s.mu.Unlock()

	response, err := s.Adapter.ScanWifi(ctx, iface)
	scannedAt := time.Now()
	if err == nil {
		s.markCurrentConnection(ctx, iface, response)
		response.ScannedAtUnix = scannedAt.Unix()
		response.FromCache = false
	}

	s.mu.Lock()
	call.response = response
	call.err = err
	if err == nil {
		if s.cache == nil {
			s.cache = make(map[string]wifiScanCacheEntry)
		}
		s.cache[iface] = wifiScanCacheEntry{response: cloneScanResponse(response), scannedAt: scannedAt}
	}
	delete(s.inFlight, iface)
	close(call.done)
	s.mu.Unlock()
	return response, err
}

func (s *Wifi) markCurrentConnection(ctx context.Context, iface string, response *nexusv1.ScanResponse) {
	for _, network := range response.Networks {
		network.Connected = false
	}
	status, err := s.Adapter.WifiStatus(ctx)
	if err != nil || !status.Connected || status.Bssid == "" || (iface != "" && !strings.EqualFold(iface, status.Interface)) {
		return
	}
	for _, network := range response.Networks {
		network.Connected = strings.EqualFold(network.Bssid, status.Bssid)
	}
}

func cloneScanResponse(response *nexusv1.ScanResponse) *nexusv1.ScanResponse {
	return proto.Clone(response).(*nexusv1.ScanResponse)
}

type Hardware struct {
	Adapter *platform.Linux
	Watcher *HardwareStatusHub
}

func (s Hardware) notifyStatusChanged() {
	if s.Watcher != nil {
		s.Watcher.Notify()
	}
}

func (s Hardware) GetStatus(ctx context.Context, _ *nexusv1.HardwareServiceGetStatusRequest) (*nexusv1.HardwareServiceGetStatusResponse, error) {
	return s.Adapter.HardwareStatus(ctx)
}
func (s Hardware) WatchStatus(ctx context.Context, _ *nexusv1.HardwareServiceWatchStatusRequest, stream nexusv1connect.HardwareServiceWatchStatusServerStream) error {
	if s.Watcher == nil {
		return fmt.Errorf("hardware status notifications are unavailable")
	}
	updates, unsubscribe := s.Watcher.subscribe()
	defer unsubscribe()

	initial, err := s.Adapter.HardwareStatus(ctx)
	if err != nil {
		return err
	}
	previous := hardwareWatchStatus(initial)
	if err := stream.Send(previous); err != nil {
		return err
	}
	for {
		select {
		case <-ctx.Done():
			return nil
		case <-updates:
			currentStatus, err := s.Adapter.HardwareStatus(ctx)
			if err != nil {
				return err
			}
			current := hardwareWatchStatus(currentStatus)
			if proto.Equal(previous, current) {
				continue
			}
			if err := stream.Send(current); err != nil {
				return err
			}
			previous = current
		}
	}
}

func wifiWatchStatus(status *nexusv1.WifiServiceGetStatusResponse) *nexusv1.WifiServiceWatchStatusResponse {
	return &nexusv1.WifiServiceWatchStatusResponse{
		Interface: status.Interface, State: status.State, Connected: status.Connected, Ssid: status.Ssid,
		Manager: status.Manager, Detail: status.Detail, Bssid: status.Bssid,
		RadioEnabled: status.RadioEnabled, RadioStateAvailable: status.RadioStateAvailable,
	}
}

func hardwareWatchStatus(status *nexusv1.HardwareServiceGetStatusResponse) *nexusv1.HardwareServiceWatchStatusResponse {
	return &nexusv1.HardwareServiceWatchStatusResponse{
		BatteryAvailable: status.BatteryAvailable, BatteryStatus: status.BatteryStatus,
		BatteryPercent: status.BatteryPercent, BatteryVoltageMicrovolts: status.BatteryVoltageMicrovolts,
		ThermalAvailable: status.ThermalAvailable, SocTemperatureCelsius: status.SocTemperatureCelsius,
		BrightnessAvailable: status.BrightnessAvailable, Brightness: status.Brightness,
		BrightnessMax: status.BrightnessMax, AudioAvailable: status.AudioAvailable, AudioDetail: status.AudioDetail,
		VolumeAvailable: status.VolumeAvailable, VolumePercent: status.VolumePercent,
		ChargerAvailable: status.ChargerAvailable, ChargerConnected: status.ChargerConnected,
		BatteryCharging: status.BatteryCharging,
	}
}

func (s Hardware) SetBrightness(_ context.Context, req *nexusv1.SetBrightnessRequest) (*nexusv1.SetBrightnessResponse, error) {
	value, err := s.Adapter.SetBrightness(int(req.GetPercent()), req.GetConfirmed())
	if err != nil {
		return nil, err
	}
	s.notifyStatusChanged()
	return &nexusv1.SetBrightnessResponse{Value: value}, nil
}
func (s Hardware) SetVolume(ctx context.Context, req *nexusv1.SetVolumeRequest) (*nexusv1.SetVolumeResponse, error) {
	if err := s.Adapter.SetVolume(ctx, int(req.GetPercent()), req.GetConfirmed()); err != nil {
		return nil, err
	}
	s.notifyStatusChanged()
	return &nexusv1.SetVolumeResponse{Percent: req.GetPercent()}, nil
}
func (s Hardware) PowerAction(ctx context.Context, req *nexusv1.PowerActionRequest) (*nexusv1.PowerActionResponse, error) {
	if err := s.Adapter.PowerAction(ctx, req.GetAction(), req.GetConfirmed()); err != nil {
		return nil, err
	}
	return &nexusv1.PowerActionResponse{Message: "power action accepted"}, nil
}

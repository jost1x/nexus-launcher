package services

import (
	"context"
	"strings"
	"sync"
	"time"

	nexusv1 "nexusd/gen/nexus/v1"
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
	mu       sync.Mutex
	cache    map[string]wifiScanCacheEntry
	inFlight map[string]*wifiScanCall
}

func (s *Wifi) GetStatus(ctx context.Context, _ *nexusv1.WifiServiceGetStatusRequest) (*nexusv1.WifiServiceGetStatusResponse, error) {
	return s.Adapter.WifiStatus(ctx)
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
	clone := *response
	clone.Networks = make([]*nexusv1.WifiNetwork, len(response.Networks))
	for i, network := range response.Networks {
		networkClone := *network
		clone.Networks[i] = &networkClone
	}
	return &clone
}

type Hardware struct{ Adapter *platform.Linux }

func (s Hardware) GetStatus(ctx context.Context, _ *nexusv1.HardwareServiceGetStatusRequest) (*nexusv1.HardwareServiceGetStatusResponse, error) {
	return s.Adapter.HardwareStatus(ctx)
}
func (s Hardware) SetBrightness(_ context.Context, req *nexusv1.SetBrightnessRequest) (*nexusv1.SetBrightnessResponse, error) {
	value, err := s.Adapter.SetBrightness(int(req.GetPercent()), req.GetConfirmed())
	if err != nil {
		return nil, err
	}
	return &nexusv1.SetBrightnessResponse{Value: value}, nil
}
func (s Hardware) SetVolume(ctx context.Context, req *nexusv1.SetVolumeRequest) (*nexusv1.SetVolumeResponse, error) {
	if err := s.Adapter.SetVolume(ctx, int(req.GetPercent()), req.GetConfirmed()); err != nil {
		return nil, err
	}
	return &nexusv1.SetVolumeResponse{Percent: req.GetPercent()}, nil
}
func (s Hardware) PowerAction(ctx context.Context, req *nexusv1.PowerActionRequest) (*nexusv1.PowerActionResponse, error) {
	if err := s.Adapter.PowerAction(ctx, req.GetAction(), req.GetConfirmed()); err != nil {
		return nil, err
	}
	return &nexusv1.PowerActionResponse{Message: "power action accepted"}, nil
}

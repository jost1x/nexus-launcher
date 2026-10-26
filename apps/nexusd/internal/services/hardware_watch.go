package services

import (
	"context"
	"sync"
	"time"

	"google.golang.org/protobuf/proto"
	"nexusd/internal/platform"
)

// HardwareStatusHub wakes active WatchStatus streams when a hardware change occurs.
// A one-element channel coalesces bursts so a slow client cannot block nexusd.
type HardwareStatusHub struct {
	mu          sync.Mutex
	subscribers map[chan struct{}]struct{}
}

func NewHardwareStatusHub() *HardwareStatusHub {
	return &HardwareStatusHub{subscribers: make(map[chan struct{}]struct{})}
}

func (h *HardwareStatusHub) subscribe() (<-chan struct{}, func()) {
	updates := make(chan struct{}, 1)
	h.mu.Lock()
	h.subscribers[updates] = struct{}{}
	h.mu.Unlock()
	return updates, func() {
		h.mu.Lock()
		delete(h.subscribers, updates)
		h.mu.Unlock()
	}
}

// Notify signals subscribers without waiting for them to consume previous updates.
func (h *HardwareStatusHub) Notify() {
	h.mu.Lock()
	defer h.mu.Unlock()
	for updates := range h.subscribers {
		select {
		case updates <- struct{}{}:
		default:
		}
	}
}

// MonitorHardwareStatus is a low-frequency fallback for changes made outside nexusd.
func (h *HardwareStatusHub) MonitorHardwareStatus(ctx context.Context, adapter *platform.Linux, interval time.Duration) {
	if interval <= 0 {
		interval = 2 * time.Second
	}
	previous, err := adapter.HardwareStatus(ctx)
	if err != nil {
		previous = nil
	}
	ticker := time.NewTicker(interval)
	defer ticker.Stop()
	for {
		select {
		case <-ctx.Done():
			return
		case <-ticker.C:
			current, err := adapter.HardwareStatus(ctx)
			if err != nil {
				continue
			}
			if previous == nil || !proto.Equal(previous, current) {
				h.Notify()
				previous = current
			}
		}
	}
}

// WifiStatusHub uses the same coalescing notifications for Wi-Fi status streams.
type WifiStatusHub struct {
	*HardwareStatusHub
}

func NewWifiStatusHub() *WifiStatusHub {
	return &WifiStatusHub{HardwareStatusHub: NewHardwareStatusHub()}
}

// MonitorWifiStatus detects connection changes made outside nexusd.
func (h *WifiStatusHub) MonitorWifiStatus(ctx context.Context, adapter *platform.Linux, interval time.Duration) {
	if interval <= 0 {
		interval = 2 * time.Second
	}
	previous, err := adapter.WifiStatus(ctx)
	if err != nil {
		previous = nil
	}
	ticker := time.NewTicker(interval)
	defer ticker.Stop()
	for {
		select {
		case <-ctx.Done():
			return
		case <-ticker.C:
			current, err := adapter.WifiStatus(ctx)
			if err != nil {
				continue
			}
			if previous == nil || !proto.Equal(previous, current) {
				h.Notify()
				previous = current
			}
		}
	}
}

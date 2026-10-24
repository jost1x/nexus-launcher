package platform

import (
	"bufio"
	"context"
	"encoding/binary"
	"errors"
	"fmt"
	"log/slog"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strconv"
	"strings"
	"syscall"
	"time"

	"connectrpc.com/connect/v2"
	nexusv1 "nexusd/gen/nexus/v1"
)

// Linux contains read-only discovery and narrowly scoped hardware operations.
type Linux struct {
	SysRoot  string
	ProcRoot string
	DevRoot  string
	LookPath func(string) (string, error)
	Run      func(context.Context, string, ...string) ([]byte, error)
}

func NewLinux() *Linux {
	return &Linux{SysRoot: "/sys", ProcRoot: "/proc", DevRoot: "/dev", LookPath: findCommand, Run: runCommand}
}

const (
	inputEventKey  = 1
	keyVolumeDown  = 114
	keyVolumeUp    = 115
	keyFunction    = 708 // BTN_TRIGGER_HAPPY5 observed for the R36S F button
	volumeKeyStep  = 5
	inputEventSize = 24 // struct input_event on the supported 64-bit Linux targets.
)

// WatchVolumeKeys applies the R36S gpio-keys volume buttons to the ALSA master.
// It leaves the input device available to other consumers and ignores unrelated keys.
func (p *Linux) WatchVolumeKeys(ctx context.Context) error {
	device, err := p.volumeKeyDevice()
	if err != nil {
		return err
	}
	f, err := os.OpenFile(device, os.O_RDONLY|syscall.O_NONBLOCK, 0)
	if err != nil {
		return fmt.Errorf("open volume key device %s: %w", device, err)
	}
	defer f.Close()
	slog.Info("listening for volume keys", "device", device)
	buffer := make([]byte, inputEventSize*16)
	functionDown := false
	for {
		if err := ctx.Err(); err != nil {
			return nil
		}
		n, readErr := f.Read(buffer)
		if readErr != nil {
			if errors.Is(readErr, syscall.EAGAIN) || errors.Is(readErr, syscall.EWOULDBLOCK) {
				select {
				case <-ctx.Done():
					return nil
				case <-time.After(25 * time.Millisecond):
					continue
				}
			}
			return fmt.Errorf("read volume key device: %w", readErr)
		}
		for offset := 0; offset+inputEventSize <= n; offset += inputEventSize {
			event := buffer[offset : offset+inputEventSize]
			typ := binary.LittleEndian.Uint16(event[16:18])
			code := binary.LittleEndian.Uint16(event[18:20])
			value := int32(binary.LittleEndian.Uint32(event[20:24]))
			if typ != inputEventKey || (value != 1 && value != 2) {
				if typ == inputEventKey && code == keyFunction && value == 0 {
					functionDown = false
				}
				continue
			}
			if code == keyFunction {
				functionDown = true
				continue
			}
			delta := 0
			switch code {
			case keyVolumeUp:
				delta = volumeKeyStep
			case keyVolumeDown:
				delta = -volumeKeyStep
			default:
				continue
			}
			var err error
			if functionDown {
				err = p.adjustBrightness(delta)
			} else {
				err = p.adjustVolume(ctx, delta)
			}
			if err != nil {
				slog.Warn("hardware key could not change level", "error", err)
			}
		}
	}
}

func (p *Linux) volumeKeyDevice() (string, error) {
	entries, err := filepath.Glob(filepath.Join(p.SysRoot, "class/input/event*/device/name"))
	if err != nil {
		return "", err
	}
	for _, namePath := range entries {
		name, err := os.ReadFile(namePath)
		if err != nil || strings.TrimSpace(string(name)) != "gpio-keys" {
			continue
		}
		event := filepath.Base(filepath.Dir(filepath.Dir(namePath)))
		device := filepath.Join(p.DevRoot, "input", event)
		if _, err := os.Stat(device); err == nil {
			return device, nil
		}
	}
	return "", errors.New("gpio-keys input device not found")
}

func (p *Linux) adjustVolume(ctx context.Context, delta int) error {
	current, err := p.volumePercent(ctx)
	if err != nil {
		return err
	}
	next := current + delta
	if next < 0 {
		next = 0
	} else if next > 100 {
		next = 100
	}
	if next == current {
		return nil
	}
	return p.SetVolume(ctx, next, true)
}

func (p *Linux) adjustBrightness(delta int) error {
	entry, err := p.backlight()
	if err != nil {
		return err
	}
	current, err := readInt(filepath.Join(entry, "brightness"))
	if err != nil {
		return err
	}
	maximum, err := readInt(filepath.Join(entry, "max_brightness"))
	if err != nil || maximum <= 0 {
		return errors.New("backlight maximum is unavailable")
	}
	percent := int(current * 100 / maximum)
	next := percent + delta
	if next < 0 {
		next = 0
	} else if next > 100 {
		next = 100
	}
	if next == percent {
		return nil
	}
	_, err = p.SetBrightness(next, true)
	return err
}

func findCommand(name string) (string, error) {
	return findCommandIn(name, exec.LookPath, []string{"/usr/local/sbin", "/usr/sbin", "/sbin"})
}

func findCommandIn(name string, lookup func(string) (string, error), dirs []string) (string, error) {
	if path, err := lookup(name); err == nil {
		return path, nil
	}
	for _, dir := range dirs {
		path := filepath.Join(dir, name)
		info, err := os.Stat(path)
		if err == nil && info.Mode().IsRegular() && info.Mode().Perm()&0o111 != 0 {
			return path, nil
		}
	}
	return "", exec.ErrNotFound
}

func runCommand(ctx context.Context, name string, args ...string) ([]byte, error) {
	return exec.CommandContext(ctx, name, args...).CombinedOutput()
}

func (p *Linux) SystemInfo(context.Context) (*nexusv1.GetInfoResponse, error) {
	info := &nexusv1.GetInfoResponse{Architecture: runtime.GOARCH}
	if hostname, err := os.Hostname(); err == nil {
		info.Hostname = hostname
	}
	if data, err := os.ReadFile("/etc/os-release"); err == nil {
		for _, line := range strings.Split(string(data), "\n") {
			key, value, ok := strings.Cut(line, "=")
			if !ok {
				continue
			}
			value = strings.Trim(value, "\"'")
			switch key {
			case "ID":
				info.OsId = value
			case "VERSION_ID":
				info.OsVersion = value
			}
		}
	}
	if data, err := os.ReadFile(filepath.Join(p.ProcRoot, "sys/kernel/osrelease")); err == nil {
		info.Kernel = strings.TrimSpace(string(data))
	}
	if data, err := os.ReadFile(filepath.Join(p.ProcRoot, "meminfo")); err == nil {
		for _, line := range strings.Split(string(data), "\n") {
			var key string
			var kb uint64
			if _, err := fmt.Sscanf(line, "%s %d kB", &key, &kb); err != nil {
				continue
			}
			switch key {
			case "MemTotal:":
				info.MemoryTotalBytes = kb * 1024
			case "MemAvailable:":
				info.MemoryAvailableBytes = kb * 1024
			}
		}
	}
	if data, err := os.ReadFile(filepath.Join(p.ProcRoot, "uptime")); err == nil {
		var seconds float64
		if _, err := fmt.Sscanf(string(data), "%f", &seconds); err == nil && seconds > 0 {
			info.UptimeSeconds = uint64(seconds)
		}
	}
	if data, err := os.ReadFile(filepath.Join(p.ProcRoot, "loadavg")); err == nil {
		fields := strings.Fields(string(data))
		if len(fields) >= 3 {
			info.LoadAverage = strings.Join(fields[:3], " ")
		}
	}
	return info, nil
}

func (p *Linux) Capabilities(ctx context.Context) *nexusv1.ListCapabilitiesResponse {
	items := []*nexusv1.Capability{
		{Name: "system.info", Available: true, Detail: "Linux procfs and os-release"},
		{Name: "wifi.status", Available: false, Detail: "No wireless interface detected"},
		{Name: "wifi.scan", Available: false, Detail: "Requires iw and scan permissions"},
		{Name: "hardware.battery", Available: false, Detail: "No power_supply entries detected"},
		{Name: "hardware.thermal", Available: false, Detail: "No thermal zones detected"},
		{Name: "hardware.brightness", Available: false, Detail: "No backlight sysfs entry detected"},
		{Name: "hardware.audio", Available: false, Detail: "No ALSA device or amixer detected"},
		{Name: "hardware.volume", Available: p.LookPath != nil && look(p.LookPath, "amixer"), Detail: "Requires an ALSA Master mixer control"},
		{Name: "hardware.power_action", Available: p.LookPath != nil && look(p.LookPath, "busctl"), Detail: "Requires explicit CLI confirmation and a restricted systemd-logind polkit rule"},
	}
	wifiAvailable := false
	if _, err := p.wifiInterface(""); err == nil {
		wifiAvailable = true
		items[1].Available = true
		items[1].Detail = "Wireless interface detected"
	}
	if wifiAvailable && p.LookPath != nil && look(p.LookPath, "iw") {
		items[2].Available = true
		items[2].Detail = "iw detected; scan may require elevated permissions"
	} else if !wifiAvailable {
		items[2].Detail = "No wireless interface detected"
	}
	if entries, _ := filepath.Glob(filepath.Join(p.SysRoot, "class/power_supply/*")); len(entries) > 0 {
		items[3].Available = true
		items[3].Detail = "Power supply sysfs entries detected"
	}
	if entries, _ := filepath.Glob(filepath.Join(p.SysRoot, "class/thermal/thermal_zone*")); len(entries) > 0 {
		items[4].Available = true
		items[4].Detail = "Thermal zones detected"
	}
	if _, err := p.backlight(); err == nil {
		items[5].Available = true
		items[5].Detail = "Backlight sysfs entry detected; writes may require permissions"
	}
	_, sndErr := os.Stat("/dev/snd")
	if sndErr == nil || (p.LookPath != nil && look(p.LookPath, "amixer")) {
		items[6].Available = true
		items[6].Detail = "ALSA device or amixer detected"
	}
	if _, err := p.masterControl(ctx); err == nil {
		items[7].Available = true
		items[7].Detail = "Supported ALSA mixer control detected"
	}
	if p.isR36S() {
		for _, item := range items {
			if item.Available && item.Name != "hardware.power_action" && item.Name != "wifi.scan" {
				item.VerifiedOnR36S = true
			}
		}
	}
	return &nexusv1.ListCapabilitiesResponse{Items: items}
}

func look(fn func(string) (string, error), name string) bool { _, err := fn(name); return err == nil }

func (p *Linux) WifiStatus(ctx context.Context) (*nexusv1.WifiServiceGetStatusResponse, error) {
	iface, err := p.wifiInterface("")
	if err != nil {
		return &nexusv1.WifiServiceGetStatusResponse{State: "unavailable", Detail: err.Error()}, nil
	}
	state, _ := os.ReadFile(filepath.Join(p.SysRoot, "class/net", iface, "operstate"))
	status := &nexusv1.WifiServiceGetStatusResponse{Interface: iface, State: strings.TrimSpace(string(state))}
	if p.LookPath != nil {
		if look(p.LookPath, "nmcli") {
			status.Manager = "NetworkManager"
		} else if look(p.LookPath, "networkctl") {
			status.Manager = "systemd-networkd"
		} else if look(p.LookPath, "wpa_cli") {
			status.Manager = "wpa_supplicant"
		}
	}
	if status.Manager == "" {
		status.Detail = "Interface exists; connection manager not identified"
	}
	if p.LookPath != nil && status.Manager == "systemd-networkd" {
		if networkctl, err := p.LookPath("networkctl"); err == nil {
			output, runErr := p.Run(ctx, networkctl, "status", iface, "--no-pager")
			if runErr == nil && strings.Contains(string(output), "State: routable") {
				status.Connected = true
				status.Detail = "systemd-networkd reports a routable link"
			} else if status.State == "up" {
				status.Detail = "interface is up; networkd does not report a routable link"
			}
		}
	}
	if p.LookPath != nil {
		if iw, err := p.LookPath("iw"); err == nil {
			output, runErr := p.Run(ctx, iw, "dev", iface, "link")
			if runErr == nil {
				connected, ssid, bssid := parseWifiLink(output)
				status.Connected = connected
				status.Ssid = ssid
				status.Bssid = bssid
				if connected {
					status.Detail = "Associated with a Wi-Fi access point"
				} else {
					status.Detail = "Wireless interface is not associated with an access point"
				}
			} else if status.Connected {
				status.Detail = "systemd-networkd reports a routable link; iw could not read the access point"
			}
		}
	}
	return status, nil
}

func parseWifiLink(output []byte) (bool, string, string) {
	connected := false
	ssid := ""
	bssid := ""
	for _, line := range strings.Split(string(output), "\n") {
		line = strings.TrimSpace(line)
		if strings.HasPrefix(line, "Connected to ") {
			connected = true
			fields := strings.Fields(line)
			if len(fields) >= 3 {
				bssid = fields[2]
			}
		}
		if strings.HasPrefix(line, "SSID:") {
			ssid = strings.TrimSpace(strings.TrimPrefix(line, "SSID:"))
		}
	}
	return connected, ssid, bssid
}

func (p *Linux) ScanWifi(ctx context.Context, requested string) (*nexusv1.ScanResponse, error) {
	iface, err := p.wifiInterface(requested)
	if err != nil {
		return nil, connect.NewError(connect.CodeFailedPrecondition, err.Error())
	}
	if p.LookPath == nil {
		return nil, connect.NewError(connect.CodeFailedPrecondition, "cannot locate iw")
	}
	iw, err := p.LookPath("iw")
	if err != nil {
		return nil, connect.NewError(connect.CodeFailedPrecondition, "Wi-Fi scan requires the iw utility")
	}
	output, err := p.Run(ctx, iw, "dev", iface, "scan")
	if err != nil {
		if errors.Is(ctx.Err(), context.DeadlineExceeded) || errors.Is(ctx.Err(), context.Canceled) {
			return nil, connect.NewError(connect.CodeCanceled, ctx.Err().Error())
		}
		if errors.Is(err, syscall.EPERM) || errors.Is(err, syscall.EACCES) || strings.Contains(strings.ToLower(string(output)), "permission") || strings.Contains(strings.ToLower(string(output)), "not permitted") {
			return nil, connect.NewError(connect.CodePermissionDenied, "Wi-Fi scan requires network administration permission")
		}
		return nil, connect.NewError(connect.CodeUnavailable, fmt.Sprintf("iw scan failed: %s: %v", strings.TrimSpace(string(output)), err))
	}
	return parseScan(output), nil
}

func parseScan(output []byte) *nexusv1.ScanResponse {
	response := &nexusv1.ScanResponse{}
	var network *nexusv1.WifiNetwork
	scanner := bufio.NewScanner(strings.NewReader(string(output)))
	for scanner.Scan() {
		line := strings.TrimSpace(scanner.Text())
		if strings.HasPrefix(line, "BSS ") {
			if network != nil && network.Ssid != "" {
				response.Networks = append(response.Networks, network)
			}
			network = &nexusv1.WifiNetwork{}
			fields := strings.Fields(line)
			if len(fields) >= 2 {
				bssid, _, _ := strings.Cut(fields[1], "(")
				network.Bssid = strings.TrimSpace(bssid)
			}
			continue
		}
		if network == nil {
			continue
		}
		if strings.HasPrefix(line, "SSID:") {
			network.Ssid = strings.TrimSpace(strings.TrimPrefix(line, "SSID:"))
		}
		if strings.HasPrefix(line, "signal:") {
			var dbm float64
			if _, err := fmt.Sscanf(strings.TrimPrefix(line, "signal:"), "%f", &dbm); err == nil {
				percent := int((dbm + 100) * 2)
				if percent < 0 {
					percent = 0
				}
				if percent > 100 {
					percent = 100
				}
				network.SignalPercent = int32(percent)
			}
		}
		if strings.Contains(line, "WPA:") || strings.Contains(line, "RSN:") {
			network.Security = "secured"
		}
	}
	if network != nil && network.Ssid != "" {
		response.Networks = append(response.Networks, network)
	}
	return response
}

func (p *Linux) HardwareStatus(ctx context.Context) (*nexusv1.HardwareServiceGetStatusResponse, error) {
	status := &nexusv1.HardwareServiceGetStatusResponse{}
	var batteryCurrentMicroamps int64
	if entries, _ := filepath.Glob(filepath.Join(p.SysRoot, "class/power_supply/*")); len(entries) > 0 {
		for _, entry := range entries {
			typeBytes, _ := os.ReadFile(filepath.Join(entry, "type"))
			supplyType := strings.TrimSpace(string(typeBytes))
			if strings.EqualFold(supplyType, "Battery") {
				status.BatteryAvailable = true
				state, _ := os.ReadFile(filepath.Join(entry, "status"))
				status.BatteryStatus = strings.TrimSpace(string(state))
				status.BatteryCharging = strings.EqualFold(status.BatteryStatus, "Charging")
				batteryCurrentMicroamps, _ = readInt(filepath.Join(entry, "current_avg"))
				capacity, _ := os.ReadFile(filepath.Join(entry, "capacity"))
				if v, err := strconv.Atoi(strings.TrimSpace(string(capacity))); err == nil {
					status.BatteryPercent = int32(v)
				}
				for _, voltageAttribute := range []string{"voltage_now", "voltage_avg"} {
					if v, err := readInt(filepath.Join(entry, voltageAttribute)); err == nil {
						status.BatteryVoltageMicrovolts = v
						break
					}
				}
				continue
			}
			if !isChargerSupply(supplyType) {
				continue
			}
			online, err := os.ReadFile(filepath.Join(entry, "online"))
			if err != nil {
				continue
			}
			status.ChargerAvailable = true
			if value, err := strconv.Atoi(strings.TrimSpace(string(online))); err == nil && value == 1 {
				status.ChargerConnected = true
			}
		}
	}
	// On the R36S RK817, charge_now and current_avg increase while charging
	// even though the driver's status attribute can incorrectly say Not charging.
	if status.ChargerConnected && !strings.EqualFold(status.BatteryStatus, "Full") && batteryCurrentMicroamps > 0 {
		status.BatteryCharging = true
		if strings.EqualFold(status.BatteryStatus, "Not charging") {
			status.BatteryStatus = "Charging"
		}
	}
	if zones, _ := filepath.Glob(filepath.Join(p.SysRoot, "class/thermal/thermal_zone*")); len(zones) > 0 {
		for _, zone := range zones {
			typ, _ := os.ReadFile(filepath.Join(zone, "type"))
			if strings.Contains(strings.ToLower(string(typ)), "soc") || status.SocTemperatureCelsius == 0 {
				temp, _ := os.ReadFile(filepath.Join(zone, "temp"))
				if milli, err := strconv.ParseFloat(strings.TrimSpace(string(temp)), 64); err == nil {
					status.ThermalAvailable = true
					status.SocTemperatureCelsius = milli / 1000
					if strings.Contains(strings.ToLower(string(typ)), "soc") {
						break
					}
				}
			}
		}
	}
	if entry, err := p.backlight(); err == nil {
		status.BrightnessAvailable = true
		status.Brightness, _ = readInt(filepath.Join(entry, "brightness"))
		status.BrightnessMax, _ = readInt(filepath.Join(entry, "max_brightness"))
	}
	if _, err := os.Stat("/dev/snd"); err == nil {
		status.AudioAvailable = true
		status.AudioDetail = "ALSA device detected"
	}
	if percent, err := p.volumePercent(ctx); err == nil {
		status.VolumeAvailable = true
		status.VolumePercent = int32(percent)
	}
	return status, nil
}

func isChargerSupply(supplyType string) bool {
	switch strings.ToLower(strings.TrimSpace(supplyType)) {
	case "mains", "usb", "usb_dcp", "usb_c", "usb_pd", "usb_pd_drp", "wireless":
		return true
	default:
		return false
	}
}

func (p *Linux) SetVolume(ctx context.Context, percent int, confirmed bool) error {
	if !confirmed {
		return connect.NewError(connect.CodeFailedPrecondition, "volume changes require explicit confirmation")
	}
	if percent < 0 || percent > 100 {
		return connect.NewError(connect.CodeInvalidArgument, "volume percent must be between 0 and 100")
	}
	control, err := p.masterControl(ctx)
	if err != nil {
		return connect.NewError(connect.CodeFailedPrecondition, err.Error())
	}
	amixer, err := p.LookPath("amixer")
	if err != nil {
		return connect.NewError(connect.CodeFailedPrecondition, "volume control requires amixer")
	}
	if output, err := p.Run(ctx, amixer, "sset", control, fmt.Sprintf("%d%%", percent)); err != nil {
		return connect.NewError(connect.CodePermissionDenied, fmt.Sprintf("amixer failed: %s: %v", strings.TrimSpace(string(output)), err))
	}
	return nil
}

func (p *Linux) volumePercent(ctx context.Context) (int, error) {
	control, err := p.masterControl(ctx)
	if err != nil {
		return 0, err
	}
	amixer, err := p.LookPath("amixer")
	if err != nil {
		return 0, err
	}
	output, err := p.Run(ctx, amixer, "sget", control)
	if err != nil {
		return 0, err
	}
	for _, token := range strings.Fields(string(output)) {
		if strings.HasPrefix(token, "[") && strings.HasSuffix(token, "%]") {
			value, parseErr := strconv.Atoi(strings.TrimSuffix(strings.TrimPrefix(token, "["), "%]"))
			if parseErr == nil {
				return value, nil
			}
		}
	}
	return 0, errors.New("amixer did not report a volume percentage")
}

func (p *Linux) masterControl(ctx context.Context) (string, error) {
	if p.LookPath == nil {
		return "", errors.New("cannot locate amixer")
	}
	amixer, err := p.LookPath("amixer")
	if err != nil {
		return "", errors.New("volume control requires amixer")
	}
	output, err := p.Run(ctx, amixer, "scontrols")
	if err != nil {
		return "", fmt.Errorf("list ALSA controls: %w", err)
	}
	for _, name := range []string{"Master", "PCM", "Speaker", "Headphone"} {
		if strings.Contains(string(output), "'"+name+"'") {
			return name, nil
		}
	}
	return "", errors.New("no supported ALSA mixer control found (Master, PCM, Speaker, Headphone)")
}

func (p *Linux) SetBrightness(percent int, confirmed bool) (int64, error) {
	if !confirmed {
		return 0, connect.NewError(connect.CodeFailedPrecondition, "brightness changes require explicit confirmation")
	}
	if percent < 0 || percent > 100 {
		return 0, connect.NewError(connect.CodeInvalidArgument, "brightness percent must be between 0 and 100")
	}
	entry, err := p.backlight()
	if err != nil {
		return 0, connect.NewError(connect.CodeFailedPrecondition, err.Error())
	}
	maximum, err := readInt(filepath.Join(entry, "max_brightness"))
	if err != nil || maximum <= 0 {
		return 0, connect.NewError(connect.CodeUnavailable, "backlight maximum is unavailable")
	}
	value := maximum * int64(percent) / 100
	if err := os.WriteFile(filepath.Join(entry, "brightness"), []byte(strconv.FormatInt(value, 10)), 0); err != nil {
		return 0, connect.NewError(connect.CodePermissionDenied, fmt.Sprintf("write backlight brightness: %v", err))
	}
	return value, nil
}

func (p *Linux) PowerAction(ctx context.Context, action nexusv1.PowerActionKind, confirmed bool) error {
	if !confirmed {
		return connect.NewError(connect.CodeFailedPrecondition, "power actions require explicit confirmation")
	}
	command := ""
	switch action {
	case nexusv1.PowerActionKind_POWER_ACTION_KIND_REBOOT:
		command = "reboot"
	case nexusv1.PowerActionKind_POWER_ACTION_KIND_POWEROFF:
		command = "poweroff"
	default:
		return connect.NewError(connect.CodeInvalidArgument, "power action must be reboot or poweroff")
	}
	if p.LookPath == nil {
		return connect.NewError(connect.CodeFailedPrecondition, "cannot locate busctl")
	}
	busctl, err := p.LookPath("busctl")
	if err != nil {
		return connect.NewError(connect.CodeFailedPrecondition, "power actions require busctl and systemd-logind")
	}
	method := "Reboot"
	if command == "poweroff" {
		method = "PowerOff"
	}
	args := []string{
		"--system", "call",
		"org.freedesktop.login1",
		"/org/freedesktop/login1",
		"org.freedesktop.login1.Manager",
		method,
		"b", "false",
	}
	if output, err := p.Run(ctx, busctl, args...); err != nil {
		detail := strings.TrimSpace(string(output))
		if detail == "" {
			detail = err.Error()
		}
		return connect.NewError(connect.CodePermissionDenied, fmt.Sprintf("systemd-logind %s failed: %s", command, detail))
	}
	return nil
}

func (p *Linux) interfaces() []string {
	entries, _ := os.ReadDir(filepath.Join(p.SysRoot, "class/net"))
	names := make([]string, 0, len(entries))
	for _, entry := range entries {
		names = append(names, entry.Name())
	}
	return names
}

func (p *Linux) wifiInterface(requested string) (string, error) {
	if requested != "" {
		if !strings.HasPrefix(requested, "wl") || len(requested) > 15 || strings.ContainsAny(requested, "/\\") {
			return "", fmt.Errorf("%q is not a wireless interface name", requested)
		}
		if _, err := os.Stat(filepath.Join(p.SysRoot, "class/net", requested, "wireless")); err == nil {
			return requested, nil
		}
		if _, err := os.Stat(filepath.Join(p.SysRoot, "class/net", requested, "phy80211")); err == nil {
			return requested, nil
		}
		return "", fmt.Errorf("wireless interface %q not found", requested)
	}
	for _, name := range p.interfaces() {
		if _, err := os.Stat(filepath.Join(p.SysRoot, "class/net", name, "wireless")); err == nil {
			return name, nil
		}
		if _, err := os.Stat(filepath.Join(p.SysRoot, "class/net", name, "phy80211")); err == nil {
			return name, nil
		}
		if strings.HasPrefix(name, "wl") {
			return name, nil
		}
	}
	return "", errors.New("no wireless interface detected")
}

func (p *Linux) backlight() (string, error) {
	entries, _ := filepath.Glob(filepath.Join(p.SysRoot, "class/backlight/*"))
	for _, entry := range entries {
		if _, err := os.Stat(filepath.Join(entry, "brightness")); err == nil {
			return entry, nil
		}
	}
	return "", errors.New("no backlight sysfs entry detected")
}

func (p *Linux) isR36S() bool {
	model, err := os.ReadFile(filepath.Join(p.SysRoot, "firmware/devicetree/base/model"))
	if err != nil {
		return false
	}
	return strings.Contains(strings.ToLower(strings.TrimRight(string(model), "\x00\n")), "r36s")
}

func readInt(path string) (int64, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		return 0, err
	}
	return strconv.ParseInt(strings.TrimSpace(string(data)), 10, 64)
}

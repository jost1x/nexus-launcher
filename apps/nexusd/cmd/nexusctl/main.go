package main

import (
	"context"
	"encoding/json"
	"flag"
	"fmt"
	"net"
	"net/http"
	"os"
	"sort"
	"strconv"
	"strings"
	"text/tabwriter"
	"time"

	"connectrpc.com/connect/v2"
	"connectrpc.com/connect/v2/connecthttp"
	"connectrpc.com/grpchealth/v2"
	nexusv1 "nexusd/gen/nexus/v1"
	nexusv1connect "nexusd/gen/nexus/v1/nexusv1connect"
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
	timeout := flags.Duration("timeout", 15*time.Second, "RPC timeout")
	outputFormat := flags.String("format", "table", "output format: table or json")
	confirmed := flags.Bool("confirm", false, "confirm a hardware or power operation")
	if err := flags.Parse(os.Args[1:]); err != nil {
		return err
	}
	if *timeout <= 0 {
		return fmt.Errorf("timeout must be greater than zero")
	}
	if *outputFormat != "json" && *outputFormat != "table" {
		return fmt.Errorf("unsupported output format %q (choose table or json)", *outputFormat)
	}
	args := flags.Args()
	if len(args) == 0 {
		return usageError()
	}

	protocols := new(http.Protocols)
	protocols.SetUnencryptedHTTP2(true)
	transport := &http.Transport{Protocols: protocols, DialContext: func(ctx context.Context, _, _ string) (net.Conn, error) {
		return (&net.Dialer{}).DialContext(ctx, "unix", *socket)
	}}
	defer transport.CloseIdleConnections()
	httpClient := &http.Client{Transport: transport}
	connectTransport := connecthttp.NewTransport(httpClient, "http://nexusd", connecthttp.WithGRPC())
	connectClient := connect.NewClient(connectTransport)
	ctx, cancel := context.WithTimeout(context.Background(), *timeout)
	defer cancel()

	switch args[0] {
	case "health":
		if len(args) > 2 {
			return usageError()
		}
		service := ""
		if len(args) == 2 {
			service = args[1]
		}
		client := grpchealth.NewClient(connectClient)
		response, err := client.Check(ctx, &grpchealth.CheckRequest{Service: service})
		if err != nil {
			return fmt.Errorf("health check failed: %w", err)
		}
		if err := printOutput(*outputFormat, response.Status); err != nil {
			return err
		}
		if response.Status != grpchealth.StatusServing {
			return fmt.Errorf("service is %s", response.Status)
		}
		return nil
	case "system":
		if len(args) != 2 {
			return usageError()
		}
		client := nexusv1connect.NewSystemServiceClient(connectClient)
		switch args[1] {
		case "info":
			result, err := client.GetInfo(ctx, &nexusv1.GetInfoRequest{})
			if err != nil {
				return rpcError("system info", err)
			}
			return printOutput(*outputFormat, result)
		case "capabilities":
			result, err := client.ListCapabilities(ctx, &nexusv1.ListCapabilitiesRequest{})
			if err != nil {
				return rpcError("list capabilities", err)
			}
			return printOutput(*outputFormat, result)
		default:
			return usageError()
		}
	case "wifi":
		if len(args) < 2 || len(args) > 4 {
			return usageError()
		}
		client := nexusv1connect.NewWifiServiceClient(connectClient)
		switch args[1] {
		case "status":
			if len(args) != 2 {
				return usageError()
			}
			result, err := client.GetStatus(ctx, &nexusv1.WifiServiceGetStatusRequest{})
			if err != nil {
				return rpcError("Wi-Fi status", err)
			}
			return printOutput(*outputFormat, result)
		case "scan":
			iface := ""
			refresh := false
			for _, arg := range args[2:] {
				switch arg {
				case "--refresh":
					if refresh {
						return usageError()
					}
					refresh = true
				default:
					if strings.HasPrefix(arg, "-") || iface != "" {
						return usageError()
					}
					iface = arg
				}
			}
			result, err := client.Scan(ctx, &nexusv1.ScanRequest{Interface: iface, Refresh: refresh})
			if err != nil {
				return rpcError("Wi-Fi scan", err)
			}
			sort.Slice(result.Networks, func(i, j int) bool {
				left, right := result.Networks[i], result.Networks[j]
				if left.SignalPercent != right.SignalPercent {
					return left.SignalPercent > right.SignalPercent
				}
				return left.Ssid < right.Ssid
			})
			return printOutput(*outputFormat, result)
		default:
			return usageError()
		}
	case "hardware":
		if len(args) < 2 {
			return usageError()
		}
		client := nexusv1connect.NewHardwareServiceClient(connectClient)
		switch args[1] {
		case "status":
			if len(args) != 2 {
				return usageError()
			}
			result, err := client.GetStatus(ctx, &nexusv1.HardwareServiceGetStatusRequest{})
			if err != nil {
				return rpcError("hardware status", err)
			}
			return printOutput(*outputFormat, result)
		case "brightness":
			if len(args) != 4 || args[2] != "set" {
				return usageError()
			}
			parsed, err := strconv.ParseInt(args[3], 10, 32)
			if err != nil {
				return fmt.Errorf("invalid brightness percentage %q", args[3])
			}
			percent := int32(parsed)
			result, err := client.SetBrightness(ctx, &nexusv1.SetBrightnessRequest{Percent: percent, Confirmed: *confirmed})
			if err != nil {
				return rpcError("set brightness", err)
			}
			return printOutput(*outputFormat, result)
		case "volume":
			if len(args) != 4 || args[2] != "set" {
				return usageError()
			}
			parsed, err := strconv.ParseInt(args[3], 10, 32)
			if err != nil {
				return fmt.Errorf("invalid volume percentage %q", args[3])
			}
			result, err := client.SetVolume(ctx, &nexusv1.SetVolumeRequest{Percent: int32(parsed), Confirmed: *confirmed})
			if err != nil {
				return rpcError("set volume", err)
			}
			return printOutput(*outputFormat, result)
		case "reboot", "poweroff":
			if len(args) != 2 {
				return usageError()
			}
			action := nexusv1.PowerActionKind_POWER_ACTION_KIND_REBOOT
			if args[1] == "poweroff" {
				action = nexusv1.PowerActionKind_POWER_ACTION_KIND_POWEROFF
			}
			result, err := client.PowerAction(ctx, &nexusv1.PowerActionRequest{Action: action, Confirmed: *confirmed})
			if err != nil {
				return rpcError(args[1], err)
			}
			return printOutput(*outputFormat, result)
		default:
			return usageError()
		}
	default:
		return usageError()
	}
}

func printOutput(format string, value any) error {
	if format == "json" {
		if result, ok := value.(*nexusv1.ScanResponse); ok {
			return json.NewEncoder(os.Stdout).Encode(struct {
				Networks      []*nexusv1.WifiNetwork `json:"networks"`
				ScannedAtUnix int64                  `json:"scanned_at_unix"`
				FromCache     bool                   `json:"from_cache"`
			}{
				Networks:      result.Networks,
				ScannedAtUnix: result.ScannedAtUnix,
				FromCache:     result.FromCache,
			})
		}
		encoder := json.NewEncoder(os.Stdout)
		encoder.SetIndent("", "  ")
		return encoder.Encode(value)
	}

	writer := tabwriter.NewWriter(os.Stdout, 0, 4, 2, ' ', 0)
	writeKeyValues := func(rows ...[2]string) error {
		if _, err := fmt.Fprintln(writer, "FIELD\tVALUE"); err != nil {
			return err
		}
		for _, row := range rows {
			if _, err := fmt.Fprintf(writer, "%s\t%s\n", row[0], cleanCell(row[1])); err != nil {
				return err
			}
		}
		return writer.Flush()
	}

	switch result := value.(type) {
	case string:
		if _, err := fmt.Fprintln(writer, "STATUS"); err != nil {
			return err
		}
		if _, err := fmt.Fprintln(writer, cleanCell(result)); err != nil {
			return err
		}
		return writer.Flush()
	case grpchealth.Status:
		if _, err := fmt.Fprintln(writer, "STATUS"); err != nil {
			return err
		}
		if _, err := fmt.Fprintln(writer, result); err != nil {
			return err
		}
		return writer.Flush()
	case *nexusv1.GetInfoResponse:
		return writeKeyValues(
			[2]string{"OS", result.OsId},
			[2]string{"OS VERSION", result.OsVersion},
			[2]string{"KERNEL", result.Kernel},
			[2]string{"ARCHITECTURE", result.Architecture},
			[2]string{"HOSTNAME", result.Hostname},
			[2]string{"MEMORY TOTAL BYTES", strconv.FormatUint(result.MemoryTotalBytes, 10)},
			[2]string{"MEMORY AVAILABLE BYTES", strconv.FormatUint(result.MemoryAvailableBytes, 10)},
			[2]string{"UPTIME SECONDS", strconv.FormatUint(result.UptimeSeconds, 10)},
			[2]string{"LOAD AVERAGE", result.LoadAverage},
		)
	case *nexusv1.ListCapabilitiesResponse:
		if _, err := fmt.Fprintln(writer, "NAME\tAVAILABLE\tVERIFIED ON R36S\tDETAIL"); err != nil {
			return err
		}
		for _, item := range result.Items {
			if _, err := fmt.Fprintf(writer, "%s\t%t\t%t\t%s\n", cleanCell(item.Name), item.Available, item.VerifiedOnR36S, cleanCell(item.Detail)); err != nil {
				return err
			}
		}
		return writer.Flush()
	case *nexusv1.WifiServiceGetStatusResponse:
		return writeKeyValues(
			[2]string{"INTERFACE", result.Interface},
			[2]string{"STATE", result.State},
			[2]string{"CONNECTED", strconv.FormatBool(result.Connected)},
			[2]string{"SSID", result.Ssid},
			[2]string{"BSSID", result.Bssid},
			[2]string{"MANAGER", result.Manager},
			[2]string{"DETAIL", result.Detail},
		)
	case *nexusv1.ScanResponse:
		scanAge := time.Since(time.Unix(result.ScannedAtUnix, 0))
		if scanAge < 0 {
			scanAge = 0
		}
		if _, err := fmt.Fprintf(writer, "SCANNED AT\t%s\nFROM CACHE\t%t\nAGE\t%s\n\n",
			time.Unix(result.ScannedAtUnix, 0).Format(time.RFC3339), result.FromCache, scanAge.Round(time.Second)); err != nil {
			return err
		}
		if len(result.Networks) == 0 {
			_, err := fmt.Fprintln(writer, "No networks found.")
			return err
		}
		if _, err := fmt.Fprintln(writer, "SIGNAL\tCURRENT\tSSID\tBSSID\tSECURITY"); err != nil {
			return err
		}
		for _, network := range result.Networks {
			signal := "-"
			if network.SignalPercent > 0 {
				signal = fmt.Sprintf("%d%%", network.SignalPercent)
			}
			current := "-"
			if network.Connected {
				current = "yes"
			}
			if _, err := fmt.Fprintf(writer, "%s\t%s\t%s\t%s\t%s\n", signal, current, cleanCell(network.Ssid), cleanCell(network.Bssid), cleanCell(network.Security)); err != nil {
				return err
			}
		}
		return writer.Flush()
	case *nexusv1.HardwareServiceGetStatusResponse:
		return writeKeyValues(
			[2]string{"BATTERY AVAILABLE", strconv.FormatBool(result.BatteryAvailable)},
			[2]string{"BATTERY STATUS", result.BatteryStatus},
			[2]string{"BATTERY PERCENT", fmt.Sprintf("%d%%", result.BatteryPercent)},
			[2]string{"BATTERY VOLTAGE MICROVOLTS", strconv.FormatInt(result.BatteryVoltageMicrovolts, 10)},
			[2]string{"THERMAL AVAILABLE", strconv.FormatBool(result.ThermalAvailable)},
			[2]string{"SOC TEMPERATURE CELSIUS", fmt.Sprintf("%.1f", result.SocTemperatureCelsius)},
			[2]string{"BRIGHTNESS AVAILABLE", strconv.FormatBool(result.BrightnessAvailable)},
			[2]string{"BRIGHTNESS", fmt.Sprintf("%d/%d", result.Brightness, result.BrightnessMax)},
			[2]string{"AUDIO AVAILABLE", strconv.FormatBool(result.AudioAvailable)},
			[2]string{"AUDIO DETAIL", result.AudioDetail},
			[2]string{"VOLUME AVAILABLE", strconv.FormatBool(result.VolumeAvailable)},
			[2]string{"VOLUME PERCENT", fmt.Sprintf("%d%%", result.VolumePercent)},
		)
	case *nexusv1.SetVolumeResponse:
		return writeKeyValues([2]string{"VOLUME", fmt.Sprintf("%d%%", result.Percent)})
	case *nexusv1.SetBrightnessResponse:
		return writeKeyValues([2]string{"BRIGHTNESS VALUE", strconv.FormatInt(result.Value, 10)})
	case *nexusv1.PowerActionResponse:
		return writeKeyValues([2]string{"RESULT", result.Message})
	default:
		return fmt.Errorf("table output is not supported for %T", value)
	}
}

func cleanCell(value string) string {
	return strings.NewReplacer("\t", " ", "\n", " ", "\r", " ").Replace(value)
}

func rpcError(operation string, err error) error {
	if rpcErr, ok := err.(*connect.Error); ok {
		return fmt.Errorf("%s: %s (%s)", operation, rpcErr.Message(), rpcErr.Code())
	}
	return fmt.Errorf("%s: %w", operation, err)
}

func usageError() error {
	return fmt.Errorf("usage: nexusctl [--socket PATH] [--timeout DURATION] [--format json|table] [--confirm] COMMAND\n  health [SERVICE]\n  system info|capabilities\n  wifi status|scan [INTERFACE] [--refresh]\n  hardware status|brightness set PERCENT|volume set PERCENT|reboot|poweroff\nWi-Fi scan uses cached results for 15 seconds unless --refresh is given; results sort by signal strength (descending), then SSID.\n--confirm is required for brightness, volume, reboot, and poweroff operations")
}

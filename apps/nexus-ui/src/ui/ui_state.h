#pragma once

#include <optional>
#include <string>

#include "nexus/v1/nexus.pb.h"

namespace nexus {

enum class View {
    Home,
    Games,
    Apps,
    Favorites,
    Settings,
    Wifi,
    WifiAvailable,
    WifiSaved,
    Display,
    Audio,
    System,
    About,
};
enum class Action {
    Home = 1,
    Wifi,
    Display,
    Audio,
    System,
    About,
    RefreshWifi,
    ApplyBrightness,
    ApplyVolume,
    Games,
    Apps,
    Favorites,
    Settings,
    ToggleWifi,
    ConnectWifiNetwork,
    ConnectSavedWifiNetwork,
    SetPreferredWifiNetwork,
    ForgetWifiNetwork,
    SubmitWifiPassword,
    CancelWifiPassword,
    RefreshSavedWifiNetworks,
    ShowWifiAvailable,
    ShowWifiSaved,
};

struct WifiActionContext {
    Action action = Action::ConnectWifiNetwork;
    std::string id;
    std::string ssid;
    std::string security;
};

struct UiState {
    std::optional<nexus::v1::WifiServiceGetStatusResponse> wifi_status;
    std::optional<nexus::v1::ScanResponse> wifi_scan;
    std::optional<nexus::v1::WifiServiceListSavedNetworksResponse> saved_wifi_networks;
    std::optional<nexus::v1::HardwareServiceGetStatusResponse> hardware;
    std::optional<nexus::v1::GetInfoResponse> system_info;
    bool wifi_status_loading = false;
    bool wifi_scan_loading = false;
    bool wifi_saved_loading = false;
    bool wifi_radio_changing = false;
    bool hardware_loading = false;
    bool system_loading = false;
    std::string wifi_status_error;
    std::string wifi_scan_error;
    std::string wifi_saved_error;
    std::string wifi_action_message;
    std::string wifi_connect_ssid;
    std::string wifi_connect_security;
    std::string wifi_password;
    std::string hardware_error;
    std::string system_error;
    std::string action_message;
    int brightness_target = -1;
    int volume_target = -1;
};

}  // namespace nexus

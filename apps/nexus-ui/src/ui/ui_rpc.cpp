#include "ui/ui.h"

#include "rpc/rpc_client.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace nexus {
namespace {

int brightness_percent(const nexus::v1::HardwareServiceGetStatusResponse& status) {
    return static_cast<int>(status.brightness() * 100 / std::max<int64_t>(1, status.brightness_max()));
}

nexus::v1::WifiServiceGetStatusResponse wifi_status_from_watch(const nexus::v1::WifiServiceWatchStatusResponse& status) {
    nexus::v1::WifiServiceGetStatusResponse result;
    result.set_interface(status.interface());
    result.set_state(status.state());
    result.set_connected(status.connected());
    result.set_ssid(status.ssid());
    result.set_manager(status.manager());
    result.set_detail(status.detail());
    result.set_bssid(status.bssid());
    result.set_radio_enabled(status.radio_enabled());
    result.set_radio_state_available(status.radio_state_available());
    return result;
}

nexus::v1::HardwareServiceGetStatusResponse hardware_status_from_watch(
    const nexus::v1::HardwareServiceWatchStatusResponse& status) {
    nexus::v1::HardwareServiceGetStatusResponse result;
    result.set_battery_available(status.battery_available());
    result.set_battery_status(status.battery_status());
    result.set_battery_percent(status.battery_percent());
    result.set_battery_voltage_microvolts(status.battery_voltage_microvolts());
    result.set_thermal_available(status.thermal_available());
    result.set_soc_temperature_celsius(status.soc_temperature_celsius());
    result.set_brightness_available(status.brightness_available());
    result.set_brightness(status.brightness());
    result.set_brightness_max(status.brightness_max());
    result.set_audio_available(status.audio_available());
    result.set_audio_detail(status.audio_detail());
    result.set_volume_available(status.volume_available());
    result.set_volume_percent(status.volume_percent());
    result.set_charger_available(status.charger_available());
    result.set_charger_connected(status.charger_connected());
    result.set_battery_charging(status.battery_charging());
    return result;
}

bool same_hardware_view(View view,
                        const UiState& state,
                        const nexus::v1::HardwareServiceGetStatusResponse& previous,
                        const nexus::v1::HardwareServiceGetStatusResponse& current) {
    switch (view) {
        case View::Home:
            return previous.battery_available() == current.battery_available() &&
                   previous.battery_percent() == current.battery_percent() &&
                   previous.battery_status() == current.battery_status() &&
                   previous.battery_charging() == current.battery_charging() &&
                   previous.brightness_available() == current.brightness_available() &&
                   (!current.brightness_available() || brightness_percent(previous) == brightness_percent(current)) &&
                   previous.volume_available() == current.volume_available() &&
                   (!current.volume_available() || previous.volume_percent() == current.volume_percent());
        case View::Display:
            return previous.brightness_available() == current.brightness_available() &&
                   (state.brightness_target >= 0 || !current.brightness_available() ||
                    brightness_percent(previous) == brightness_percent(current));
        case View::Audio:
            return previous.volume_available() == current.volume_available() &&
                   (state.volume_target >= 0 || !current.volume_available() ||
                    previous.volume_percent() == current.volume_percent());
        case View::System:
            return previous.battery_available() == current.battery_available() &&
                   previous.battery_percent() == current.battery_percent() &&
                   previous.battery_status() == current.battery_status() &&
                   previous.charger_available() == current.charger_available() &&
                   previous.charger_connected() == current.charger_connected() &&
                   previous.thermal_available() == current.thermal_available() &&
                   (!current.thermal_available() ||
                    std::lround(previous.soc_temperature_celsius()) == std::lround(current.soc_temperature_celsius()));
        case View::Wifi:
        case View::WifiAvailable:
        case View::WifiSaved:
        case View::Games:
        case View::Apps:
        case View::Favorites:
        case View::Settings:
        case View::About:
            return true;
    }
    return true;
}

}  // namespace

void UiController::request_overview() {
    wifi_watch_id_ = rpc_.watch_wifi_status();
    state_.wifi_status_loading = wifi_watch_id_ != 0;
    hardware_watch_id_ = rpc_.watch_hardware_status();
    state_.hardware_loading = hardware_watch_id_ != 0;
    system_id_ = rpc_.request_system_info();
    state_.system_loading = system_id_ != 0;
}

void UiController::collect_results() {
    bool changed = false;
    for (auto& result : rpc_.take_results()) {
        if (stopped_) return;
        if (!result.status.ok()) {
            const std::string code = std::to_string(static_cast<int>(result.status.error_code()));
            std::string message = "Error de servicio (código " + code + ").";
            switch (result.operation) {
                case RpcOperation::WifiStatus:
                    if (result.id != wifi_status_id_) continue;
                    message = "No se pudo consultar el estado Wi-Fi (código " + code + ").";
                    wifi_status_id_ = 0;
                    state_.wifi_status_loading = false;
                    state_.wifi_status_error = message;
                    break;
                case RpcOperation::WifiWatch:
                    if (result.id != wifi_watch_id_) continue;
                    message = "Se perdió la suscripción de Wi-Fi (código " + code + "); reconectando.";
                    state_.wifi_status_loading = false;
                    state_.wifi_status_error = message;
                    break;
                case RpcOperation::WifiScan:
                    if (result.id != wifi_scan_id_) continue;
                    message = "No se pudo buscar redes Wi-Fi (código " + code + ").";
                    wifi_scan_id_ = 0;
                    state_.wifi_scan_loading = false;
                    state_.wifi_scan_error = message;
                    break;
                case RpcOperation::WifiSavedNetworks:
                    if (result.id != wifi_saved_id_) continue;
                    wifi_saved_id_ = 0;
                    state_.wifi_saved_loading = false;
                    state_.wifi_saved_error = "No se pudieron cargar las redes guardadas.";
                    break;
                case RpcOperation::WifiSetEnabled:
                    if (result.id != wifi_set_enabled_id_) continue;
                    wifi_set_enabled_id_ = 0;
                    state_.wifi_radio_changing = false;
                    state_.wifi_action_message = "No se pudo cambiar el estado del Wi-Fi.";
                    break;
                case RpcOperation::WifiConnect:
                    if (result.id != wifi_connect_id_) continue;
                    wifi_connect_id_ = 0;
                    state_.wifi_action_message = "No se pudo conectar. Revisa la contraseña y el control de red.";
                    break;
                case RpcOperation::WifiConnectSaved:
                    if (result.id != wifi_connect_saved_id_) continue;
                    wifi_connect_saved_id_ = 0;
                    state_.wifi_action_message = "No se pudo conectar a esa red guardada.";
                    break;
                case RpcOperation::WifiSetPreferred:
                    if (result.id != wifi_preferred_id_) continue;
                    wifi_preferred_id_ = 0;
                    state_.wifi_action_message = "No se pudo cambiar la red preferida.";
                    break;
                case RpcOperation::WifiForget:
                    if (result.id != wifi_forget_id_) continue;
                    wifi_forget_id_ = 0;
                    state_.wifi_action_message = "No se pudo olvidar esa red.";
                    break;
                case RpcOperation::HardwareStatus:
                    if (result.id != hardware_id_) continue;
                    message = "No se pudo consultar el hardware (código " + code + ").";
                    hardware_id_ = 0;
                    state_.hardware_loading = false;
                    state_.hardware_error = message;
                    break;
                case RpcOperation::HardwareWatch:
                    if (result.id != hardware_watch_id_) continue;
                    message = "Se perdió la suscripción de hardware (código " + code + "); reconectando.";
                    state_.hardware_loading = false;
                    state_.hardware_error = message;
                    break;
                case RpcOperation::SystemInfo:
                    if (result.id != system_id_) continue;
                    message = "No se pudo consultar el sistema (código " + code + ").";
                    system_id_ = 0;
                    state_.system_loading = false;
                    state_.system_error = message;
                    break;
                case RpcOperation::SetBrightness:
                    if (result.id != brightness_id_) continue;
                    message = "No se pudo aplicar el brillo (código " + code + ").";
                    brightness_id_ = 0;
                    state_.action_message.clear();
                    state_.hardware_error = message;
                    state_.brightness_target = -1;
                    break;
                case RpcOperation::SetVolume:
                    if (result.id != volume_id_) continue;
                    message = "No se pudo aplicar el volumen (código " + code + ").";
                    volume_id_ = 0;
                    state_.action_message.clear();
                    state_.hardware_error = message;
                    state_.volume_target = -1;
                    break;
            }
            changed = true;
            continue;
        }

        bool update_view = true;
        switch (result.operation) {
            case RpcOperation::WifiStatus:
                if (result.id != wifi_status_id_) continue;
                state_.wifi_status = std::get<nexus::v1::WifiServiceGetStatusResponse>(std::move(result.payload));
                state_.wifi_status_error.clear();
                state_.wifi_status_loading = false;
                wifi_status_id_ = 0;
                break;
            case RpcOperation::WifiWatch:
                if (result.id != wifi_watch_id_) continue;
                state_.wifi_status = wifi_status_from_watch(
                    std::get<nexus::v1::WifiServiceWatchStatusResponse>(result.payload));
                state_.wifi_status_error.clear();
                state_.wifi_status_loading = false;
                break;
            case RpcOperation::WifiScan: {
                if (result.id != wifi_scan_id_) continue;
                auto response = std::get<nexus::v1::ScanResponse>(std::move(result.payload));
                std::vector<nexus::v1::WifiNetwork> networks(response.networks().begin(), response.networks().end());
                std::sort(networks.begin(), networks.end(), [](const auto& left, const auto& right) {
                    if (left.signal_percent() != right.signal_percent()) return left.signal_percent() > right.signal_percent();
                    return left.ssid() < right.ssid();
                });
                response.clear_networks();
                for (const auto& network : networks) *response.add_networks() = network;
                state_.wifi_scan = std::move(response);
                state_.wifi_scan_error.clear();
                state_.wifi_scan_loading = false;
                wifi_scan_id_ = 0;
                break;
            }
            case RpcOperation::WifiSavedNetworks:
                if (result.id != wifi_saved_id_) continue;
                state_.saved_wifi_networks = std::get<nexus::v1::WifiServiceListSavedNetworksResponse>(std::move(result.payload));
                state_.wifi_saved_error.clear();
                state_.wifi_saved_loading = false;
                wifi_saved_id_ = 0;
                break;
            case RpcOperation::WifiSetEnabled:
                if (result.id != wifi_set_enabled_id_) continue;
                {
                    const auto response = std::get<nexus::v1::WifiServiceSetEnabledResponse>(std::move(result.payload));
                    if (!state_.wifi_status) state_.wifi_status.emplace();
                    state_.wifi_status->set_radio_enabled(response.enabled());
                    state_.wifi_status->set_radio_state_available(response.radio_state_available());
                    state_.wifi_status->set_state(response.enabled() ? "up" : "down");
                    if (!response.enabled()) {
                        state_.wifi_status->set_connected(false);
                        state_.wifi_status->clear_ssid();
                    }
                }
                state_.wifi_action_message = state_.wifi_status->radio_enabled() ? "Wi-Fi activado." : "Wi-Fi desactivado.";
                wifi_set_enabled_id_ = 0;
                if (state_.wifi_status->radio_enabled() && wifi_scan_id_ == 0) {
                    state_.wifi_scan.reset();
                    state_.wifi_scan_error.clear();
                    state_.wifi_scan_loading = true;
                    wifi_scan_id_ = rpc_.scan_wifi(true);
                }
                state_.wifi_radio_changing = false;
                break;
            case RpcOperation::WifiConnect:
                if (result.id != wifi_connect_id_) continue;
                {
                    const auto response = std::get<nexus::v1::WifiServiceConnectResponse>(std::move(result.payload));
                    if (!state_.wifi_status) state_.wifi_status.emplace();
                    state_.wifi_status->set_state(response.state());
                    state_.wifi_status->set_connected(response.connected());
                    state_.wifi_status->set_ssid(response.ssid());
                }
                state_.wifi_action_message = "Red guardada como preferida. Conectando...";
                wifi_connect_id_ = 0;
                state_.saved_wifi_networks.reset();
                state_.wifi_saved_loading = true;
                wifi_saved_id_ = rpc_.list_saved_wifi_networks();
                if (wifi_scan_id_ == 0) {
                    state_.wifi_scan.reset();
                    state_.wifi_scan_error.clear();
                    state_.wifi_scan_loading = true;
                    wifi_scan_id_ = rpc_.scan_wifi(true);
                }
                break;
            case RpcOperation::WifiConnectSaved:
                if (result.id != wifi_connect_saved_id_) continue;
                {
                    const auto response = std::get<nexus::v1::WifiServiceConnectSavedNetworkResponse>(std::move(result.payload));
                    if (!state_.wifi_status) state_.wifi_status.emplace();
                    state_.wifi_status->set_state(response.state());
                    state_.wifi_status->set_connected(response.connected());
                    state_.wifi_status->set_ssid(response.ssid());
                }
                state_.wifi_action_message = "Conectando a la red preferida...";
                wifi_connect_saved_id_ = 0;
                state_.saved_wifi_networks.reset();
                state_.wifi_saved_loading = true;
                wifi_saved_id_ = rpc_.list_saved_wifi_networks();
                if (wifi_scan_id_ == 0) {
                    state_.wifi_scan.reset();
                    state_.wifi_scan_error.clear();
                    state_.wifi_scan_loading = true;
                    wifi_scan_id_ = rpc_.scan_wifi(true);
                }
                break;
            case RpcOperation::WifiSetPreferred: {
                if (result.id != wifi_preferred_id_) continue;
                auto response = std::get<nexus::v1::WifiServiceSetPreferredNetworkResponse>(std::move(result.payload));
                nexus::v1::WifiServiceListSavedNetworksResponse saved;
                for (const auto& network : response.networks()) *saved.add_networks() = network;
                state_.saved_wifi_networks = std::move(saved);
                state_.wifi_action_message = "Red preferida actualizada.";
                wifi_preferred_id_ = 0;
                break;
            }
            case RpcOperation::WifiForget: {
                if (result.id != wifi_forget_id_) continue;
                auto response = std::get<nexus::v1::WifiServiceForgetNetworkResponse>(std::move(result.payload));
                nexus::v1::WifiServiceListSavedNetworksResponse saved;
                for (const auto& network : response.networks()) *saved.add_networks() = network;
                state_.saved_wifi_networks = std::move(saved);
                state_.wifi_action_message = "Red olvidada.";
                wifi_forget_id_ = 0;
                break;
            }
            case RpcOperation::HardwareStatus:
                if (result.id != hardware_id_) continue;
                [[fallthrough]];
            case RpcOperation::HardwareWatch:
                if (result.operation == RpcOperation::HardwareWatch && result.id != hardware_watch_id_) continue;
                {
                    auto response = result.operation == RpcOperation::HardwareWatch
                                        ? hardware_status_from_watch(
                                              std::get<nexus::v1::HardwareServiceWatchStatusResponse>(result.payload))
                                        : std::get<nexus::v1::HardwareServiceGetStatusResponse>(
                                              std::move(result.payload));
                    update_view = !state_.hardware ||
                                  !same_hardware_view(view_, state_, *state_.hardware, response) ||
                                  !state_.hardware_error.empty() || state_.hardware_loading;
                    state_.hardware = std::move(response);
                }
                state_.hardware_error.clear();
                state_.hardware_loading = false;
                if (result.operation == RpcOperation::HardwareStatus) hardware_id_ = 0;
                break;
            case RpcOperation::SystemInfo:
                if (result.id != system_id_) continue;
                state_.system_info = std::get<nexus::v1::GetInfoResponse>(std::move(result.payload));
                state_.system_error.clear();
                state_.system_loading = false;
                system_id_ = 0;
                break;
            case RpcOperation::SetBrightness:
                if (result.id != brightness_id_) continue;
                brightness_id_ = 0;
                state_.action_message = "Brillo actualizado.";
                state_.brightness_target = -1;
                hardware_id_ = rpc_.request_hardware_status();
                state_.hardware_loading = hardware_id_ != 0;
                break;
            case RpcOperation::SetVolume:
                if (result.id != volume_id_) continue;
                volume_id_ = 0;
                state_.action_message = "Volumen actualizado.";
                state_.volume_target = -1;
                hardware_id_ = rpc_.request_hardware_status();
                state_.hardware_loading = hardware_id_ != 0;
                break;
        }
        changed = changed || update_view;
    }
    if (changed) render();
}

}  // namespace nexus

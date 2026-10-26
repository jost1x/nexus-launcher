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
                state_.wifi_status = std::get<nexus::v1::WifiServiceGetStatusResponse>(std::move(result.payload));
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
            case RpcOperation::HardwareStatus:
                if (result.id != hardware_id_) continue;
                [[fallthrough]];
            case RpcOperation::HardwareWatch:
                if (result.operation == RpcOperation::HardwareWatch && result.id != hardware_watch_id_) continue;
                {
                    auto response = std::get<nexus::v1::HardwareServiceGetStatusResponse>(std::move(result.payload));
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

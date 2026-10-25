#include "ui.h"

#include "views.h"

#include <algorithm>
#include <ctime>
#include <utility>
#include <vector>

namespace nexus {

UiController* UiController::active_ = nullptr;

UiController::UiController(RpcClient& rpc) : rpc_(rpc), focus_group_(lv_group_create()) {
    active_ = this;
    lv_group_set_default(focus_group_);
}

UiController::~UiController() {
    stopped_ = true;
    if (refresh_timer_ != nullptr) lv_timer_delete(refresh_timer_);
    for (const auto id : {wifi_status_id_, wifi_scan_id_, hardware_id_, system_id_, brightness_id_, volume_id_}) {
        if (id != 0) rpc_.cancel(id);
    }
    if (root_ != nullptr) lv_obj_delete(root_);
    if (focus_group_ != nullptr) lv_group_delete(focus_group_);
    if (active_ == this) active_ = nullptr;
}

void UiController::start() {
    request_overview();
    refresh_timer_ = lv_timer_create(timer_event, 150, this);
    render();
}

void UiController::navigate(View view) {
    if (view_ == View::Wifi && view != View::Wifi && wifi_scan_id_ != 0) {
        rpc_.cancel(wifi_scan_id_);
        wifi_scan_id_ = 0;
        state_.wifi_scan_loading = false;
    }
    view_ = view;
    if (view == View::Wifi) {
        if (wifi_status_id_ == 0 && (!state_.wifi_status || !state_.wifi_status_error.empty())) {
            wifi_status_id_ = rpc_.request_wifi_status();
            state_.wifi_status_loading = wifi_status_id_ != 0;
            state_.wifi_status_error.clear();
        }
        if (wifi_scan_id_ == 0) {
            state_.wifi_scan_error.clear();
            state_.wifi_scan_loading = true;
            wifi_scan_id_ = rpc_.scan_wifi(false);
        }
    } else if (view == View::Display || view == View::Audio || view == View::System) {
        if (hardware_id_ == 0) {
            hardware_id_ = rpc_.request_hardware_status();
            state_.hardware_loading = hardware_id_ != 0;
            state_.hardware_error.clear();
        }
        if (view == View::System && system_id_ == 0) {
            system_id_ = rpc_.request_system_info();
            state_.system_loading = system_id_ != 0;
            state_.system_error.clear();
        }
    }
    render();
}

void UiController::dispatch(Action action) {
    switch (action) {
        case Action::Home: navigate(View::Home); return;
        case Action::Wifi: navigate(View::Wifi); return;
        case Action::Display: navigate(View::Display); return;
        case Action::Audio: navigate(View::Audio); return;
        case Action::System: navigate(View::System); return;
        case Action::About: navigate(View::About); return;
        case Action::RefreshWifi:
            if (wifi_scan_id_ != 0) rpc_.cancel(wifi_scan_id_);
            if (wifi_status_id_ == 0) {
                wifi_status_id_ = rpc_.request_wifi_status();
                state_.wifi_status_loading = wifi_status_id_ != 0;
                state_.wifi_status_error.clear();
            }
            state_.wifi_scan_error.clear();
            state_.wifi_scan_loading = true;
            wifi_scan_id_ = rpc_.scan_wifi(true);
            render();
            return;
        case Action::ApplyBrightness:
            if (brightness_id_ != 0 || !state_.hardware || !state_.hardware->brightness_available()) return;
            if (state_.brightness_target < 0) {
                state_.brightness_target = static_cast<int>(state_.hardware->brightness() * 100 /
                                                             std::max<int64_t>(1, state_.hardware->brightness_max()));
            }
            state_.action_message = "Aplicando brillo...";
            state_.hardware_error.clear();
            brightness_id_ = rpc_.set_brightness(state_.brightness_target);
            render();
            return;
        case Action::ApplyVolume:
            if (volume_id_ != 0 || !state_.hardware || !state_.hardware->volume_available()) return;
            if (state_.volume_target < 0) state_.volume_target = state_.hardware->volume_percent();
            state_.action_message = "Aplicando volumen...";
            state_.hardware_error.clear();
            volume_id_ = rpc_.set_volume(state_.volume_target);
            render();
            return;
    }
}

void UiController::render() {
    lv_group_remove_all_objs(focus_group_);
    if (root_ != nullptr) {
        lv_obj_delete(root_);
        root_ = nullptr;
    }
    const auto objects = build_view(view_, state_, focus_group_);
    root_ = objects.root;
    status_label_ = objects.status;
    last_header_text_.clear();
    lv_obj_add_event_cb(root_, root_deleted, LV_EVENT_DELETE, this);
    if (objects.initial_focus != nullptr) lv_group_focus_obj(objects.initial_focus);
    update_header();
}

void UiController::request_overview() {
    wifi_status_id_ = rpc_.request_wifi_status();
    state_.wifi_status_loading = wifi_status_id_ != 0;
    hardware_id_ = rpc_.request_hardware_status();
    state_.hardware_loading = hardware_id_ != 0;
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

        switch (result.operation) {
            case RpcOperation::WifiStatus:
                if (result.id != wifi_status_id_) continue;
                state_.wifi_status = std::get<nexus::v1::WifiServiceGetStatusResponse>(std::move(result.payload));
                state_.wifi_status_error.clear();
                state_.wifi_status_loading = false;
                wifi_status_id_ = 0;
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
                state_.hardware = std::get<nexus::v1::HardwareServiceGetStatusResponse>(std::move(result.payload));
                state_.hardware_error.clear();
                state_.hardware_loading = false;
                hardware_id_ = 0;
                if (state_.brightness_target < 0 && state_.hardware->brightness_available()) {
                    state_.brightness_target = static_cast<int>(state_.hardware->brightness() * 100 /
                                                                 std::max<int64_t>(1, state_.hardware->brightness_max()));
                }
                if (state_.volume_target < 0 && state_.hardware->volume_available()) {
                    state_.volume_target = state_.hardware->volume_percent();
                }
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
        changed = true;
    }
    if (changed) render();
}

void UiController::update_header() {
    if (status_label_ == nullptr) return;
    std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_r(&now, &local);
    char clock[8]{};
    std::strftime(clock, sizeof(clock), "%H:%M", &local);
    std::string status = clock;
    status += "  ·  ";
    if (state_.hardware && state_.hardware->battery_available()) {
        status += std::to_string(state_.hardware->battery_percent()) + " %";
        if (state_.hardware->battery_charging()) status += "  ·  Cargando";
    } else if (state_.wifi_status_error.size() || state_.hardware_error.size()) {
        status += "Daemon sin conexión";
    } else {
        status += "R36S";
    }
    if (status != last_header_text_) {
        lv_label_set_text(status_label_, status.c_str());
        last_header_text_ = std::move(status);
    }
}

void UiController::timer_event(lv_timer_t* timer) {
    auto* self = static_cast<UiController*>(lv_timer_get_user_data(timer));
    if (self == nullptr || self->stopped_) return;
    if (lv_display_get_default() == nullptr) {
        self->stopped_ = true;
        return;
    }
    self->collect_results();
    self->update_header();
}

void UiController::root_deleted(lv_event_t* event) {
    auto* self = static_cast<UiController*>(lv_event_get_user_data(event));
    if (self == nullptr) return;
    self->root_ = nullptr;
    self->status_label_ = nullptr;
    if (self->focus_group_ != nullptr) lv_group_remove_all_objs(self->focus_group_);
}

void UiController::action_event(lv_event_t* event) {
    if (active_ == nullptr) return;
    const auto action = static_cast<Action>(reinterpret_cast<std::uintptr_t>(lv_event_get_user_data(event)));
    active_->dispatch(action);
}

void UiController::key_event(lv_event_t* event) {
    if (active_ != nullptr && lv_event_get_key(event) == LV_KEY_ESC) active_->navigate(View::Home);
}

void UiController::slider_event(lv_event_t* event) {
    if (active_ == nullptr) return;
    const auto action = static_cast<Action>(reinterpret_cast<std::uintptr_t>(lv_event_get_user_data(event)));
    const auto value = static_cast<int>(lv_slider_get_value(static_cast<lv_obj_t*>(lv_event_get_target(event))));
    if (action == Action::ApplyBrightness) active_->state_.brightness_target = value;
    if (action == Action::ApplyVolume) active_->state_.volume_target = value;
}

}  // namespace nexus

#include "ui/ui.h"

#include "rpc/rpc_client.h"

#include <algorithm>

namespace nexus {

void UiController::navigate(View view) {
    navigate_to(view, true);
}

void UiController::navigate_to(View view, bool remember_current) {
    if (view == view_) return;
    if (remember_current && view == View::Home) navigation_history_.clear();
    else if (remember_current) navigation_history_.push_back(view_);
    if (view_ == View::Wifi && view != View::Wifi && wifi_scan_id_ != 0) {
        rpc_.cancel(wifi_scan_id_);
        wifi_scan_id_ = 0;
        state_.wifi_scan_loading = false;
    }
    view_ = view;
    if (view == View::Wifi) {
        if (wifi_watch_id_ == 0 && wifi_status_id_ == 0 && (!state_.wifi_status || !state_.wifi_status_error.empty())) {
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
        if (hardware_watch_id_ == 0 && hardware_id_ == 0) {
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

void UiController::navigate_back() {
    if (view_ == View::Home || navigation_history_.empty()) return;
    const View previous = navigation_history_.back();
    navigation_history_.pop_back();
    navigate_to(previous, false);
}

void UiController::dispatch(Action action) {
    switch (action) {
        case Action::Home: navigate(View::Home); return;
        case Action::Games: navigate(View::Games); return;
        case Action::Apps: navigate(View::Apps); return;
        case Action::Favorites: navigate(View::Favorites); return;
        case Action::Settings: navigate(View::Settings); return;
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


}  // namespace nexus

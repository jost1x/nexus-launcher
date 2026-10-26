#include "ui/ui.h"

#include "rpc/rpc_client.h"

#include <algorithm>
#include <array>

namespace nexus {

void UiController::navigate(View view) {
    navigate_to(view, true);
}

void UiController::navigate_to(View view, bool remember_current) {
    if (view == view_) return;
    if (view == View::Home) previous_view_.reset();
    else if (remember_current) {
        uint32_t focused_index = 0;
        auto* focused = lv_group_get_focused(focus_group_);
        const uint32_t count = lv_group_get_obj_count(focus_group_);
        for (uint32_t index = 0; index < count; ++index) {
            if (lv_group_get_obj_by_index(focus_group_, index) == focused) {
                focused_index = index;
                break;
            }
        }
        previous_view_ = NavigationEntry{view_, focused_index};
    }
    if (view_ == View::WifiAvailable && view != View::WifiAvailable && wifi_scan_id_ != 0) {
        rpc_.cancel(wifi_scan_id_);
        wifi_scan_id_ = 0;
        state_.wifi_scan_loading = false;
    }
    view_ = view;
    if (view == View::Wifi || view == View::WifiAvailable || view == View::WifiSaved) {
        if (wifi_watch_id_ == 0 && wifi_status_id_ == 0 && (!state_.wifi_status || !state_.wifi_status_error.empty())) {
            wifi_status_id_ = rpc_.request_wifi_status();
            state_.wifi_status_loading = wifi_status_id_ != 0;
            state_.wifi_status_error.clear();
        }
    }
    if (view == View::WifiAvailable) {
        if (wifi_scan_id_ == 0) {
            state_.wifi_scan_error.clear();
            state_.wifi_scan.reset();
            state_.wifi_scan_loading = true;
            wifi_scan_id_ = rpc_.scan_wifi(true);
        }
    }
    if (view == View::WifiSaved) {
        if (wifi_saved_id_ == 0 && !state_.saved_wifi_networks) {
            state_.wifi_saved_error.clear();
            state_.wifi_saved_loading = true;
            wifi_saved_id_ = rpc_.list_saved_wifi_networks();
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
    if (view_ == View::Home || !previous_view_) return;
    const auto previous = *previous_view_;
    previous_view_.reset();
    restore_focus_index_ = previous.focused_index;
    navigate_to(previous.view, false);
}

void UiController::cycle_section(int direction) {
    constexpr std::array settings_sections = {View::About, View::Wifi, View::Display, View::Audio, View::System};
    const bool in_settings = view_ == View::Settings || view_ == View::Wifi || view_ == View::Display ||
                             view_ == View::WifiAvailable || view_ == View::WifiSaved || view_ == View::Audio ||
                             view_ == View::System || view_ == View::About;
    if (in_settings) {
        const View current = view_ == View::Settings ? View::About
                             : view_ == View::WifiAvailable || view_ == View::WifiSaved ? View::Wifi
                                                                                         : view_;
        const auto iterator = std::find(settings_sections.begin(), settings_sections.end(), current);
        const int current_index = iterator == settings_sections.end()
                                      ? 0
                                      : static_cast<int>(iterator - settings_sections.begin());
        const int next_index = (current_index + direction + static_cast<int>(settings_sections.size())) %
                               static_cast<int>(settings_sections.size());
        navigate_to(settings_sections[static_cast<size_t>(next_index)], false);
        return;
    }

    constexpr std::array main_sections = {View::Home, View::Games, View::Apps, View::Favorites, View::About};
    const auto iterator = std::find(main_sections.begin(), main_sections.end(), view_);
    const int current_index = iterator == main_sections.end() ? 0 : static_cast<int>(iterator - main_sections.begin());
    const int next_index = (current_index + direction + static_cast<int>(main_sections.size())) %
                           static_cast<int>(main_sections.size());
    const bool remember_home = view_ == View::Home && !previous_view_;
    navigate_to(main_sections[static_cast<size_t>(next_index)], remember_home);
}

void UiController::dispatch(Action action) {
    switch (action) {
        case Action::Home: navigate(View::Home); return;
        case Action::Games: navigate(View::Games); return;
        case Action::Apps: navigate(View::Apps); return;
        case Action::Favorites: navigate(View::Favorites); return;
        case Action::Settings: navigate(View::About); return;
        case Action::Wifi: navigate(View::Wifi); return;
        case Action::ShowWifiAvailable: navigate(View::WifiAvailable); return;
        case Action::ShowWifiSaved: navigate(View::WifiSaved); return;
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
            state_.wifi_scan.reset();
            state_.wifi_scan_loading = true;
            wifi_scan_id_ = rpc_.scan_wifi(true);
            render();
            return;
        case Action::ToggleWifi:
            if (wifi_set_enabled_id_ != 0) return;
            if (!state_.wifi_status || !state_.wifi_status->radio_state_available()) {
                state_.wifi_action_message = "El control de radio Wi-Fi no está disponible.";
                render();
                return;
            }
            // A scan can occupy the RPC worker for up to 20 seconds. Stop it
            // before disabling the radio so the toggle is handled immediately.
            if (state_.wifi_status->radio_enabled() && wifi_scan_id_ != 0) {
                rpc_.cancel(wifi_scan_id_);
                wifi_scan_id_ = 0;
                state_.wifi_scan_loading = false;
            }
            state_.wifi_action_message = state_.wifi_status->radio_enabled() ? "Desactivando Wi-Fi..." : "Activando Wi-Fi...";
            state_.wifi_radio_changing = true;
            wifi_set_enabled_id_ = rpc_.set_wifi_enabled(!state_.wifi_status->radio_enabled());
            render();
            return;
        case Action::RefreshSavedWifiNetworks:
            if (wifi_saved_id_ != 0) return;
            state_.wifi_saved_error.clear();
            state_.wifi_saved_loading = true;
            wifi_saved_id_ = rpc_.list_saved_wifi_networks();
            render();
            return;
        case Action::SubmitWifiPassword:
            if (wifi_connect_id_ != 0 || state_.wifi_connect_ssid.empty()) return;
            if (state_.wifi_connect_security != "open" && state_.wifi_password.size() < 8) {
                state_.wifi_action_message = "La contraseña debe tener al menos 8 caracteres.";
                render();
                return;
            }
            state_.wifi_action_message = "Conectando y guardando la red...";
            state_.wifi_saved_error.clear();
            wifi_connect_id_ = rpc_.connect_wifi(state_.wifi_connect_ssid, state_.wifi_password,
                                                 state_.wifi_connect_security);
            std::fill(state_.wifi_password.begin(), state_.wifi_password.end(), '\0');
            state_.wifi_password.clear();
            state_.wifi_connect_ssid.clear();
            state_.wifi_connect_security.clear();
            wifi_keyboard_selected_button_ = LV_BUTTONMATRIX_BUTTON_NONE;
            wifi_keyboard_mode_ = LV_KEYBOARD_MODE_TEXT_LOWER;
            render();
            return;
        case Action::CancelWifiPassword:
            state_.wifi_connect_ssid.clear();
            state_.wifi_connect_security.clear();
            std::fill(state_.wifi_password.begin(), state_.wifi_password.end(), '\0');
            state_.wifi_password.clear();
            wifi_keyboard_selected_button_ = LV_BUTTONMATRIX_BUTTON_NONE;
            wifi_keyboard_mode_ = LV_KEYBOARD_MODE_TEXT_LOWER;
            state_.wifi_action_message.clear();
            render();
            return;
        case Action::ConnectWifiNetwork:
        case Action::ConnectSavedWifiNetwork:
        case Action::SetPreferredWifiNetwork:
        case Action::ForgetWifiNetwork:
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

void UiController::dispatch_wifi_action(const WifiActionContext& context) {
    switch (context.action) {
        case Action::ConnectWifiNetwork:
            state_.wifi_connect_ssid = context.ssid;
            state_.wifi_connect_security = context.security.empty() ? "open" : context.security;
            state_.wifi_password.clear();
            wifi_keyboard_selected_button_ = LV_BUTTONMATRIX_BUTTON_NONE;
            wifi_keyboard_mode_ = LV_KEYBOARD_MODE_TEXT_LOWER;
            if (state_.wifi_connect_security == "open") {
                state_.wifi_action_message.clear();
                dispatch(Action::SubmitWifiPassword);
            } else {
                state_.wifi_action_message.clear();
                render();
            }
            return;
        case Action::ConnectSavedWifiNetwork:
            if (wifi_connect_saved_id_ != 0) return;
            state_.wifi_action_message = "Conectando a la red guardada...";
            wifi_connect_saved_id_ = rpc_.connect_saved_wifi_network(context.id);
            render();
            return;
        case Action::SetPreferredWifiNetwork:
            if (wifi_preferred_id_ != 0) return;
            state_.wifi_action_message = "Guardando la red preferida...";
            wifi_preferred_id_ = rpc_.set_preferred_wifi_network(context.id);
            render();
            return;
        case Action::ForgetWifiNetwork:
            if (wifi_forget_id_ != 0) return;
            state_.wifi_action_message = "Olvidando la red...";
            wifi_forget_id_ = rpc_.forget_wifi_network(context.id);
            render();
            return;
        default:
            return;
    }
}


}  // namespace nexus

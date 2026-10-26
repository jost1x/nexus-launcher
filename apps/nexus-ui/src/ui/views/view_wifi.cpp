#include "ui/views/view_builders.h"
#include "ui/views/view_helpers.h"

#include "assets/icons/lucide_icons.h"
#include "ui/components/components.h"
#include "ui/ui.h"
#include "ui/ui_controls.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace nexus {
namespace {
using namespace components;
using namespace ui_controls;
using namespace view_helpers;

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

std::string wifi_state_text(const std::string& state) {
    const auto normalized = lowercase(state);
    if (normalized == "up") return "Activa";
    if (normalized == "down") return "Desactivada";
    if (normalized == "unavailable") return "No disponible";
    if (normalized == "unknown") return "Desconocido";
    if (normalized == "dormant") return "En espera";
    if (normalized == "lowerlayerdown") return "Capa inferior inactiva";
    if (normalized == "notpresent") return "No presente";
    if (normalized == "testing") return "Comprobando";
    return state.empty() ? "Desconocido" : "Otro estado";
}

std::string wifi_security_text(const std::string& security) {
    const auto normalized = lowercase(security);
    if (normalized == "secured" || normalized == "wpa-psk") return "Protegida";
    if (normalized == "open") return "Abierta";
    if (normalized.empty()) return "Seguridad desconocida";
    return security;
}

lv_obj_t* create_wifi_action_button(lv_obj_t* parent, const char* text, int width, int height,
                                    lv_group_t* group, WifiActionContext context) {
    auto* button = components::create_button(parent, text, width, height);
    components::style_focusable(button);
    auto* event_context = new WifiActionContext(std::move(context));
    lv_obj_add_event_cb(button, UiController::wifi_action_event, LV_EVENT_CLICKED, event_context);
    lv_obj_add_event_cb(button, UiController::wifi_action_delete_event, LV_EVENT_DELETE, event_context);
    lv_obj_add_event_cb(button, UiController::key_event, LV_EVENT_KEY, nullptr);
    if (group != nullptr) lv_group_add_obj(group, button);
    return button;
}

void build_password_prompt(lv_obj_t* panel, const UiState& state, lv_group_t* group, ViewObjects& objects) {
    const int width = lv_obj_get_content_width(panel);
    const int height = lv_obj_get_content_height(panel);
    auto* dialog = lv_obj_create(panel);
    lv_obj_set_size(dialog, width, height);
    lv_obj_set_pos(dialog, 0, 0);
    lv_obj_set_style_bg_color(dialog, lv_color_hex(0x111A2C), 0);
    lv_obj_set_style_bg_opa(dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dialog, 1, 0);
    lv_obj_set_style_border_color(dialog, lv_color_hex(0x26334A), 0);
    lv_obj_set_style_radius(dialog, 8, 0);
    lv_obj_set_style_pad_all(dialog, 8, 0);
    lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);

    auto* title = create_label(dialog, "Conectar a una red", kText, true);
    lv_obj_set_pos(title, 0, 0);
    auto* network = create_label(dialog, state.wifi_connect_ssid.c_str(), kMuted);
    lv_obj_set_width(network, width - 18);
    lv_obj_set_pos(network, 0, 31);

    auto* password = lv_textarea_create(dialog);
    lv_obj_set_size(password, width - 16, 34);
    lv_obj_set_pos(password, 0, 53);
    lv_textarea_set_password_mode(password, true);
    lv_textarea_set_one_line(password, true);
    lv_textarea_set_placeholder_text(password, "Contraseña Wi-Fi");
    lv_textarea_set_max_length(password, 64);
    auto* keyboard = lv_keyboard_create(dialog);
    lv_obj_set_size(keyboard, width - 16, std::max(132, height - 132));
    lv_obj_set_pos(keyboard, 0, 94);
    lv_keyboard_set_textarea(keyboard, password);
    lv_obj_add_event_cb(keyboard, UiController::wifi_password_ready_event, LV_EVENT_READY, nullptr);
    if (group != nullptr) lv_group_add_obj(group, keyboard);

    const int button_y = height - 38;
    auto* connect = create_action_button(dialog, "Conectar", Action::SubmitWifiPassword, group,
                                          std::max(110, (width - 22) / 2), 34);
    lv_obj_set_pos(connect, 0, button_y);
    auto* cancel = create_action_button(dialog, "Cancelar", Action::CancelWifiPassword, group,
                                         std::max(110, (width - 22) / 2), 34);
    lv_obj_align(cancel, LV_ALIGN_TOP_RIGHT, 0, button_y);
    objects.initial_focus = keyboard;
}

}  // namespace

void build_wifi_content(lv_obj_t* panel, const UiState& state, lv_group_t* group, ViewObjects& objects) {
    if (!state.wifi_connect_ssid.empty()) {
        build_password_prompt(panel, state, group, objects);
        return;
    }

    const int content_width = lv_obj_get_content_width(panel);
    const int content_height = lv_obj_get_content_height(panel);
    auto* heading = create_label(panel, "Wi-Fi", kText, true);
    lv_obj_set_pos(heading, 0, 0);

    std::string status_text = "Estado: ";
    status_text += state.wifi_status ? wifi_state_text(state.wifi_status->state()) : "desconocido";
    if (state.wifi_status && state.wifi_status->radio_state_available()) {
        status_text = state.wifi_status->radio_enabled() ? "Radio activada" : "Radio desactivada";
    }
    if (state.wifi_status && state.wifi_status->connected() && !state.wifi_status->ssid().empty()) {
        status_text += "  ·  " + state.wifi_status->ssid();
    }
    auto* connection = create_label(panel, status_text.c_str(), state.wifi_status && state.wifi_status->connected() ? kGood : kMuted);
    lv_obj_set_width(connection, std::max(150, content_width - 220));
    lv_obj_set_pos(connection, 0, 34);

    const bool radio_available = state.wifi_status && state.wifi_status->radio_state_available();
    const bool radio_enabled = !radio_available || state.wifi_status->radio_enabled();
    auto* toggle = create_action_button(panel,
                                        state.wifi_status && state.wifi_status->radio_enabled() ? "Desactivar" : "Activar",
                                        Action::ToggleWifi, group, 106, 34);
    lv_obj_align(toggle, LV_ALIGN_TOP_RIGHT, 0, 26);
    if (!radio_available || state.wifi_status_loading) lv_obj_add_state(toggle, LV_STATE_DISABLED);
    auto* scan = create_action_button(panel, state.wifi_scan_loading ? "Buscando..." : "Buscar redes",
                                      Action::RefreshWifi, group, 112, 34);
    lv_obj_align(scan, LV_ALIGN_TOP_RIGHT, 0, 64);
    if (!radio_enabled || state.wifi_scan_loading) lv_obj_add_state(scan, LV_STATE_DISABLED);

    int error_count = 0;
    for (const auto* error_text : {&state.wifi_status_error, &state.wifi_scan_error, &state.wifi_saved_error}) {
        if (error_text->empty()) continue;
        auto* error = create_label(panel, error_text->c_str(), kWarn);
        lv_obj_set_width(error, content_width);
        lv_obj_set_pos(error, 0, 78 + error_count++ * 18);
    }
    if (!state.wifi_action_message.empty()) {
        auto* message = create_label(panel, state.wifi_action_message.c_str(), kMuted);
        lv_obj_set_width(message, std::max(120, content_width - 120));
        lv_obj_set_pos(message, 0, 56);
    }

    const int list_y = std::max(101, 82 + error_count * 18);
    auto* list = lv_obj_create(panel);
    lv_obj_set_size(list, content_width, std::max(100, content_height - list_y));
    lv_obj_set_pos(list, 0, list_y);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 0, 0);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);

    int y = 0;
    auto* saved_heading = create_label(list, "Redes guardadas", kText);
    lv_obj_set_pos(saved_heading, 0, y);
    y += 22;
    if (state.wifi_saved_loading && !state.saved_wifi_networks) {
        auto* loading = create_label(list, "Cargando redes guardadas...", kMuted);
        lv_obj_set_pos(loading, 4, y);
        y += 26;
    } else if (!state.saved_wifi_networks || state.saved_wifi_networks->networks_size() == 0) {
        auto* empty = create_label(list, "Aún no hay redes guardadas.", kMuted);
        lv_obj_set_pos(empty, 4, y);
        y += 26;
    } else {
        for (const auto& network : state.saved_wifi_networks->networks()) {
            auto* row = lv_obj_create(list);
            const int row_width = std::max(150, content_width - 4);
            lv_obj_set_size(row, row_width, 36);
            lv_obj_set_pos(row, 0, y);
            lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(row, 0, 0);
            lv_obj_set_style_pad_all(row, 0, 0);
            const std::string name = network.ssid() + (network.connected() ? " · conectada" : "");
            create_wifi_action_button(row, name.c_str(), std::max(90, row_width - 140), 32, group,
                                      WifiActionContext{Action::ConnectSavedWifiNetwork, network.id()});
            const std::string preferred_text = network.preferred() ? "Preferida" : "★";
            auto* preferred = create_wifi_action_button(row, preferred_text.c_str(), 76, 32, group,
                                                        WifiActionContext{Action::SetPreferredWifiNetwork, network.id()});
            lv_obj_align(preferred, LV_ALIGN_TOP_RIGHT, -58, 0);
            auto* forget = create_wifi_action_button(row, "Olvidar", 56, 32, group,
                                                     WifiActionContext{Action::ForgetWifiNetwork, network.id()});
            lv_obj_align(forget, LV_ALIGN_TOP_RIGHT, 0, 0);
            y += 38;
        }
    }

    auto* available_heading = create_label(list, "Redes disponibles", kText);
    lv_obj_set_pos(available_heading, 0, y);
    y += 22;
    if (!radio_enabled) {
        auto* disabled = create_label(list, "Activa Wi-Fi para buscar redes.", kMuted);
        lv_obj_set_pos(disabled, 4, y);
    } else if (state.wifi_scan_loading && !state.wifi_scan) {
        auto* loading = create_label(list, "Buscando redes Wi-Fi...", kMuted);
        lv_obj_set_pos(loading, 4, y);
    } else if (!state.wifi_scan || state.wifi_scan->networks_size() == 0) {
        auto* empty = create_label(list, "Pulsa Buscar redes para actualizar.", kMuted);
        lv_obj_set_pos(empty, 4, y);
    } else {
        for (const auto& network : state.wifi_scan->networks()) {
            auto* row = lv_obj_create(list);
            const int row_width = std::max(160, content_width - 4);
            lv_obj_set_size(row, row_width, 38);
            lv_obj_set_pos(row, 0, y);
            lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(row, 0, 0);
            lv_obj_set_style_pad_all(row, 0, 0);
            const std::string name = network.ssid().empty() ? "Red oculta" : network.ssid();
            const std::string text = name + "  ·  " + wifi_security_text(network.security()) + "  ·  " +
                                     std::to_string(std::clamp(network.signal_percent(), 0, 100)) + "%";
            create_wifi_action_button(row, text.c_str(), row_width - 6, 34, group,
                                      WifiActionContext{Action::ConnectWifiNetwork, {}, name,
                                                        network.security().empty() ? "open" : network.security()});
            y += 40;
        }
    }

    objects.initial_focus = toggle;
    if (state.wifi_saved_loading) objects.initial_focus = scan;
}

}  // namespace nexus

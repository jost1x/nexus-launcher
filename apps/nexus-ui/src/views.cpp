#include "views.h"

#include "components.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <string>

namespace nexus {
namespace {

using namespace components;
const lv_color_t kBackground = lv_color_hex(0x070B14);
const lv_color_t kText = lv_color_hex(0xF1F4FC);
const lv_color_t kMuted = lv_color_hex(0x99A7BC);
const lv_color_t kGood = lv_color_hex(0x44D19A);
const lv_color_t kWarn = lv_color_hex(0xFFC857);

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

std::string battery_state_text(const std::string& state) {
    const auto normalized = lowercase(state);
    if (normalized == "charging") return "Cargando";
    if (normalized == "discharging") return "Descargando";
    if (normalized == "not charging") return "Sin cargar";
    if (normalized == "full") return "Completa";
    if (normalized == "unknown" || normalized.empty()) return "Desconocido";
    return "Otro estado";
}

std::string wifi_security_text(const std::string& security) {
    const auto normalized = lowercase(security);
    if (normalized == "secured") return "Protegida";
    if (normalized == "open") return "Abierta";
    if (normalized.empty()) return "Seguridad desconocida";
    return security;
}

const char* view_title(View view) {
    switch (view) {
        case View::Home: return "Inicio";
        case View::Wifi: return "Red Wi-Fi";
        case View::Display: return "Pantalla";
        case View::Audio: return "Audio";
        case View::System: return "Sistema";
        case View::About: return "Acerca de NEXUS";
    }
    return "NEXUS";
}

void set_card_text(lv_obj_t* card, const char* heading, const char* value, const char* detail = nullptr) {
    auto* title = create_label(card, heading, kMuted);
    lv_obj_set_pos(title, 0, 0);
    auto* main = create_label(card, value, kText);
    lv_obj_set_width(main, 182);
    lv_obj_set_pos(main, 0, 26);
    if (detail != nullptr) {
        auto* note = create_label(card, detail, kMuted);
        lv_obj_set_width(note, 182);
        lv_obj_set_pos(note, 0, 57);
    }
}

void slider_value_event(lv_event_t* event) {
    auto* slider = static_cast<lv_obj_t*>(lv_event_get_target(event));
    auto* value_label = static_cast<lv_obj_t*>(lv_event_get_user_data(event));
    lv_label_set_text_fmt(value_label, "%d %%", static_cast<int>(lv_slider_get_value(slider)));
}

ViewObjects create_shell(View view, const UiState& state, lv_group_t* group) {
    ViewObjects objects;
    lv_obj_clear_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(lv_screen_active(), LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(lv_screen_active(), kBackground, 0);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);
    lv_obj_set_style_radius(lv_screen_active(), 0, 0);
    auto* root = lv_obj_create(lv_screen_active());
    objects.root = root;
    lv_obj_set_size(root, 640, 480);
    lv_obj_set_style_bg_color(root, kBackground, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_radius(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(root, LV_SCROLLBAR_MODE_OFF);

    auto* header = lv_obj_create(root);
    lv_obj_set_size(header, 640, 48);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(header, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x0B1220), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_hor(header, 17, 0);
    auto* brand = create_label(header, "NEXUS", lv_color_hex(0xA8B4FF), true);
    lv_obj_align(brand, LV_ALIGN_LEFT_MID, 0, 0);
    auto* heading = create_label(header, view_title(view), kText);
    lv_obj_align(heading, LV_ALIGN_LEFT_MID, 106, 0);

    auto* status = create_label(header, "Conectando al sistema...", kMuted);
    objects.status = status;
    lv_obj_set_width(status, 292);
    lv_obj_set_style_text_align(status, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(status, LV_ALIGN_RIGHT_MID, 0, 0);

    auto* sidebar = create_card(root, 144, 414);
    lv_obj_set_pos(sidebar, 12, 56);
    const struct NavItem { const char* title; View view; Action action; } items[] = {
        {"Inicio", View::Home, Action::Home},
        {"Wi-Fi", View::Wifi, Action::Wifi},
        {"Pantalla", View::Display, Action::Display},
        {"Audio", View::Audio, Action::Audio},
        {"Sistema", View::System, Action::System},
        {"Acerca de", View::About, Action::About},
    };
    for (size_t i = 0; i < std::size(items); ++i) {
        auto* button = create_button(sidebar, items[i].title, items[i].action, group, 118, 43, items[i].view == view);
        lv_obj_set_pos(button, 0, 4 + static_cast<int>(i) * 53);
        if (items[i].view == view) objects.initial_focus = button;
    }

    auto* panel = create_card(root, 460, 414);
    lv_obj_set_pos(panel, 168, 56);
    lv_obj_set_style_pad_all(panel, 16, 0);

    if (view == View::Home) {
        auto* welcome = create_label(panel, "Estado del dispositivo", kText, true);
        lv_obj_set_pos(welcome, 0, 0);
        std::string wifi = state.wifi_status && state.wifi_status->connected()
                               ? state.wifi_status->ssid()
                               : (state.wifi_status ? "Sin conexión" : "No disponible");
        std::string battery = state.hardware && state.hardware->battery_available()
                                  ? std::to_string(state.hardware->battery_percent()) + " %"
                                  : "No disponible";
        std::string brightness = state.hardware && state.hardware->brightness_available()
                                     ? std::to_string(state.hardware->brightness() * 100 /
                                                      std::max<int64_t>(1, state.hardware->brightness_max())) + " %"
                                     : "No disponible";
        std::string volume = state.hardware && state.hardware->volume_available()
                                 ? std::to_string(state.hardware->volume_percent()) + " %"
                                 : "No disponible";
        auto* wifi_card = create_card(panel, 194, 105);
        lv_obj_set_pos(wifi_card, 0, 45);
        const std::string wifi_state = state.wifi_status ? wifi_state_text(state.wifi_status->state()) : "Estado pendiente";
        set_card_text(wifi_card, "Wi-Fi", wifi.c_str(), wifi_state.c_str());
        auto* battery_card = create_card(panel, 194, 105);
        lv_obj_set_pos(battery_card, 210, 45);
        const std::string battery_state = state.hardware && state.hardware->battery_available()
                                              ? battery_state_text(state.hardware->battery_status())
                                              : "Estado pendiente";
        set_card_text(battery_card, "Batería", battery.c_str(), battery_state.c_str());
        auto* display_card = create_card(panel, 194, 105);
        lv_obj_set_pos(display_card, 0, 164);
        set_card_text(display_card, "Brillo", brightness.c_str(), "Pantalla");
        auto* audio_card = create_card(panel, 194, 105);
        lv_obj_set_pos(audio_card, 210, 164);
        set_card_text(audio_card, "Volumen", volume.c_str(), "Audio");
        auto* open_wifi = create_button(panel, "Abrir Wi-Fi", Action::Wifi, group, 194, 42);
        lv_obj_set_pos(open_wifi, 0, 284);
        auto* open_settings = create_button(panel, "Ajustes del dispositivo", Action::System, group, 194, 42);
        lv_obj_set_pos(open_settings, 210, 284);
        if (objects.initial_focus == nullptr) objects.initial_focus = open_wifi;
    } else if (view == View::Wifi) {
        auto* heading = create_label(panel, "Redes disponibles", kText, true);
        lv_obj_set_pos(heading, 0, 0);
        std::string status_text = "Estado: ";
        status_text += state.wifi_status ? wifi_state_text(state.wifi_status->state()) : "desconocido";
        if (state.wifi_status && state.wifi_status->connected() && !state.wifi_status->ssid().empty()) {
            status_text += "  ·  " + state.wifi_status->ssid();
        }
        auto* connection = create_label(panel, status_text.c_str(), state.wifi_status && state.wifi_status->connected() ? kGood : kMuted);
        lv_obj_set_width(connection, 294);
        lv_obj_set_pos(connection, 0, 36);
        auto* scan = create_button(panel, state.wifi_scan_loading ? "Buscando..." : "Actualizar", Action::RefreshWifi, group, 118, 38);
        lv_obj_align(scan, LV_ALIGN_TOP_RIGHT, 0, 26);

        int error_count = 0;
        if (!state.wifi_status_error.empty()) {
            auto* error = create_label(panel, state.wifi_status_error.c_str(), kWarn);
            lv_obj_set_width(error, 420);
            lv_obj_set_pos(error, 0, 74 + error_count++ * 22);
        }
        if (!state.wifi_scan_error.empty()) {
            auto* error = create_label(panel, state.wifi_scan_error.c_str(), kWarn);
            lv_obj_set_width(error, 420);
            lv_obj_set_pos(error, 0, 74 + error_count++ * 22);
        }

        const int list_y = 82 + error_count * 22;
        const int cache_footer = state.wifi_scan && state.wifi_scan->from_cache() ? 24 : 0;
        auto* list = lv_obj_create(panel);
        lv_obj_set_size(list, 426, std::max(150, 374 - list_y - cache_footer));
        lv_obj_set_pos(list, 0, list_y);
        lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(list, 0, 0);
        lv_obj_set_style_pad_all(list, 0, 0);
        lv_obj_set_scroll_dir(list, LV_DIR_VER);
        lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);
        if (state.wifi_scan_loading && !state.wifi_scan) {
            auto* loading = create_label(list, "Buscando redes Wi-Fi...", kMuted);
            lv_obj_set_pos(loading, 4, 8);
        } else if (!state.wifi_scan || state.wifi_scan->networks_size() == 0) {
            auto* empty = create_label(list, state.wifi_scan ? "No se encontraron redes." : "Pulsa Actualizar para buscar redes.", kMuted);
            lv_obj_set_pos(empty, 4, 8);
        } else {
            int y = 0;
            for (const auto& network : state.wifi_scan->networks()) {
                auto* row = lv_obj_create(list);
                lv_obj_set_size(row, 405, 43);
                lv_obj_set_pos(row, 0, y);
                lv_obj_set_style_bg_color(row, network.connected() ? lv_color_hex(0x18264A) : lv_color_hex(0x0D1422), 0);
                lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
                lv_obj_set_style_border_width(row, network.connected() ? 1 : 0, 0);
                lv_obj_set_style_border_color(row, lv_color_hex(0x427CFF), 0);
                lv_obj_set_style_radius(row, 7, 0);
                lv_obj_set_style_pad_hor(row, 9, 0);
                lv_obj_set_style_pad_ver(row, 4, 0);
                const std::string ssid = network.ssid().empty() ? "Red oculta" : network.ssid();
                const std::string current = network.connected() ? "  (conectada)" : "";
                auto* name = create_label(row, (ssid + current).c_str(), kText);
                lv_obj_set_width(name, 265);
                lv_obj_align(name, LV_ALIGN_LEFT_MID, 0, 0);
                const std::string detail = wifi_security_text(network.security()) + "  ·  " +
                                           std::to_string(std::clamp(network.signal_percent(), 0, 100)) + " %";
                auto* metadata = create_label(row, detail.c_str(), kMuted);
                lv_obj_align(metadata, LV_ALIGN_RIGHT_MID, 0, 0);
                y += 47;
            }
        }
        if (state.wifi_scan && state.wifi_scan->from_cache()) {
            auto* cached = create_label(panel, "Resultados recientes (caché)", kMuted);
            lv_obj_align(cached, LV_ALIGN_BOTTOM_LEFT, 0, 0);
        }
        if (objects.initial_focus == nullptr) objects.initial_focus = scan;
    } else if (view == View::Display || view == View::Audio) {
        const bool display = view == View::Display;
        const bool available = state.hardware && (display ? state.hardware->brightness_available() : state.hardware->volume_available());
        const int current = display
                                ? (state.hardware && state.hardware->brightness_available()
                                       ? static_cast<int>(state.hardware->brightness() * 100 /
                                                          std::max<int64_t>(1, state.hardware->brightness_max()))
                                       : 0)
                                : (state.hardware && state.hardware->volume_available() ? state.hardware->volume_percent() : 0);
        const int selected = display ? (state.brightness_target >= 0 ? state.brightness_target : current)
                                     : (state.volume_target >= 0 ? state.volume_target : current);
        const Action slider_action = display ? Action::ApplyBrightness : Action::ApplyVolume;
        auto* title = create_label(panel, display ? "Brillo de pantalla" : "Volumen del sistema", kText, true);
        lv_obj_set_pos(title, 0, 0);
        auto* explanation = create_label(panel, display ? "Ajusta el nivel de retroiluminación." : "Ajusta el nivel de audio del dispositivo.", kMuted);
        lv_obj_set_pos(explanation, 0, 39);
        if (!available) {
            auto* unavailable = create_label(panel, "Este control no está disponible en el sistema.", kWarn);
            lv_obj_set_pos(unavailable, 0, 102);
        } else {
            auto* value = create_label(panel, (std::to_string(selected) + " %").c_str(), kText, true);
            lv_obj_set_pos(value, 0, 96);
            auto* slider = create_slider(panel, selected, slider_action, group, 398);
            lv_obj_set_pos(slider, 0, 153);
            lv_obj_add_event_cb(slider, slider_value_event, LV_EVENT_VALUE_CHANGED, value);
            auto* apply = create_button(panel, "Aplicar cambio", display ? Action::ApplyBrightness : Action::ApplyVolume, group, 184, 44);
            lv_obj_set_pos(apply, 0, 205);
            if (objects.initial_focus == nullptr) objects.initial_focus = slider;
        }
        if (!state.hardware_error.empty()) {
            auto* error = create_label(panel, state.hardware_error.c_str(), kWarn);
            lv_obj_set_width(error, 420);
            lv_obj_set_pos(error, 0, 280);
        }
        if (!state.action_message.empty()) {
            auto* message = create_label(panel, state.action_message.c_str(), kGood);
            lv_obj_set_width(message, 420);
            lv_obj_set_pos(message, 0, 280);
        }
    } else if (view == View::System) {
        auto* title = create_label(panel, "Información del sistema", kText, true);
        lv_obj_set_pos(title, 0, 0);
        if (!state.hardware && !state.system_info) {
            auto* loading = create_label(panel, "Consultando el dispositivo...", kMuted);
            lv_obj_set_pos(loading, 0, 50);
        } else {
            const auto* hw = state.hardware ? &*state.hardware : nullptr;
            const auto* sys = state.system_info ? &*state.system_info : nullptr;
            const std::string battery = hw && hw->battery_available()
                                            ? std::to_string(hw->battery_percent()) + " %  ·  " + battery_state_text(hw->battery_status())
                                            : "No disponible";
            const std::string charge = hw && hw->charger_available() ? (hw->charger_connected() ? "Conectado" : "Desconectado") : "No disponible";
            const std::string temperature = hw && hw->thermal_available()
                                                ? std::to_string(static_cast<int>(std::lround(hw->soc_temperature_celsius()))) + " °C"
                                                : "No disponible";
            const std::string os = sys ? sys->os_id() + " " + sys->os_version() : "No disponible";
            create_value_row(panel, "Batería", battery.c_str(), 50);
            create_value_row(panel, "Cargador", charge.c_str(), 87);
            create_value_row(panel, "Temperatura SoC", temperature.c_str(), 124);
            create_value_row(panel, "Sistema operativo", os.c_str(), 161);
            create_value_row(panel, "Kernel", sys ? sys->kernel().c_str() : "No disponible", 198);
            create_value_row(panel, "Arquitectura", sys ? sys->architecture().c_str() : "No disponible", 235);
        }
        if (!state.hardware_error.empty() || !state.system_error.empty()) {
            const auto& error_text = !state.hardware_error.empty() ? state.hardware_error : state.system_error;
            auto* error = create_label(panel, error_text.c_str(), kWarn);
            lv_obj_set_width(error, 420);
            lv_obj_set_pos(error, 0, 290);
        }
        if (objects.initial_focus == nullptr) objects.initial_focus = lv_group_get_obj_by_index(group, 0);
    } else {
        auto* title = create_label(panel, "NEXUS para R36S", kText, true);
        lv_obj_set_pos(title, 0, 0);
        auto* version = create_label(panel, "Versión de desarrollo 0.1", kMuted);
        lv_obj_set_pos(version, 0, 52);
        auto* description = create_label(panel, "Interfaz para configurar y consultar el dispositivo.", kText);
        lv_obj_set_width(description, 410);
        lv_obj_set_pos(description, 0, 96);
        auto* offline = create_label(panel, "La interfaz sigue disponible cuando nexusd no responde.", kMuted);
        lv_obj_set_width(offline, 410);
        lv_obj_set_pos(offline, 0, 132);
        if (objects.initial_focus == nullptr) objects.initial_focus = lv_group_get_obj_by_index(group, 0);
    }

    if (objects.initial_focus == nullptr) objects.initial_focus = lv_group_get_obj_by_index(group, 0);
    return objects;
}

}  // namespace

ViewObjects build_view(View view, const UiState& state, lv_group_t* group) {
    return create_shell(view, state, group);
}

}  // namespace nexus

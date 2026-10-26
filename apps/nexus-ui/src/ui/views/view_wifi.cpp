#include "ui/views/view_builders.h"
#include "ui/views/view_helpers.h"

#include "ui/components/components.h"
#include "ui/ui_controls.h"

#include <algorithm>
#include <cmath>
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
    if (normalized == "secured") return "Protegida";
    if (normalized == "open") return "Abierta";
    if (normalized.empty()) return "Seguridad desconocida";
    return security;
}

}  // namespace

void build_wifi_content(lv_obj_t* panel, const UiState& state, lv_group_t* group, ViewObjects& objects) {
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
        auto* scan = create_action_button(panel, state.wifi_scan_loading ? "Buscando..." : "Actualizar", Action::RefreshWifi, group, 118, 38);
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
        lv_obj_set_size(list, 554, std::max(120, 284 - list_y - cache_footer));
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
                lv_obj_set_size(row, 533, 43);
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
                lv_obj_set_width(name, 350);
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
        // The sidebar is created first, so explicitly focus the Wi-Fi action.
        objects.initial_focus = scan;
    
}

}  // namespace nexus

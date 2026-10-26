#include "ui/views/view_builders.h"
#include "ui/views/view_helpers.h"

#include "ui/components/components.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <string>

namespace nexus {
namespace {
using namespace components;
using namespace view_helpers;

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
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

}  // namespace

void build_system_content(lv_obj_t* panel, const UiState& state, lv_group_t* group, ViewObjects& objects) {
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
    
}

}  // namespace nexus

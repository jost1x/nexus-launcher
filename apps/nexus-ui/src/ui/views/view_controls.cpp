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



}  // namespace

void build_controls_content(View view, lv_obj_t* panel, const UiState& state, lv_group_t* group, ViewObjects& objects) {
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
            auto* slider = create_action_slider(panel, selected, slider_action, group, 520);
            lv_obj_set_pos(slider, 0, 153);
            bind_slider_value_label(slider, value);
            auto* apply = create_action_button(panel, "Aplicar cambio", display ? Action::ApplyBrightness : Action::ApplyVolume, group, 184, 44);
            lv_obj_set_pos(apply, 0, 205);
            // Preserve direct D-pad left/right adjustment on entry and after
            // status refreshes rebuild the view.
            objects.initial_focus = slider;
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
    
}

}  // namespace nexus

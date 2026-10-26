#include "ui/views/view_builders.h"
#include "ui/views/view_helpers.h"

#include "ui/components/components.h"
#include "ui/ui_controls.h"

#include <iterator>

namespace nexus {
using namespace components;
using namespace ui_controls;
using namespace view_helpers;

void build_settings_content(lv_obj_t* panel, const UiState&, lv_group_t* group, ViewObjects& objects) {
    auto* title = create_label(panel, "Configuración del dispositivo", kText, true);
    lv_obj_set_pos(title, 0, 0);
    auto* description = create_label(panel, "Elige un área para continuar.", kMuted);
    lv_obj_set_pos(description, 0, 39);
    const struct SettingsItem { const char* title; Action action; } settings[] = {
        {"Wi-Fi", Action::Wifi},
        {"Pantalla", Action::Display},
        {"Audio", Action::Audio},
        {"Sistema", Action::System},
        {"Acerca de", Action::About},
    };
    for (size_t i = 0; i < std::size(settings); ++i) {
        const int column = static_cast<int>(i % 2);
        const int row = static_cast<int>(i / 2);
        auto* button = create_action_button(panel, settings[i].title, settings[i].action, group, 194, 48);
        lv_obj_set_pos(button, column * 210, 86 + row * 62);
        if (i == 0) objects.initial_focus = button;
    }
}

}  // namespace nexus

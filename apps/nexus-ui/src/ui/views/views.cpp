#include "ui/views/views.h"
#include "ui/views/view_builders.h"

namespace nexus {

ViewObjects build_view(View view, const UiState& state, lv_group_t* group) {
    auto objects = build_view_shell(view, group);
    switch (view) {
        case View::Home:
            build_home_content(objects.panel, state, group, objects);
            break;
        case View::Games:
        case View::Apps:
        case View::Favorites:
            build_library_content(view, objects.panel, state, group, objects);
            break;
        case View::Settings:
            build_wifi_content(objects.panel, state, group, objects);
            break;
        case View::Wifi:
            build_wifi_content(objects.panel, state, group, objects);
            break;
        case View::Display:
        case View::Audio:
            build_controls_content(view, objects.panel, state, group, objects);
            break;
        case View::System:
            build_system_content(objects.panel, state, group, objects);
            break;
        case View::About:
            build_about_content(objects.panel, state, group, objects);
            break;
    }
    if (view == View::Settings) {
        objects.initial_focus = objects.settings_menu_focus;
    }
    if (objects.initial_focus == nullptr) objects.initial_focus = lv_group_get_obj_by_index(group, 0);
    return objects;
}

}  // namespace nexus

#include "ui/views/view_builders.h"
#include "ui/ui_controls.h"
#include "assets/icons/lucide_icons.h"

namespace nexus {
using namespace ui_controls;

void build_home_content(lv_obj_t* panel, const UiState&, lv_group_t* group, ViewObjects& objects) {
    constexpr int tile_size = 112;
    constexpr int tile_gap = 18;
    constexpr int row_width = tile_size * 4 + tile_gap * 3;
    constexpr int row_x = (640 - row_width) / 2;
    constexpr int row_y = 146;

    const struct LauncherItem {
        const char* title;
        const lv_image_dsc_t* icon;
        Action action;
    } items[] = {
        {"Juegos", &nexus_icon_gamepad_2, Action::Games},
        {"Aplicaciones", &nexus_icon_grid_2x2, Action::Apps},
        {"Favoritos", &nexus_icon_star, Action::Favorites},
        {"Ajustes", &nexus_icon_settings, Action::Settings},
    };

    for (int i = 0; i < 4; ++i) {
        auto* button = create_action_icon_button(panel, items[i].icon, items[i].title, items[i].action,
                                                group, tile_size);
        lv_obj_set_pos(button, row_x + i * (tile_size + tile_gap), row_y);
        if (i == 0) objects.initial_focus = button;
    }
}

}  // namespace nexus

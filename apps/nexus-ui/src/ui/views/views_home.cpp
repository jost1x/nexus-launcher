#include "ui/views/view_builders.h"
#include "ui/views/view_helpers.h"

#include "ui/components/components.h"
#include "ui/ui_controls.h"

namespace nexus {
using namespace components;
using namespace ui_controls;
using namespace view_helpers;

void build_home_content(lv_obj_t* panel, const UiState&, lv_group_t* group, ViewObjects& objects) {
        auto* welcome = create_label(panel, "Explora tu consola", kText, true);
        lv_obj_set_pos(welcome, 0, 0);
        auto* description = create_label(panel, "Selecciona una sección para continuar.", kMuted);
        lv_obj_set_pos(description, 0, 38);
        auto* open_games = create_action_button(panel, "Juegos\nAún sin biblioteca", Action::Games, group, 180, 164, true);
        lv_obj_set_pos(open_games, 0, 82);
        auto* open_apps = create_action_button(panel, "Aplicaciones\nSin accesos", Action::Apps, group, 180, 164);
        lv_obj_set_pos(open_apps, 202, 82);
        auto* open_favorites = create_action_button(panel, "Favoritos\nElige un juego", Action::Favorites, group, 180, 164);
        lv_obj_set_pos(open_favorites, 404, 82);
        objects.initial_focus = open_games;
}

}  // namespace nexus

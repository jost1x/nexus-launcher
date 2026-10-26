#include "ui/views/view_builders.h"
#include "ui/views/view_helpers.h"

#include "ui/components/components.h"
#include "ui/ui_controls.h"

#include <iterator>

namespace nexus {
using namespace components;
using namespace ui_controls;
using namespace view_helpers;

void build_settings_content(lv_obj_t* panel, const UiState&, lv_group_t*, ViewObjects&) {
    auto* title = create_label(panel, "Ajustes de la consola", kText, true);
    lv_obj_set_pos(title, 0, 0);
    auto* description = create_label(panel, "Elige una categoría en el menú lateral.", kMuted);
    lv_obj_set_pos(description, 0, 42);
    auto* divider = lv_obj_create(panel);
    lv_obj_set_size(divider, lv_obj_get_content_width(panel), 1);
    lv_obj_set_pos(divider, 0, 78);
    lv_obj_set_style_bg_color(divider, lv_color_hex(0x26334A), 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(divider, 0, 0);
    auto* details = create_label(panel, "Conectividad, pantalla, sonido e información del sistema.", kText);
    lv_obj_set_width(details, lv_obj_get_content_width(panel));
    lv_obj_set_pos(details, 0, 104);
}

}  // namespace nexus

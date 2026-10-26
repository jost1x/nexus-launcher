#include "ui/views/view_builders.h"
#include "ui/views/view_helpers.h"

#include "ui/components/components.h"

namespace nexus {
using namespace components;
using namespace view_helpers;

void build_about_content(lv_obj_t* panel, const UiState&, lv_group_t*, ViewObjects&) {
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
}

}  // namespace nexus

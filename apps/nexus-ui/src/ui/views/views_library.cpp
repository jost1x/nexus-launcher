#include "ui/views/view_builders.h"
#include "ui/views/view_helpers.h"

#include "ui/components/components.h"

namespace nexus {
using namespace components;
using namespace view_helpers;

void build_library_content(View view, lv_obj_t* panel, const UiState&, lv_group_t*, ViewObjects&) {
        const char* title = view == View::Games ? "Biblioteca de juegos"
                            : view == View::Apps ? "Tus aplicaciones"
                                                 : "Juegos favoritos";
        const char* message = view == View::Games ? "Aún no hay juegos en la biblioteca."
                              : view == View::Apps ? "Aún no hay aplicaciones añadidas."
                                                    : "Tus juegos favoritos aparecerán aquí.";
        auto* section_title = create_label(panel, title, kText, true);
        lv_obj_set_pos(section_title, 0, 0);
        auto* empty_state = create_card(panel, 550, 142);
        lv_obj_set_pos(empty_state, 0, 58);
        auto* empty_message = create_label(empty_state, message, kMuted);
        lv_obj_set_width(empty_message, 520);
        lv_obj_align(empty_message, LV_ALIGN_CENTER, 0, 0);
}

}  // namespace nexus

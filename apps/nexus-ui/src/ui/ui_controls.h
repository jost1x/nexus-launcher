#pragma once

#include <lvgl.h>

#include "ui/ui.h"

namespace nexus::ui_controls {

lv_obj_t* create_action_button(lv_obj_t* parent, const char* text, Action action, lv_group_t* group,
                               int width, int height, bool selected = false);
lv_obj_t* create_action_slider(lv_obj_t* parent, int value, Action action, lv_group_t* group, int width);
void bind_slider_value_label(lv_obj_t* slider, lv_obj_t* value_label);

}  // namespace nexus::ui_controls

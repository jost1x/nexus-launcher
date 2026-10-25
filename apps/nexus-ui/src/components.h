#pragma once

#include <lvgl.h>

#include "ui.h"

namespace nexus::components {

lv_obj_t* create_label(lv_obj_t* parent, const char* text, lv_color_t color, bool large = false);
lv_obj_t* create_card(lv_obj_t* parent, int width, int height);
lv_obj_t* create_button(lv_obj_t* parent, const char* text, Action action, lv_group_t* group,
                        int width, int height, bool selected = false);
lv_obj_t* create_slider(lv_obj_t* parent, int value, Action action, lv_group_t* group, int width);
lv_obj_t* create_value_row(lv_obj_t* parent, const char* title, const char* value, int y);
lv_obj_t* create_section_title(lv_obj_t* parent, const char* title, int y);
void style_focusable(lv_obj_t* object);

}  // namespace nexus::components

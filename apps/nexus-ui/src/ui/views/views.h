#pragma once

#include <lvgl.h>

#include "ui/ui_state.h"

namespace nexus {

struct ViewObjects {
    lv_obj_t* root = nullptr;
    lv_obj_t* panel = nullptr;
    lv_obj_t* clock = nullptr;
    lv_obj_t* wifi_icon = nullptr;
    lv_obj_t* battery_icon = nullptr;
    lv_obj_t* battery_fill = nullptr;
    lv_obj_t* battery_value = nullptr;
    lv_obj_t* initial_focus = nullptr;
};

ViewObjects build_view(View view, const UiState& state, lv_group_t* group);

}  // namespace nexus

#pragma once

#include <lvgl.h>

#include "ui.h"

namespace nexus {

struct ViewObjects {
    lv_obj_t* root = nullptr;
    lv_obj_t* status = nullptr;
    lv_obj_t* initial_focus = nullptr;
};

ViewObjects build_view(View view, const UiState& state, lv_group_t* group);

}  // namespace nexus

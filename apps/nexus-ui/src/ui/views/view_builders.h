#pragma once

#include "ui/ui_state.h"
#include "ui/views/views.h"

namespace nexus {

ViewObjects build_view_shell(View view, lv_group_t* group);
void build_home_content(lv_obj_t* panel, const UiState& state, lv_group_t* group, ViewObjects& objects);
void build_library_content(View view, lv_obj_t* panel, const UiState& state, lv_group_t* group, ViewObjects& objects);
void build_settings_content(lv_obj_t* panel, const UiState& state, lv_group_t* group, ViewObjects& objects);
void build_wifi_content(lv_obj_t* panel, const UiState& state, lv_group_t* group, ViewObjects& objects);
void build_controls_content(View view, lv_obj_t* panel, const UiState& state, lv_group_t* group, ViewObjects& objects);
void build_system_content(lv_obj_t* panel, const UiState& state, lv_group_t* group, ViewObjects& objects);
void build_about_content(lv_obj_t* panel, const UiState& state, lv_group_t* group, ViewObjects& objects);

}  // namespace nexus

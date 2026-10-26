#include "ui/ui_controls.h"

#include "ui/components/components.h"

#include <cstdint>

namespace nexus::ui_controls {
namespace {

void slider_value_event(lv_event_t* event) {
    auto* slider = static_cast<lv_obj_t*>(lv_event_get_target(event));
    auto* value_label = static_cast<lv_obj_t*>(lv_event_get_user_data(event));
    lv_label_set_text_fmt(value_label, "%d %%", static_cast<int>(lv_slider_get_value(slider)));
}

}  // namespace

lv_obj_t* create_action_button(lv_obj_t* parent, const char* text, Action action, lv_group_t* group,
                               int width, int height, bool selected) {
    auto* button = components::create_button(parent, text, width, height, selected);
    components::style_focusable(button);
    lv_obj_add_event_cb(button, UiController::action_event, LV_EVENT_CLICKED,
                        reinterpret_cast<void*>(static_cast<std::uintptr_t>(action)));
    lv_obj_add_event_cb(button, UiController::key_event, LV_EVENT_KEY, nullptr);
    if (group != nullptr) lv_group_add_obj(group, button);
    return button;
}

lv_obj_t* create_action_slider(lv_obj_t* parent, int value, Action action, lv_group_t* group, int width) {
    auto* slider = components::create_slider(parent, value, width);
    components::style_focusable(slider);
    lv_obj_add_event_cb(slider, UiController::slider_event, LV_EVENT_VALUE_CHANGED,
                        reinterpret_cast<void*>(static_cast<std::uintptr_t>(action)));
    lv_obj_add_event_cb(slider, UiController::key_event, LV_EVENT_KEY,
                        reinterpret_cast<void*>(static_cast<std::uintptr_t>(1)));
    if (group != nullptr) lv_group_add_obj(group, slider);
    return slider;
}

void bind_slider_value_label(lv_obj_t* slider, lv_obj_t* value_label) {
    lv_obj_add_event_cb(slider, slider_value_event, LV_EVENT_VALUE_CHANGED, value_label);
}

}  // namespace nexus::ui_controls

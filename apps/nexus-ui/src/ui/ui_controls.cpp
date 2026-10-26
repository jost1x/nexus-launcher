#include "ui/ui_controls.h"

#include "ui/components/components.h"
#include "assets/icons/lucide_icons.h"

#include <algorithm>
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

lv_obj_t* create_action_icon_label_button(lv_obj_t* parent, const lv_image_dsc_t* icon, const char* text,
                                          Action action, lv_group_t* group, int width, int height,
                                          bool selected) {
    auto* button = components::create_button(parent, "", width, height, selected);
    components::style_focusable(button);

    auto* image = lv_image_create(button);
    lv_image_set_src(image, icon);
    const int icon_width = icon == nullptr ? 24 : icon->header.w;
    lv_image_set_scale(image, static_cast<uint32_t>(20 * 256 / std::max(1, icon_width)));
    lv_obj_set_style_image_recolor(image, lv_color_hex(0xF1F4FC), 0);
    lv_obj_set_style_image_recolor_opa(image, LV_OPA_COVER, 0);
    lv_obj_align(image, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_clear_flag(image, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(image, LV_OBJ_FLAG_SCROLLABLE);

    auto* label = components::create_label(button, text, lv_color_hex(0xF1F4FC));
    lv_obj_set_width(label, std::max(0, width - 42));
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 38, 0);

    lv_obj_add_event_cb(button, UiController::action_event, LV_EVENT_CLICKED,
                        reinterpret_cast<void*>(static_cast<std::uintptr_t>(action)));
    lv_obj_add_event_cb(button, UiController::key_event, LV_EVENT_KEY, nullptr);
    if (group != nullptr) lv_group_add_obj(group, button);
    return button;
}

lv_obj_t* create_action_icon_stacked_button(lv_obj_t* parent, const lv_image_dsc_t* icon, const char* text,
                                            Action action, lv_group_t* group, int width, int height,
                                            bool selected) {
    auto* button = components::create_button(parent, "", width, height, selected);
    components::style_focusable(button);

    auto* image = lv_image_create(button);
    lv_image_set_src(image, icon);
    const int icon_width = icon == nullptr ? 24 : icon->header.w;
    lv_image_set_scale(image, static_cast<uint32_t>(16 * 256 / std::max(1, icon_width)));
    lv_obj_set_style_image_recolor(image, lv_color_hex(0xF1F4FC), 0);
    lv_obj_set_style_image_recolor_opa(image, LV_OPA_COVER, 0);
    lv_obj_align(image, LV_ALIGN_TOP_MID, 0, 3);
    lv_obj_clear_flag(image, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(image, LV_OBJ_FLAG_SCROLLABLE);

    auto* label = components::create_label(button, text, lv_color_hex(0xF1F4FC));
    lv_obj_set_width(label, width - 16);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(label, LV_ALIGN_BOTTOM_LEFT, 8, -2);

    lv_obj_add_event_cb(button, UiController::action_event, LV_EVENT_CLICKED,
                        reinterpret_cast<void*>(static_cast<std::uintptr_t>(action)));
    lv_obj_add_event_cb(button, UiController::key_event, LV_EVENT_KEY, nullptr);
    if (group != nullptr) lv_group_add_obj(group, button);
    return button;
}

lv_obj_t* create_action_icon_button(lv_obj_t* parent, const lv_image_dsc_t* icon, const char* text,
                                    Action action, lv_group_t* group, int size) {
    auto* button = lv_button_create(parent);
    lv_obj_set_size(button, size, size);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x111A2C), 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x5C39F5), LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_border_color(button, lv_color_hex(0x26334A), 0);
    lv_obj_set_style_border_color(button, lv_color_hex(0x8A7AFF), LV_STATE_FOCUSED);
    lv_obj_set_style_radius(button, 12, 0);
    lv_obj_set_style_pad_all(button, 0, 0);
    lv_obj_set_style_shadow_width(button, 0, 0);
    components::style_focusable(button);

    auto* image = lv_image_create(button);
    lv_image_set_src(image, icon);
    lv_obj_set_style_image_recolor(image, lv_color_hex(0xC0B6FF), 0);
    lv_obj_set_style_image_recolor_opa(image, LV_OPA_COVER, 0);
    lv_obj_set_style_image_recolor(image, lv_color_hex(0xFFFFFF), LV_STATE_FOCUSED);
    lv_obj_align(image, LV_ALIGN_CENTER, 0, -15);
    lv_obj_clear_flag(image, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(image, LV_OBJ_FLAG_SCROLLABLE);

    auto* label = components::create_label(button, text, lv_color_hex(0xF1F4FC));
    lv_obj_set_width(label, size - 8);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 34);

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

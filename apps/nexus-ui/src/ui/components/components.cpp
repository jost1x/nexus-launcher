#include "ui/components/components.h"
#include "assets/fonts.h"

namespace nexus::components {
namespace {

const lv_color_t kPanel = lv_color_hex(0x111A2C);
const lv_color_t kPanelLight = lv_color_hex(0x1A263A);
const lv_color_t kAccent = lv_color_hex(0x5C39F5);
const lv_color_t kFocus = lv_color_hex(0x13A8FF);
const lv_color_t kText = lv_color_hex(0xF1F4FC);
const lv_color_t kMuted = lv_color_hex(0x99A7BC);

const lv_style_transition_dsc_t* focus_transition() {
    static const lv_style_prop_t properties[] = {
        LV_STYLE_BG_COLOR,
        LV_STYLE_BORDER_COLOR,
        LV_STYLE_OUTLINE_COLOR,
        LV_STYLE_OUTLINE_WIDTH,
        0,
    };
    static lv_style_transition_dsc_t transition;
    static bool initialized = false;
    if (!initialized) {
        lv_style_transition_dsc_init(&transition, properties, lv_anim_path_ease_in_out, 120, 0, nullptr);
        initialized = true;
    }
    return &transition;
}

}  // namespace

lv_obj_t* create_label(lv_obj_t* parent, const char* text, lv_color_t color, bool large) {
    auto* object = lv_label_create(parent);
    lv_label_set_text(object, text);
    lv_obj_set_style_text_color(object, color, 0);
    lv_obj_set_style_text_font(object, large ? &nexus_font_28 : &nexus_font_16, 0);
    lv_label_set_long_mode(object, LV_LABEL_LONG_DOT);
    return object;
}

lv_obj_t* create_card(lv_obj_t* parent, int width, int height) {
    auto* card = lv_obj_create(parent);
    lv_obj_set_size(card, width, height);
    lv_obj_set_style_bg_color(card, kPanel, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x26334A), 0);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);
    return card;
}

void style_focusable(lv_obj_t* object) {
    lv_obj_set_style_transition(object, focus_transition(), LV_PART_MAIN);
    lv_obj_set_style_outline_width(object, 2, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(object, kFocus, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(object, 2, LV_STATE_FOCUSED);
}

lv_obj_t* create_button(lv_obj_t* parent, const char* text, int width, int height, bool selected) {
    auto* button = lv_button_create(parent);
    lv_obj_set_size(button, width, height);
    lv_obj_set_style_bg_color(button, selected ? kAccent : kPanelLight, 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(button, 9, 0);
    lv_obj_set_style_border_width(button, 0, 0);
    lv_obj_set_style_pad_hor(button, 10, 0);
    lv_obj_set_style_shadow_width(button, selected ? 8 : 0, 0);
    lv_obj_set_style_shadow_color(button, kAccent, 0);
    auto* label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &nexus_font_16, 0);
    lv_obj_set_style_text_color(label, kText, 0);
    lv_obj_center(label);
    return button;
}

lv_obj_t* create_slider(lv_obj_t* parent, int value, int width) {
    auto* slider = lv_slider_create(parent);
    lv_obj_set_size(slider, width, 20);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, value, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x27334A), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, kAccent, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, kText, LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider, 3, LV_PART_KNOB);
    return slider;
}

lv_obj_t* create_value_row(lv_obj_t* parent, const char* title, const char* value, int y) {
    auto* name = create_label(parent, title, kMuted);
    lv_obj_set_pos(name, 0, y);
    auto* content = create_label(parent, value, kText);
    lv_obj_set_width(content, 190);
    lv_obj_set_style_text_align(content, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(content, LV_ALIGN_TOP_RIGHT, 0, y);
    return content;
}

}  // namespace nexus::components

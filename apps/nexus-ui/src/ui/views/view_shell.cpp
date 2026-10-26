#include "ui/views/views.h"
#include "ui/views/view_helpers.h"

#include "ui/components/components.h"
#include "ui/ui_controls.h"
#include "assets/fonts.h"
#include "assets/icons/lucide_icons.h"

#include <iterator>

namespace nexus {
namespace {
using namespace components;
using namespace ui_controls;
using namespace view_helpers;

const char* view_title(View view) {
    switch (view) {
        case View::Home: return "Inicio";
        case View::Games: return "Juegos";
        case View::Apps: return "Aplicaciones";
        case View::Favorites: return "Favoritos";
        case View::Settings: return "Ajustes";
        case View::Wifi: return "Red Wi-Fi";
        case View::Display: return "Pantalla";
        case View::Audio: return "Audio";
        case View::System: return "Sistema";
        case View::About: return "Acerca de NEXUS";
    }
    return "NEXUS";
}

}  // namespace

ViewObjects build_view_shell(View view, lv_group_t* group) {
    ViewObjects objects;
    lv_obj_clear_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(lv_screen_active(), LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(lv_screen_active(), kBackground, 0);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);
    lv_obj_set_style_radius(lv_screen_active(), 0, 0);
    auto* root = lv_obj_create(lv_screen_active());
    objects.root = root;
    lv_obj_set_size(root, 640, 480);
    lv_obj_set_style_bg_color(root, kBackground, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_radius(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(root, LV_SCROLLBAR_MODE_OFF);

    auto* header = lv_obj_create(root);
    lv_obj_set_size(header, 640, 48);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(header, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x0B1220), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_hor(header, 17, 0);
    auto* brand = create_label(header, "NEXUS", lv_color_hex(0xA8B4FF), true);
    lv_obj_align(brand, LV_ALIGN_LEFT_MID, 0, 0);
    auto* heading = create_label(header, view_title(view), kText);
    lv_obj_align_to(heading, brand, LV_ALIGN_OUT_RIGHT_MID, 12, 0);

    objects.clock = create_label(header, "--:--", kMuted);
    lv_obj_set_width(objects.clock, 68);
    lv_obj_align(objects.clock, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_align(objects.clock, LV_TEXT_ALIGN_CENTER, 0);

    auto* status = lv_obj_create(header);
    lv_obj_set_size(status, 96, 34);
    lv_obj_clear_flag(status, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(status, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(status, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(status, 0, 0);
    lv_obj_set_style_pad_all(status, 0, 0);
    lv_obj_align(status, LV_ALIGN_RIGHT_MID, 0, 0);

    objects.wifi_icon = lv_image_create(status);
    lv_image_set_src(objects.wifi_icon, &nexus_icon_wifi);
    lv_obj_set_pos(objects.wifi_icon, 0, 5);
    lv_obj_set_style_image_recolor(objects.wifi_icon, kMuted, 0);
    lv_obj_set_style_image_recolor_opa(objects.wifi_icon, LV_OPA_COVER, 0);
    lv_obj_clear_flag(objects.wifi_icon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(objects.wifi_icon, LV_OBJ_FLAG_SCROLLABLE);

    auto* battery = lv_obj_create(status);
    lv_obj_set_size(battery, 24, 24);
    lv_obj_set_pos(battery, 30, 5);
    lv_obj_clear_flag(battery, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(battery, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(battery, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(battery, 0, 0);
    lv_obj_set_style_pad_all(battery, 0, 0);
    objects.battery_icon = lv_image_create(battery);
    lv_image_set_src(objects.battery_icon, &nexus_icon_battery);
    lv_obj_set_style_image_recolor(objects.battery_icon, kMuted, 0);
    lv_obj_set_style_image_recolor_opa(objects.battery_icon, LV_OPA_COVER, 0);
    lv_obj_clear_flag(objects.battery_icon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(objects.battery_icon, LV_OBJ_FLAG_SCROLLABLE);
    objects.battery_fill = lv_obj_create(battery);
    lv_obj_set_size(objects.battery_fill, 11, 8);
    lv_obj_set_pos(objects.battery_fill, 4, 8);
    lv_obj_clear_flag(objects.battery_fill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(objects.battery_fill, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(objects.battery_fill, kGood, 0);
    lv_obj_set_style_bg_opa(objects.battery_fill, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(objects.battery_fill, 0, 0);
    lv_obj_set_style_radius(objects.battery_fill, 1, 0);

    objects.battery_value = create_label(status, "--%", kMuted);
    lv_obj_set_width(objects.battery_value, LV_SIZE_CONTENT);
    lv_obj_align(objects.battery_value, LV_ALIGN_LEFT_MID, 56, 0);
    lv_label_set_long_mode(objects.battery_value, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(objects.battery_value, &nexus_font_16, 0);
    lv_obj_set_style_text_align(objects.battery_value, LV_TEXT_ALIGN_LEFT, 0);

    auto* navigation = create_card(root, 616, 42);
    lv_obj_set_pos(navigation, 12, 54);
    lv_obj_set_style_pad_all(navigation, 4, 0);
    const bool settings_page = view == View::Wifi || view == View::Display || view == View::Audio ||
                               view == View::System || view == View::About;
    const View selected_nav_view = settings_page ? View::Settings : view;
    const struct NavItem { const char* title; View view; Action action; } items[] = {
        {"Inicio", View::Home, Action::Home},
        {"Juegos", View::Games, Action::Games},
        {"Aplicaciones", View::Apps, Action::Apps},
        {"Favoritos", View::Favorites, Action::Favorites},
        {"Ajustes", View::Settings, Action::Settings},
    };
    for (size_t i = 0; i < std::size(items); ++i) {
        auto* button = create_action_button(navigation, items[i].title, items[i].action, group, 118, 34,
                                     items[i].view == selected_nav_view);
        lv_obj_set_pos(button, 4 + static_cast<int>(i) * 120, 0);
        if (items[i].view == selected_nav_view) objects.initial_focus = button;
    }

    auto* panel = create_card(root, 616, 326);
    objects.panel = panel;
    lv_obj_set_pos(panel, 12, 104);
    lv_obj_set_style_pad_all(panel, 16, 0);

    auto* controls_hint = create_label(root, "A: aceptar    ·    B: volver", kMuted);
    lv_obj_align(controls_hint, LV_ALIGN_BOTTOM_MID, 0, -7);

    return objects;
}

}  // namespace nexus

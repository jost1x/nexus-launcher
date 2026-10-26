#include "ui/views/view_builders.h"
#include "ui/views/view_helpers.h"

#include "assets/icons/lucide_icons.h"
#include "assets/fonts.h"
#include "ui/components/components.h"
#include "ui/ui.h"
#include "ui/ui_controls.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <string>

namespace nexus {
namespace {
using namespace components;
using namespace ui_controls;
using namespace view_helpers;

const char* const kWifiNumericKeyboardMap[] = {
    "abc", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", LV_SYMBOL_BACKSPACE, "\n",
    "+", "-", "_", "@", "#", "$", "%", "&", "*", "(", ")", LV_SYMBOL_NEW_LINE, "\n",
    ".", ",", ":", ";", "!", "?", "/", "\\", "'", "\"", "\n",
    LV_SYMBOL_KEYBOARD, LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, ""
};

constexpr std::array<lv_buttonmatrix_ctrl_t, 39> make_wifi_numeric_ctrl_map() {
    std::array<lv_buttonmatrix_ctrl_t, 39> controls{};
    for (auto& control : controls) {
        control = LV_BUTTONMATRIX_CTRL_NO_REPEAT;
    }
    return controls;
}

constexpr auto kWifiNumericKeyboardCtrlMap = make_wifi_numeric_ctrl_map();

void wifi_keyboard_activate_on_press(lv_obj_t* keyboard) {
    // LVGL's release-trigger keys lose their selection during the keypad
    // press/release sequence, so trigger all Wi-Fi keys on press.
    lv_buttonmatrix_clear_button_ctrl_all(keyboard, LV_BUTTONMATRIX_CTRL_CLICK_TRIG);
}

void wifi_keyboard_event(lv_event_t* event) {
    auto* keyboard = static_cast<lv_obj_t*>(lv_event_get_target(event));
    auto* textarea = lv_keyboard_get_textarea(keyboard);
    if (textarea == nullptr) return;
    const uint32_t selected = lv_keyboard_get_selected_button(keyboard);
    const char* key = lv_keyboard_get_button_text(keyboard, selected);
    if (key == nullptr) return;

    if (std::strcmp(key, "1#") == 0) {
        lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_USER_1);
        wifi_keyboard_activate_on_press(keyboard);
        // The numeric map starts with its mode switch at index 0. Select 1
        // so the mode change visibly lands on a digit ready to be entered.
        lv_buttonmatrix_set_selected_button(keyboard, 1);
    } else if (std::strcmp(key, "abc") == 0) {
        const uint32_t previous_selection = selected;
        lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
        wifi_keyboard_activate_on_press(keyboard);
        // Keep the case-toggle key under focus when changing case. In the
        // numeric map, move from its mode switch to the first letter instead.
        lv_buttonmatrix_set_selected_button(keyboard, previous_selection == 13 ? 13 : 1);
    } else if (std::strcmp(key, "ABC") == 0) {
        lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_UPPER);
        wifi_keyboard_activate_on_press(keyboard);
        lv_buttonmatrix_set_selected_button(keyboard, 13);
    } else if (std::strcmp(key, LV_SYMBOL_BACKSPACE) == 0) {
        lv_textarea_delete_char(textarea);
    } else if (std::strcmp(key, LV_SYMBOL_LEFT) == 0) {
        lv_textarea_cursor_left(textarea);
    } else if (std::strcmp(key, LV_SYMBOL_RIGHT) == 0) {
        lv_textarea_cursor_right(textarea);
    } else if (std::strcmp(key, LV_SYMBOL_OK) == 0) {
        lv_obj_send_event(keyboard, LV_EVENT_READY, nullptr);
    } else if (std::strcmp(key, LV_SYMBOL_KEYBOARD) == 0 || std::strcmp(key, LV_SYMBOL_CLOSE) == 0) {
        lv_obj_send_event(keyboard, LV_EVENT_CANCEL, nullptr);
    } else if (std::strcmp(key, LV_SYMBOL_NEW_LINE) == 0) {
        if (lv_textarea_get_one_line(textarea)) lv_obj_send_event(keyboard, LV_EVENT_READY, nullptr);
        else lv_textarea_add_char(textarea, '\n');
    } else {
        lv_textarea_add_text(textarea, key);
    }
}

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

std::string wifi_state_text(const std::string& state) {
    const auto normalized = lowercase(state);
    if (normalized == "up") return "Activa";
    if (normalized == "down") return "Desactivada";
    if (normalized == "unavailable") return "No disponible";
    if (normalized == "unknown") return "Desconocido";
    if (normalized == "dormant") return "En espera";
    if (normalized == "lowerlayerdown") return "Capa inferior inactiva";
    if (normalized == "notpresent") return "No presente";
    if (normalized == "testing") return "Comprobando";
    return state.empty() ? "Desconocido" : "Otro estado";
}

std::string wifi_security_text(const std::string& security) {
    const auto normalized = lowercase(security);
    if (normalized == "secured" || normalized == "wpa-psk") return "Protegida";
    if (normalized == "open") return "Abierta";
    if (normalized.empty()) return "Seguridad desconocida";
    return security;
}

lv_obj_t* create_wifi_action_button(lv_obj_t* parent, const char* text, int width, int height,
                                    lv_group_t* group, WifiActionContext context) {
    auto* button = components::create_button(parent, text, width, height);
    components::style_focusable(button);
    auto* event_context = new WifiActionContext(std::move(context));
    lv_obj_add_event_cb(button, UiController::wifi_action_event, LV_EVENT_CLICKED, event_context);
    lv_obj_add_event_cb(button, UiController::wifi_action_delete_event, LV_EVENT_DELETE, event_context);
    lv_obj_add_event_cb(button, UiController::key_event, LV_EVENT_KEY, nullptr);
    if (group != nullptr) lv_group_add_obj(group, button);
    return button;
}

lv_obj_t* create_wifi_network_button(lv_obj_t* list, const char* text, lv_group_t* group,
                                     WifiActionContext context, int width, int y) {
    auto* button = lv_label_create(list);
    lv_label_set_text(button, text);
    lv_obj_set_size(button, width, 34);
    lv_obj_set_pos(button, 0, y);
    lv_obj_add_flag(button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_text_font(button, &nexus_font_16, 0);
    lv_obj_set_style_text_color(button, lv_color_hex(0xF1F4FC), 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x1A263A), 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(button, 0, 0);
    lv_obj_set_style_radius(button, 8, 0);
    lv_obj_set_style_pad_hor(button, 9, 0);
    lv_obj_set_style_text_align(button, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_long_mode(button, LV_LABEL_LONG_DOT);
    lv_obj_set_height(button, 34);
    style_focusable(button);
    auto* event_context = new WifiActionContext(std::move(context));
    lv_obj_add_event_cb(button, UiController::wifi_action_event, LV_EVENT_CLICKED, event_context);
    lv_obj_add_event_cb(button, UiController::wifi_action_delete_event, LV_EVENT_DELETE, event_context);
    lv_obj_add_event_cb(button, UiController::key_event, LV_EVENT_KEY, nullptr);
    if (group != nullptr) lv_group_add_obj(group, button);
    return button;
}

void style_wifi_list_heading(lv_obj_t* label) {
    lv_obj_set_style_text_color(label, lv_color_hex(0xF1F4FC), 0);
    lv_obj_set_style_text_font(label, &nexus_font_16, 0);
    lv_obj_set_style_pad_top(label, 5, 0);
    lv_obj_set_style_pad_bottom(label, 2, 0);
}

void toggle_password_visibility_event(lv_event_t* event) {
    auto* textarea = static_cast<lv_obj_t*>(lv_event_get_user_data(event));
    if (textarea == nullptr) return;
    const bool visible = lv_textarea_get_password_mode(textarea);
    lv_textarea_set_password_mode(textarea, !visible);
    auto* button = static_cast<lv_obj_t*>(lv_event_get_target(event));
    auto* icon = lv_obj_get_child(button, 0);
    if (icon != nullptr) lv_image_set_src(icon, visible ? &nexus_icon_eye_off : &nexus_icon_eye);
}

void build_password_prompt(lv_obj_t* root, const UiState& state, lv_group_t* group, ViewObjects& objects) {
    constexpr int horizontal_margin = 12;
    constexpr int top_margin = 52;
    constexpr int bottom_margin = 16;
    const int width = lv_obj_get_content_width(root) - horizontal_margin * 2;
    const int height = lv_obj_get_content_height(root) - top_margin - bottom_margin;
    auto* dialog = lv_obj_create(root);
    lv_obj_set_size(dialog, width, height);
    lv_obj_set_pos(dialog, horizontal_margin, top_margin);
    lv_obj_set_style_bg_color(dialog, lv_color_hex(0x111A2C), 0);
    lv_obj_set_style_bg_opa(dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dialog, 1, 0);
    lv_obj_set_style_border_color(dialog, lv_color_hex(0x26334A), 0);
    lv_obj_set_style_radius(dialog, 8, 0);
    lv_obj_set_style_pad_all(dialog, 8, 0);
    lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);
    // Account for the 8 px padding and 1 px border on each side. Reading the
    // content area immediately after creating the object can return stale
    // layout dimensions before LVGL refreshes it.
    const int dialog_content_width = width - 18;
    const int dialog_content_height = height - 18;

    auto* title = create_label(dialog, "Conectar a una red", kText, true);
    lv_obj_set_pos(title, 0, 0);
    auto* network = create_label(dialog, state.wifi_connect_ssid.c_str(), kMuted);
    lv_obj_set_width(network, width - 18);
    lv_obj_set_pos(network, 0, 31);

    auto* password = lv_textarea_create(dialog);
    constexpr int visibility_button_size = 40;
    lv_obj_set_size(password, dialog_content_width - visibility_button_size, 40);
    lv_obj_set_pos(password, 0, 53);
    lv_textarea_set_password_mode(password, true);
    lv_textarea_set_one_line(password, true);
    lv_textarea_set_placeholder_text(password, "Contraseña Wi-Fi");
    lv_textarea_set_max_length(password, 64);
    if (!state.wifi_password.empty()) lv_textarea_set_text(password, state.wifi_password.c_str());
    // LVGL adds text areas to the default group automatically. Keep the
    // field editable by the keyboard, but don't let it trap D-pad focus.
    lv_group_remove_obj(password);

    auto* visibility = lv_button_create(dialog);
    lv_obj_set_size(visibility, visibility_button_size, visibility_button_size);
    lv_obj_set_pos(visibility, dialog_content_width - visibility_button_size, 53);
    // Treat the eye as a trailing control inside the field, using the field's
    // own fill so it doesn't look like a separate, mismatched button.
    lv_obj_set_style_bg_color(visibility, lv_obj_get_style_bg_color(password, LV_PART_MAIN), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(visibility, lv_obj_get_style_bg_opa(password, LV_PART_MAIN), LV_PART_MAIN);
    lv_obj_set_style_border_width(visibility, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(visibility, 8, LV_PART_MAIN);
    components::style_focusable(visibility);
    auto* visibility_icon = lv_image_create(visibility);
    lv_image_set_src(visibility_icon, &nexus_icon_eye);
    lv_obj_set_style_image_recolor(visibility_icon, lv_color_hex(0x5C39F5), LV_PART_MAIN);
    lv_obj_set_style_image_recolor_opa(visibility_icon, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_center(visibility_icon);
    lv_obj_clear_flag(visibility_icon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(visibility, toggle_password_visibility_event, LV_EVENT_CLICKED, password);
    lv_obj_add_event_cb(visibility, UiController::key_event, LV_EVENT_KEY, nullptr);
    if (group != nullptr) lv_group_add_obj(group, visibility);

    auto* keyboard = lv_keyboard_create(dialog);
    lv_keyboard_set_map(keyboard, LV_KEYBOARD_MODE_USER_1, kWifiNumericKeyboardMap,
                        kWifiNumericKeyboardCtrlMap.data());
    // Keep the four-row LVGL keyboard inside its own area. The default map
    // needs room for its bottom space/OK row; letting it grow to the action
    // buttons' top edge makes that row draw underneath the buttons.
    constexpr int keyboard_y = 98;
    constexpr int keyboard_height = 228;
    const int button_y = dialog_content_height - 50;
    lv_obj_set_size(keyboard, dialog_content_width, keyboard_height);
    // lv_keyboard_create() aligns the keyboard to the bottom of its parent.
    // Override that default explicitly so the dialog's compact keymap is
    // positioned in the space reserved above the action buttons.
    lv_obj_align(keyboard, LV_ALIGN_TOP_LEFT, 0, keyboard_y);
    lv_obj_set_style_bg_color(keyboard, lv_color_hex(0x111A2C), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(keyboard, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(keyboard, lv_color_hex(0x26334A), LV_PART_MAIN);
    lv_obj_set_style_border_width(keyboard, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(keyboard, 9, LV_PART_MAIN);
    lv_obj_set_style_pad_all(keyboard, 5, LV_PART_MAIN);
    lv_obj_set_style_bg_color(keyboard, lv_color_hex(0x1A263A), LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(keyboard, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_border_color(keyboard, lv_color_hex(0x34435C), LV_PART_ITEMS);
    lv_obj_set_style_border_width(keyboard, 1, LV_PART_ITEMS);
    lv_obj_set_style_radius(keyboard, 7, LV_PART_ITEMS);
    lv_obj_set_style_text_color(keyboard, lv_color_hex(0xF1F4FC), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(keyboard, lv_color_hex(0x5C39F5),
                              static_cast<lv_style_selector_t>(LV_PART_ITEMS | LV_STATE_FOCUSED));
    lv_obj_set_style_border_color(keyboard, lv_color_hex(0x8A7AFF),
                                  static_cast<lv_style_selector_t>(LV_PART_ITEMS | LV_STATE_FOCUSED));
    lv_obj_set_style_text_color(keyboard, lv_color_hex(0xFFFFFF),
                                static_cast<lv_style_selector_t>(LV_PART_ITEMS | LV_STATE_FOCUSED));
    lv_obj_set_style_bg_color(keyboard, lv_color_hex(0x482BCC),
                              static_cast<lv_style_selector_t>(LV_PART_ITEMS | LV_STATE_PRESSED));
    lv_keyboard_set_textarea(keyboard, password);
    // The default "1#" key enters LVGL's mixed symbols map. Wi-Fi passwords
    // benefit from a dedicated, full-width numeric layout with an obvious
    // "abc" key to return to letters, so handle the keyboard map locally.
    lv_obj_remove_event_cb(keyboard, lv_keyboard_def_event_cb);
    wifi_keyboard_activate_on_press(keyboard);
    lv_obj_add_event_cb(keyboard, wifi_keyboard_event, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(keyboard, UiController::wifi_password_ready_event, LV_EVENT_READY, nullptr);
    lv_obj_add_event_cb(keyboard, UiController::wifi_password_changed_event, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(keyboard, UiController::wifi_password_clicked_event, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(keyboard, UiController::wifi_password_cancel_event, LV_EVENT_CANCEL, nullptr);
    lv_obj_add_event_cb(keyboard, UiController::wifi_keyboard_edge_event,
                        static_cast<lv_event_code_t>(LV_EVENT_KEY | LV_EVENT_PREPROCESS), nullptr);
    lv_obj_add_event_cb(keyboard, UiController::key_event, LV_EVENT_KEY, nullptr);
    if (group != nullptr) lv_group_add_obj(group, keyboard);

    const int button_width = (dialog_content_width - 12) / 2;
    auto* connect = create_action_button(dialog, "Conectar", Action::SubmitWifiPassword, group,
                                          std::max(110, button_width), 38);
    lv_obj_set_pos(connect, 0, button_y);
    auto* cancel = create_action_button(dialog, "Cancelar", Action::CancelWifiPassword, group,
                                         std::max(110, button_width), 38);
    lv_obj_align(cancel, LV_ALIGN_TOP_RIGHT, 0, button_y);
    objects.initial_focus = keyboard;
}

}  // namespace

void build_wifi_content(View view, lv_obj_t* panel, const UiState& state, lv_group_t* group,
                        ViewObjects& objects) {
    if (!state.wifi_connect_ssid.empty()) {
        build_password_prompt(objects.root, state, group, objects);
        return;
    }

    const int content_width = lv_obj_get_content_width(panel);
    if (view == View::Wifi) {
        auto* heading = create_label(panel, "Wi-Fi", kText, true);
        lv_obj_set_pos(heading, 0, 0);
        auto* description = create_label(panel, "Estado y administración de la conexión.", kMuted);
        lv_obj_set_pos(description, 0, 39);

        std::string status_text = state.wifi_status ? wifi_state_text(state.wifi_status->state())
                                                    : "Estado desconocido";
        if (state.wifi_status && state.wifi_status->radio_state_available()) {
            status_text = state.wifi_status->radio_enabled() ? "Radio activada" : "Radio desactivada";
        }
        if (state.wifi_status && state.wifi_status->connected() && !state.wifi_status->ssid().empty()) {
            status_text += "  ·  " + state.wifi_status->ssid();
        }
        auto* connection = create_label(panel, status_text.c_str(),
                                        state.wifi_status && state.wifi_status->connected() ? kGood : kMuted);
        lv_obj_set_width(connection, content_width - 8);
        lv_label_set_long_mode(connection, LV_LABEL_LONG_CLIP);
        lv_obj_set_pos(connection, 0, 72);

        auto* radio_label = create_label(panel, "Radio Wi-Fi", kText);
        lv_obj_set_pos(radio_label, 0, 113);
        const bool radio_available = state.wifi_status && state.wifi_status->radio_state_available();
        auto* toggle = lv_switch_create(panel);
        lv_obj_set_size(toggle, 48, 26);
        lv_obj_align(toggle, LV_ALIGN_TOP_RIGHT, 0, 105);
        lv_obj_set_style_bg_color(toggle, lv_color_hex(0x27334A), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(toggle, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(toggle, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(toggle, 14, LV_PART_MAIN);
        lv_obj_set_style_pad_all(toggle, 2, LV_PART_MAIN);
        lv_obj_set_style_bg_color(toggle, lv_color_hex(0x44D19A), LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(toggle, LV_OPA_COVER, LV_PART_INDICATOR);
        lv_obj_set_style_radius(toggle, 14, LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(toggle, lv_color_hex(0xF1F4FC), LV_PART_KNOB);
        lv_obj_set_style_bg_opa(toggle, LV_OPA_COVER, LV_PART_KNOB);
        lv_obj_set_style_radius(toggle, 12, LV_PART_KNOB);
        if (radio_available && state.wifi_status->radio_enabled()) lv_obj_add_state(toggle, LV_STATE_CHECKED);
        if (!radio_available || state.wifi_status_loading || state.wifi_radio_changing) {
            lv_obj_add_state(toggle, LV_STATE_DISABLED);
        }
        style_focusable(toggle);
        lv_obj_add_event_cb(toggle, UiController::action_event, LV_EVENT_VALUE_CHANGED,
                            reinterpret_cast<void*>(static_cast<std::uintptr_t>(Action::ToggleWifi)));
        lv_obj_add_event_cb(toggle, UiController::key_event, LV_EVENT_KEY, nullptr);
        if (group != nullptr) lv_group_add_obj(group, toggle);

        auto* available = create_action_button(panel, "Redes disponibles", Action::ShowWifiAvailable,
                                               group, std::min(260, content_width), 42);
        lv_obj_set_pos(available, 0, 158);
        auto* saved = create_action_button(panel, "Redes guardadas", Action::ShowWifiSaved,
                                           group, std::min(260, content_width), 42);
        lv_obj_set_pos(saved, 0, 210);
        if (!state.wifi_action_message.empty()) {
            auto* message = create_label(panel, state.wifi_action_message.c_str(), kMuted);
            lv_obj_set_width(message, content_width);
            lv_label_set_long_mode(message, LV_LABEL_LONG_DOT);
            lv_obj_set_pos(message, 0, 266);
        }
        if (!state.wifi_status_error.empty()) {
            auto* error = create_label(panel, state.wifi_status_error.c_str(), kWarn);
            lv_obj_set_width(error, content_width);
            lv_label_set_long_mode(error, LV_LABEL_LONG_DOT);
            lv_obj_set_pos(error, 0, 292);
        }
        objects.initial_focus = available;
        return;
    }

    // Each network collection has its own page. The list panel scrolls only
    // when that collection is longer than the visible area.
    lv_obj_add_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(panel, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(panel, LV_SCROLLBAR_MODE_AUTO);

    const bool available_page = view == View::WifiAvailable;
    auto* heading = create_label(panel, available_page ? "Redes disponibles" : "Redes guardadas", kText, true);
    lv_obj_set_pos(heading, 0, 0);

    auto* refresh = create_action_button(panel,
                                         available_page ? (state.wifi_scan_loading ? "Buscando..." : "Buscar redes")
                                                        : (state.wifi_saved_loading ? "Actualizando..." : "Actualizar"),
                                         available_page ? Action::RefreshWifi : Action::RefreshSavedWifiNetworks,
                                         group, 142, 34);
    lv_obj_set_pos(refresh, 0, 38);
    const bool radio_enabled = !state.wifi_status || !state.wifi_status->radio_state_available() ||
                               state.wifi_status->radio_enabled();
    if (available_page && (!radio_enabled || state.wifi_scan_loading || state.wifi_status_loading)) {
        lv_obj_add_state(refresh, LV_STATE_DISABLED);
    }
    if (!available_page && state.wifi_saved_loading) lv_obj_add_state(refresh, LV_STATE_DISABLED);

    const std::string& error_text = available_page ? state.wifi_scan_error : state.wifi_saved_error;
    if (!error_text.empty()) {
        auto* error = create_label(panel, error_text.c_str(), kWarn);
        lv_obj_set_width(error, content_width);
        lv_label_set_long_mode(error, LV_LABEL_LONG_DOT);
        lv_obj_set_pos(error, 0, 78);
    }
    if (!state.wifi_action_message.empty()) {
        auto* message = create_label(panel, state.wifi_action_message.c_str(), kMuted);
        lv_obj_set_width(message, content_width);
        lv_label_set_long_mode(message, LV_LABEL_LONG_DOT);
        lv_obj_set_pos(message, 0, 101);
    }

    int y = 128;
    if (available_page) {
        std::string list_title = "Redes encontradas";
        if (state.wifi_scan && state.wifi_scan->networks_size() > 0) {
            list_title += " (" + std::to_string(state.wifi_scan->networks_size()) + ")";
        }
        auto* list_heading = create_label(panel, list_title.c_str(), kText, true);
        lv_obj_set_style_text_font(list_heading, &nexus_font_16, 0);
        lv_obj_set_pos(list_heading, 0, y);
        style_wifi_list_heading(list_heading);
        y += 26;
        if (!radio_enabled) {
            auto* disabled = create_label(panel, "Activa Wi-Fi para buscar redes.", kMuted);
            lv_obj_set_pos(disabled, 4, y);
            objects.initial_focus = refresh;
            return;
        }
        if (state.wifi_scan_loading && !state.wifi_scan) {
            auto* loading = create_label(panel, "Buscando redes Wi-Fi...", kMuted);
            lv_obj_set_pos(loading, 4, y);
            objects.initial_focus = refresh;
            return;
        }
        if (!state.wifi_scan || state.wifi_scan->networks_size() == 0) {
            auto* empty = create_label(panel, "No se encontraron redes. Pulsa Buscar redes para reintentar.", kMuted);
            lv_obj_set_width(empty, content_width - 8);
            lv_label_set_long_mode(empty, LV_LABEL_LONG_DOT);
            lv_obj_set_pos(empty, 4, y);
            objects.initial_focus = refresh;
            return;
        }
        bool first_network = true;
        for (const auto& network : state.wifi_scan->networks()) {
            const std::string name = network.ssid().empty() ? "Red oculta" : network.ssid();
            const std::string text = name + "  ·  " + wifi_security_text(network.security()) + "  ·  " +
                                     std::to_string(std::clamp(network.signal_percent(), 0, 100)) + "%";
            auto* button = create_wifi_network_button(
                panel, text.c_str(), group,
                WifiActionContext{Action::ConnectWifiNetwork, {}, name,
                                  network.security().empty() ? "open" : network.security()},
                content_width, y);
            if (first_network) {
                objects.initial_focus = button;
                first_network = false;
            }
            y += 38;
        }
        return;
    }

    if (state.wifi_saved_loading && !state.saved_wifi_networks) {
        auto* loading = create_label(panel, "Cargando redes guardadas...", kMuted);
        lv_obj_set_pos(loading, 4, y);
        objects.initial_focus = refresh;
        return;
    }
    if (!state.saved_wifi_networks || state.saved_wifi_networks->networks_size() == 0) {
        auto* empty = create_label(panel, "Aún no hay redes guardadas.", kMuted);
        lv_obj_set_pos(empty, 4, y);
        objects.initial_focus = refresh;
        return;
    }
    bool first_saved = true;
    for (const auto& network : state.saved_wifi_networks->networks()) {
        auto* row = lv_obj_create(panel);
        lv_obj_set_width(row, content_width - 4);
        lv_obj_set_height(row, 36);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
        lv_obj_set_pos(row, 0, y);
        const std::string name = network.ssid() + (network.connected() ? " · conectada" : "");
        auto* connect = create_wifi_action_button(row, name.c_str(), std::max(100, content_width - 164), 32, group,
                                                  WifiActionContext{Action::ConnectSavedWifiNetwork, network.id()});
        lv_obj_set_pos(connect, 0, 0);
        const std::string preferred_text = network.preferred() ? "Preferida" : "★";
        auto* preferred = create_wifi_action_button(row, preferred_text.c_str(), 76, 32, group,
                                                   WifiActionContext{Action::SetPreferredWifiNetwork, network.id()});
        lv_obj_align(preferred, LV_ALIGN_TOP_RIGHT, -66, 0);
        auto* forget = create_wifi_action_button(row, "Olvidar", 64, 32, group,
                                                 WifiActionContext{Action::ForgetWifiNetwork, network.id()});
        lv_obj_align(forget, LV_ALIGN_TOP_RIGHT, 0, 0);
        if (first_saved) {
            objects.initial_focus = connect;
            first_saved = false;
        }
        y += 38;
    }
}

}  // namespace nexus

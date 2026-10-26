#include "ui/ui.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <climits>
#include <memory>

namespace nexus {
namespace {

bool keyboard_button_is_row(lv_obj_t* keyboard, uint32_t selected, bool last_row) {
    if (keyboard == nullptr || selected == LV_BUTTONMATRIX_BUTTON_NONE) return false;
    const char* const* map = lv_keyboard_get_map_array(keyboard);
    if (map == nullptr) return false;
    uint32_t button_index = 0;
    uint32_t row_index = 0;
    uint32_t selected_row = 0;
    for (uint32_t map_index = 0; map[map_index] != nullptr && map[map_index][0] != '\0'; ++map_index) {
        if (map[map_index][0] == '\n') {
            ++row_index;
            continue;
        }
        if (button_index == selected) selected_row = row_index;
        ++button_index;
    }
    return last_row ? selected_row == row_index : selected_row == 0;
}

bool keyboard_button_is_first_row(lv_obj_t* keyboard, uint32_t selected) {
    return keyboard_button_is_row(keyboard, selected, false);
}

bool keyboard_button_is_last_row(lv_obj_t* keyboard, uint32_t selected) {
    return keyboard_button_is_row(keyboard, selected, true);
}

lv_obj_t* directional_neighbor(lv_group_t* group, lv_obj_t* current, uint32_t key,
                               bool same_parent = false) {
    if (group == nullptr || current == nullptr) return nullptr;
    lv_area_t current_area{};
    lv_obj_get_coords(current, &current_area);
    const int current_x = (current_area.x1 + current_area.x2) / 2;
    const int current_y = (current_area.y1 + current_area.y2) / 2;
    lv_obj_t* best = nullptr;
    int best_score = INT32_MAX;
    const uint32_t count = lv_group_get_obj_count(group);
    for (uint32_t i = 0; i < count; ++i) {
        auto* candidate = lv_group_get_obj_by_index(group, i);
        if (candidate == nullptr || candidate == current || !lv_obj_is_visible(candidate)) continue;
        if (same_parent && lv_obj_get_parent(candidate) != lv_obj_get_parent(current)) continue;
        lv_area_t area{};
        lv_obj_get_coords(candidate, &area);
        const int dx = (area.x1 + area.x2) / 2 - current_x;
        const int dy = (area.y1 + area.y2) / 2 - current_y;
        int primary = 0;
        int secondary = 0;
        if (key == LV_KEY_LEFT || key == LV_KEY_RIGHT) {
            if ((key == LV_KEY_LEFT && dx >= 0) || (key == LV_KEY_RIGHT && dx <= 0) || std::abs(dy) > 64) continue;
            primary = std::abs(dx);
            secondary = std::abs(dy);
        } else {
            if ((key == LV_KEY_UP && dy >= 0) || (key == LV_KEY_DOWN && dy <= 0)) continue;
            primary = std::abs(dy);
            secondary = std::abs(dx);
        }
        const int score = primary * 3 + secondary;
        if (score < best_score) {
            best_score = score;
            best = candidate;
        }
    }
    return best;
}

}  // namespace

UiController* UiController::active_ = nullptr;

void UiController::action_event(lv_event_t* event) {
    if (active_ == nullptr) return;
    lv_async_call(dispatch_action_async, lv_event_get_user_data(event));
}

void UiController::dispatch_action_async(void* user_data) {
    if (active_ == nullptr || active_->stopped_) return;
    const auto action = static_cast<Action>(reinterpret_cast<std::uintptr_t>(user_data));
    active_->dispatch(action);
}

void UiController::dispatch_wifi_action_async(void* user_data) {
    std::unique_ptr<WifiActionContext> context(static_cast<WifiActionContext*>(user_data));
    if (active_ == nullptr || active_->stopped_ || !context) return;
    active_->dispatch_wifi_action(*context);
}

void UiController::wifi_action_event(lv_event_t* event) {
    if (active_ == nullptr || active_->stopped_) return;
    const auto* context = static_cast<const WifiActionContext*>(lv_event_get_user_data(event));
    if (context == nullptr) return;
    lv_async_call(dispatch_wifi_action_async, new WifiActionContext(*context));
}

void UiController::wifi_action_delete_event(lv_event_t* event) {
    delete static_cast<WifiActionContext*>(lv_event_get_user_data(event));
}

void UiController::wifi_password_ready_event(lv_event_t* event) {
    if (active_ == nullptr || active_->stopped_) return;
    auto* keyboard = static_cast<lv_obj_t*>(lv_event_get_target(event));
    auto* textarea = lv_keyboard_get_textarea(keyboard);
    if (textarea == nullptr) return;
    active_->state_.wifi_password = lv_textarea_get_text(textarea);
    lv_async_call(dispatch_action_async,
                  reinterpret_cast<void*>(static_cast<std::uintptr_t>(Action::SubmitWifiPassword)));
}

void UiController::wifi_password_changed_event(lv_event_t* event) {
    if (active_ == nullptr || active_->stopped_) return;
    auto* keyboard = static_cast<lv_obj_t*>(lv_event_get_target(event));
    auto* textarea = lv_keyboard_get_textarea(keyboard);
    if (textarea != nullptr) active_->state_.wifi_password = lv_textarea_get_text(textarea);
    active_->wifi_keyboard_mode_ = lv_keyboard_get_mode(keyboard);

    // Read the selection after the keyboard's map/character handler has run.
    // Mode switches can replace the map and deliberately choose a new key.
    const uint32_t selected = lv_keyboard_get_selected_button(keyboard);
    if (selected != LV_BUTTONMATRIX_BUTTON_NONE) {
        active_->wifi_keyboard_selected_button_ = selected;
        lv_buttonmatrix_set_selected_button(keyboard, selected);
        if (lv_group_get_focused(active_->focus_group_) != keyboard) {
            lv_group_focus_obj(keyboard);
        }
    }
}

void UiController::wifi_password_clicked_event(lv_event_t* event) {
    if (active_ == nullptr || active_->stopped_) return;
    auto* keyboard = static_cast<lv_obj_t*>(lv_event_get_target(event));
    const auto selected = active_->wifi_keyboard_selected_button_;
    if (selected == LV_BUTTONMATRIX_BUTTON_NONE) return;
    lv_buttonmatrix_set_selected_button(keyboard, selected);
    if (lv_group_get_focused(active_->focus_group_) != keyboard) {
        lv_group_focus_obj(keyboard);
    }
}

void UiController::wifi_password_cancel_event(lv_event_t* event) {
    if (active_ == nullptr || active_->stopped_) return;
    lv_async_call(dispatch_action_async,
                  reinterpret_cast<void*>(static_cast<std::uintptr_t>(Action::CancelWifiPassword)));
}

void UiController::wifi_keyboard_edge_event(lv_event_t* event) {
    if (active_ == nullptr || active_->stopped_) return;
    const uint32_t key = lv_event_get_key(event);
    if (key != LV_KEY_UP && key != LV_KEY_DOWN) return;

    auto* keyboard = static_cast<lv_obj_t*>(lv_event_get_target(event));
    const uint32_t selected = lv_keyboard_get_selected_button(keyboard);
    const bool at_edge = key == LV_KEY_UP ? keyboard_button_is_first_row(keyboard, selected)
                                          : keyboard_button_is_last_row(keyboard, selected);
    if (selected != LV_BUTTONMATRIX_BUTTON_NONE) active_->wifi_keyboard_selected_button_ = selected;
    if (!at_edge) return;

    // Only move between the keyboard and its adjacent password/action
    // controls. The settings sidebar and page content share the group but
    // must not be candidates for these keyboard boundary moves.
    if (auto* next = directional_neighbor(active_->focus_group_, keyboard, key, true)) {
        lv_group_focus_obj(next);
        // This callback runs before LVGL's button-matrix handler, so the
        // boundary key cannot also change the keyboard selection.
        lv_event_stop_processing(event);
    }
}

void UiController::navigate_back_async(void* user_data) {
    auto* controller = static_cast<UiController*>(user_data);
    if (controller == nullptr || controller != active_ || controller->stopped_) return;
    controller->navigate_back();
}

void UiController::cycle_section_async(void* user_data) {
    if (active_ == nullptr || active_->stopped_) return;
    const int direction = reinterpret_cast<std::uintptr_t>(user_data) == 1 ? -1 : 1;
    active_->cycle_section(direction);
}

void UiController::key_event(lv_event_t* event) {
    if (active_ == nullptr) return;
    const uint32_t key = lv_event_get_key(event);
    if (key == LV_KEY_ESC) {
        if (!active_->state_.wifi_connect_ssid.empty()) {
            lv_async_call(dispatch_action_async,
                          reinterpret_cast<void*>(static_cast<std::uintptr_t>(Action::CancelWifiPassword)));
            lv_event_stop_bubbling(event);
            return;
        }
        lv_async_call(navigate_back_async, active_);
        return;
    }
    if (key == LV_KEY_HOME || key == LV_KEY_END) {
        lv_async_call(cycle_section_async, reinterpret_cast<void*>(
                                               static_cast<std::uintptr_t>(key == LV_KEY_HOME ? 1 : 2)));
        lv_event_stop_bubbling(event);
        return;
    }
    auto* target = static_cast<lv_obj_t*>(lv_event_get_target(event));
    if (lv_obj_check_type(target, &lv_keyboard_class)) {
        const uint32_t selected = lv_keyboard_get_selected_button(target);
        if (selected != LV_BUTTONMATRIX_BUTTON_NONE) active_->wifi_keyboard_selected_button_ = selected;
        // The button matrix owns normal D-pad movement between keys. Letting
        // the launcher's spatial focus handler see these arrows jumps away
        // from the selected key and restarts navigation at the first key.
        return;
    }
    // Sliders own horizontal keys so left/right continues to adjust their value.
    if ((key == LV_KEY_LEFT || key == LV_KEY_RIGHT) &&
        lv_event_get_user_data(event) == reinterpret_cast<void*>(static_cast<std::uintptr_t>(1))) return;
    if (key != LV_KEY_UP && key != LV_KEY_DOWN && key != LV_KEY_LEFT && key != LV_KEY_RIGHT) return;
    const bool in_wifi_prompt = !active_->state_.wifi_connect_ssid.empty();
    if (auto* next = directional_neighbor(active_->focus_group_, target, key, in_wifi_prompt)) {
        lv_group_focus_obj(next);
    }
    // Directional keys are handled by the launcher's spatial focus navigation.
    // Stop LVGL's widget handler too: a focused switch otherwise interprets a
    // boundary arrow as a value change (for example, turning Wi-Fi off).
    lv_event_stop_processing(event);
}

void UiController::slider_event(lv_event_t* event) {
    if (active_ == nullptr) return;
    const auto action = static_cast<Action>(reinterpret_cast<std::uintptr_t>(lv_event_get_user_data(event)));
    const auto value = static_cast<int>(lv_slider_get_value(static_cast<lv_obj_t*>(lv_event_get_target(event))));
    if (action == Action::ApplyBrightness) active_->state_.brightness_target = value;
    if (action == Action::ApplyVolume) active_->state_.volume_target = value;
}

}  // namespace nexus

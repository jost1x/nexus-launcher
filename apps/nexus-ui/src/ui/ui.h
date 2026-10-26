#pragma once

#include <lvgl.h>

#include <cstdint>
#include <optional>
#include <string>

#include "ui/ui_state.h"

namespace nexus {

class RpcClient;

class UiController {
public:
    explicit UiController(RpcClient& rpc);
    ~UiController();
    UiController(const UiController&) = delete;
    UiController& operator=(const UiController&) = delete;

    void start();
    void dispatch(Action action);
    lv_group_t* focus_group() const { return focus_group_; }
    static void action_event(lv_event_t* event);
    static void key_event(lv_event_t* event);
    static void slider_event(lv_event_t* event);

private:
    struct NavigationEntry {
        View view;
        uint32_t focused_index;
    };

    void navigate(View view);
    void navigate_to(View view, bool remember_current);
    void navigate_back();
    void cycle_section(int direction);
    void render();
    void request_overview();
    void collect_results();
    void update_header();
    static void dispatch_action_async(void* user_data);
    static void navigate_back_async(void* user_data);
    static void cycle_section_async(void* user_data);
    static void timer_event(lv_timer_t* timer);
    static void root_deleted(lv_event_t* event);

    RpcClient& rpc_;
    UiState state_;
    std::string last_header_text_;
    View view_ = View::Home;
    std::optional<NavigationEntry> previous_view_;
    std::optional<uint32_t> restore_focus_index_;
    lv_obj_t* root_ = nullptr;
    lv_obj_t* clock_label_ = nullptr;
    lv_obj_t* wifi_icon_ = nullptr;
    lv_obj_t* battery_icon_ = nullptr;
    lv_obj_t* battery_fill_ = nullptr;
    lv_obj_t* battery_value_ = nullptr;
    lv_group_t* focus_group_ = nullptr;
    lv_timer_t* refresh_timer_ = nullptr;
    uint64_t wifi_status_id_ = 0;
    uint64_t wifi_watch_id_ = 0;
    uint64_t wifi_scan_id_ = 0;
    uint64_t hardware_id_ = 0;
    uint64_t hardware_watch_id_ = 0;
    uint64_t system_id_ = 0;
    uint64_t brightness_id_ = 0;
    uint64_t volume_id_ = 0;
    bool stopped_ = false;
    static UiController* active_;
};

}  // namespace nexus

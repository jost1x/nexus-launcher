#pragma once

#include <lvgl.h>

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

#include "rpc_client.h"

namespace nexus {

enum class View { Home, Wifi, Display, Audio, System, About };
enum class Action { Home = 1, Wifi, Display, Audio, System, About, RefreshWifi, ApplyBrightness, ApplyVolume };

struct UiState {
    std::optional<nexus::v1::WifiServiceGetStatusResponse> wifi_status;
    std::optional<nexus::v1::ScanResponse> wifi_scan;
    std::optional<nexus::v1::HardwareServiceGetStatusResponse> hardware;
    std::optional<nexus::v1::GetInfoResponse> system_info;
    bool wifi_status_loading = false;
    bool wifi_scan_loading = false;
    bool hardware_loading = false;
    bool system_loading = false;
    std::string wifi_status_error;
    std::string wifi_scan_error;
    std::string hardware_error;
    std::string system_error;
    std::string action_message;
    int brightness_target = -1;
    int volume_target = -1;
};

class UiController {
public:
    explicit UiController(RpcClient& rpc);
    ~UiController();
    UiController(const UiController&) = delete;
    UiController& operator=(const UiController&) = delete;

    void start();
    void navigate(View view);
    void dispatch(Action action);
    const UiState& state() const { return state_; }
    lv_group_t* focus_group() const { return focus_group_; }
    static void action_event(lv_event_t* event);
    static void key_event(lv_event_t* event);
    static void slider_event(lv_event_t* event);

private:
    void render();
    void request_overview();
    void collect_results();
    void update_header();
    static void timer_event(lv_timer_t* timer);
    static void root_deleted(lv_event_t* event);

    RpcClient& rpc_;
    UiState state_;
    std::string last_header_text_;
    View view_ = View::Home;
    lv_obj_t* root_ = nullptr;
    lv_obj_t* status_label_ = nullptr;
    lv_group_t* focus_group_ = nullptr;
    lv_indev_t* keyboard_ = nullptr;
    lv_indev_t* controller_ = nullptr;
    lv_timer_t* refresh_timer_ = nullptr;
    uint64_t wifi_status_id_ = 0;
    uint64_t wifi_scan_id_ = 0;
    uint64_t hardware_id_ = 0;
    uint64_t system_id_ = 0;
    uint64_t brightness_id_ = 0;
    uint64_t volume_id_ = 0;
    bool stopped_ = false;
    static UiController* active_;
};

}  // namespace nexus

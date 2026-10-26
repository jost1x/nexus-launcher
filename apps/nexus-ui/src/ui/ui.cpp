#include "ui/ui.h"

#include "rpc/rpc_client.h"
#include "assets/icons/lucide_icons.h"
#include "ui/views/view_helpers.h"
#include "ui/views/views.h"

#include <algorithm>
#include <ctime>
#include <utility>

namespace nexus {

UiController::UiController(RpcClient& rpc) : rpc_(rpc), focus_group_(lv_group_create()) {
    active_ = this;
    lv_group_set_default(focus_group_);
}

UiController::~UiController() {
    stopped_ = true;
    if (refresh_timer_ != nullptr) lv_timer_delete(refresh_timer_);
    for (const auto id : {wifi_status_id_, wifi_watch_id_, wifi_scan_id_, hardware_id_, hardware_watch_id_, system_id_, brightness_id_, volume_id_}) {
        if (id != 0) rpc_.cancel(id);
    }
    if (root_ != nullptr) lv_obj_delete(root_);
    if (focus_group_ != nullptr) lv_group_delete(focus_group_);
    if (active_ == this) active_ = nullptr;
}

void UiController::start() {
    request_overview();
    refresh_timer_ = lv_timer_create(timer_event, 150, this);
    render();
}

void UiController::render() {
    lv_group_remove_all_objs(focus_group_);
    if (root_ != nullptr) {
        lv_obj_delete(root_);
        root_ = nullptr;
    }
    const auto objects = build_view(view_, state_, focus_group_);
    root_ = objects.root;
    clock_label_ = objects.clock;
    wifi_icon_ = objects.wifi_icon;
    battery_icon_ = objects.battery_icon;
    battery_fill_ = objects.battery_fill;
    battery_value_ = objects.battery_value;
    last_header_text_.clear();
    lv_obj_add_event_cb(root_, root_deleted, LV_EVENT_DELETE, this);
    if (objects.initial_focus != nullptr) lv_group_focus_obj(objects.initial_focus);
    update_header();
}

void UiController::update_header() {
    if (clock_label_ == nullptr) return;
    std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_r(&now, &local);
    char clock[8]{};
    std::strftime(clock, sizeof(clock), "%H:%M", &local);
    const bool wifi_known = state_.wifi_status.has_value() && state_.wifi_status_error.empty();
    const bool wifi_connected = wifi_known && state_.wifi_status->connected();
    const bool battery_available = state_.hardware && state_.hardware->battery_available();
    const bool battery_charging = battery_available && state_.hardware->battery_charging();
    const int battery_percent = battery_available
                                    ? std::clamp(state_.hardware->battery_percent(), 0, 100)
                                    : 0;
    const std::string status = std::string(clock) + (wifi_connected ? "|wifi" : (wifi_known ? "|offline" : "|unknown")) +
                               (battery_available ? "|" + std::to_string(battery_percent) : "|none") +
                               (battery_charging ? "|charging" : "|not-charging");
    if (status == last_header_text_) return;

    lv_label_set_text(clock_label_, clock);
    const lv_color_t wifi_color = !wifi_known ? view_helpers::kWarn
                                    : wifi_connected ? view_helpers::kGood
                                                     : view_helpers::kMuted;
    if (wifi_icon_ != nullptr) {
        lv_image_set_src(wifi_icon_, (wifi_connected || !wifi_known) ? &nexus_icon_wifi : &nexus_icon_wifi_off);
        lv_obj_set_style_image_recolor(wifi_icon_, wifi_color, 0);
        lv_obj_set_style_image_recolor_opa(wifi_icon_, LV_OPA_COVER, 0);
    }

    if (battery_available) {
        const int fill_width = battery_charging
                                   ? 0
                                   : (battery_percent == 0 ? 1 : std::max(1, battery_percent * 11 / 100));
        const lv_color_t battery_color = battery_charging
                                             ? view_helpers::kGood
                                             : (battery_percent <= 20 ? view_helpers::kWarn : view_helpers::kGood);
        if (battery_icon_ != nullptr) {
            lv_image_set_src(battery_icon_, battery_charging ? &nexus_icon_battery_charging : &nexus_icon_battery);
            lv_obj_set_style_image_recolor(battery_icon_, battery_charging ? battery_color : view_helpers::kMuted, 0);
            lv_obj_set_style_image_recolor_opa(battery_icon_, LV_OPA_COVER, 0);
        }
        if (battery_fill_ != nullptr) {
            lv_obj_set_width(battery_fill_, fill_width);
            lv_obj_set_style_bg_color(battery_fill_, battery_color, 0);
        }
        if (battery_value_ != nullptr) {
            lv_label_set_text_fmt(battery_value_, "%d%%", battery_percent);
            lv_obj_set_style_text_color(battery_value_,
                                        !battery_charging && battery_percent <= 20 ? view_helpers::kWarn
                                                                                  : view_helpers::kText,
                                        0);
        }
    } else {
        if (battery_icon_ != nullptr) {
            lv_image_set_src(battery_icon_, &nexus_icon_battery);
            lv_obj_set_style_image_recolor(battery_icon_, view_helpers::kMuted, 0);
        }
        if (battery_value_ != nullptr) {
            lv_label_set_text(battery_value_, "--%");
            lv_obj_set_style_text_color(battery_value_, view_helpers::kMuted, 0);
        }
        if (battery_fill_ != nullptr) lv_obj_set_width(battery_fill_, 0);
    }
    last_header_text_ = status;
}

void UiController::timer_event(lv_timer_t* timer) {
    auto* self = static_cast<UiController*>(lv_timer_get_user_data(timer));
    if (self == nullptr || self->stopped_) return;
    if (lv_display_get_default() == nullptr) {
        self->stopped_ = true;
        return;
    }
    self->collect_results();
    self->update_header();
}

void UiController::root_deleted(lv_event_t* event) {
    auto* self = static_cast<UiController*>(lv_event_get_user_data(event));
    if (self == nullptr) return;
    self->root_ = nullptr;
    self->clock_label_ = nullptr;
    self->wifi_icon_ = nullptr;
    self->battery_icon_ = nullptr;
    self->battery_fill_ = nullptr;
    self->battery_value_ = nullptr;
    if (self->focus_group_ != nullptr) lv_group_remove_all_objs(self->focus_group_);
}

}  // namespace nexus

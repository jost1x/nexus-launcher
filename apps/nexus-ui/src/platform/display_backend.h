#pragma once

#include <lvgl.h>

namespace nexus {

class DisplayBackend {
public:
    DisplayBackend();
    ~DisplayBackend();
    DisplayBackend(const DisplayBackend&) = delete;
    DisplayBackend& operator=(const DisplayBackend&) = delete;

    lv_indev_t* keyboard() const { return keyboard_; }
    bool available() const { return lv_display_get_default() != nullptr; }

private:
    lv_indev_t* keyboard_ = nullptr;
#ifdef NEXUS_UI_FBDEV
    int tty_fd_ = -1;
#endif
};

}  // namespace nexus

#include "platform/display_backend.h"

#ifdef NEXUS_UI_FBDEV
#include <src/drivers/display/fb/lv_linux_fbdev.h>
#else
#include <src/drivers/sdl/lv_sdl_keyboard.h>
#include <src/drivers/sdl/lv_sdl_mouse.h>
#include <src/drivers/sdl/lv_sdl_window.h>
#endif

#include <SDL2/SDL.h>

#include <cstdio>
#ifdef NEXUS_UI_FBDEV
#include <fcntl.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace nexus {

DisplayBackend::DisplayBackend() {
#ifdef NEXUS_UI_FBDEV
    tty_fd_ = open("/dev/tty", O_RDWR | O_CLOEXEC);
    if (tty_fd_ < 0) {
        std::perror("open /dev/tty");
    } else if (ioctl(tty_fd_, KDSETMODE, KD_GRAPHICS) != 0) {
        std::perror("ioctl(KD_GRAPHICS)");
        close(tty_fd_);
        tty_fd_ = -1;
    }
#endif
    lv_init();
#ifdef NEXUS_UI_FBDEV
    auto* display = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(display, "/dev/fb0");
#else
    lv_sdl_window_create(640, 480);
    lv_sdl_mouse_create();
    keyboard_ = lv_sdl_keyboard_create();
#endif
}

DisplayBackend::~DisplayBackend() {
#ifndef NEXUS_UI_FBDEV
    lv_sdl_quit();
#endif
    lv_deinit();
#ifdef NEXUS_UI_FBDEV
    if (tty_fd_ >= 0) {
        ioctl(tty_fd_, KDSETMODE, KD_TEXT);
        close(tty_fd_);
    }
#endif
}

}  // namespace nexus

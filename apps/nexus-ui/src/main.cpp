#include <lvgl.h>
#ifdef NEXUS_UI_FBDEV
#include <src/drivers/display/fb/lv_linux_fbdev.h>
#else
#include <src/drivers/sdl/lv_sdl_keyboard.h>
#include <src/drivers/sdl/lv_sdl_mouse.h>
#include <src/drivers/sdl/lv_sdl_window.h>
#endif
#include <SDL2/SDL.h>

#include "ui.h"

#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstring>
#ifdef NEXUS_UI_FBDEV
#include <fcntl.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace {

constexpr int kWidth = 640;
constexpr int kHeight = 480;
constexpr size_t kLvglLowMemoryBytes = 48 * 1024;
constexpr size_t kLvglCriticalMemoryBytes = 16 * 1024;
volatile std::sig_atomic_t stop_requested = 0;

void request_stop(int) { stop_requested = 1; }

void report_lvgl_memory_if_low(uint8_t& previous_level) {
    lv_mem_monitor_t monitor{};
    lv_mem_monitor(&monitor);

    const uint8_t level = monitor.free_size <= kLvglCriticalMemoryBytes
                              ? 2
                              : monitor.free_size <= kLvglLowMemoryBytes ? 1 : 0;
    if (level == previous_level) return;

    if (level == 2) {
        std::fprintf(stderr,
                     "[nexus-ui] LVGL heap critical: free=%zu/%zu bytes, largest block=%zu, peak used=%zu bytes\n",
                     monitor.free_size, monitor.total_size, monitor.free_biggest_size, monitor.max_used);
        std::fflush(stderr);
    } else if (level == 1) {
        std::fprintf(stderr,
                     "[nexus-ui] LVGL heap low: free=%zu/%zu bytes, largest block=%zu, peak used=%zu bytes\n",
                     monitor.free_size, monitor.total_size, monitor.free_biggest_size, monitor.max_used);
        std::fflush(stderr);
    } else if (previous_level != 0) {
        std::fprintf(stderr, "[nexus-ui] LVGL heap recovered: free=%zu/%zu bytes\n",
                     monitor.free_size, monitor.total_size);
        std::fflush(stderr);
    }

    previous_level = level;
}

#ifdef NEXUS_UI_FBDEV
class ConsoleGraphicsMode {
public:
    ConsoleGraphicsMode() : tty_fd_(open("/dev/tty", O_RDWR | O_CLOEXEC)) {
        if (tty_fd_ < 0) {
            std::perror("open /dev/tty");
        } else if (ioctl(tty_fd_, KDSETMODE, KD_GRAPHICS) != 0) {
            std::perror("ioctl(KD_GRAPHICS)");
            close(tty_fd_);
            tty_fd_ = -1;
        }
    }

    ~ConsoleGraphicsMode() {
        if (tty_fd_ >= 0) {
            ioctl(tty_fd_, KDSETMODE, KD_TEXT);
            close(tty_fd_);
        }
    }

private:
    int tty_fd_;
};
#endif

struct ControllerInput {
    SDL_GameController* device = nullptr;
    SDL_Joystick* r36s_keys = nullptr;
};

void open_controller_devices(ControllerInput& input) {
    if (input.device != nullptr && !SDL_GameControllerGetAttached(input.device)) {
        SDL_GameControllerClose(input.device);
        input.device = nullptr;
    }
    if (input.r36s_keys != nullptr && !SDL_JoystickGetAttached(input.r36s_keys)) {
        SDL_JoystickClose(input.r36s_keys);
        input.r36s_keys = nullptr;
    }

    for (int index = 0; index < SDL_NumJoysticks(); ++index) {
        if (input.device == nullptr && SDL_IsGameController(index)) {
            input.device = SDL_GameControllerOpen(index);
        }

        const char* name = SDL_JoystickNameForIndex(index);
        if (input.r36s_keys == nullptr && name != nullptr && std::strcmp(name, "gpio-keys") == 0) {
            input.r36s_keys = SDL_JoystickOpen(index);
        }
    }
}

void read_game_controller(lv_indev_t* indev, lv_indev_data_t* data) {
    auto* input = static_cast<ControllerInput*>(lv_indev_get_user_data(indev));
    if (input == nullptr) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }
    open_controller_devices(*input);
    SDL_GameControllerUpdate();
    SDL_JoystickUpdate();
    struct Mapping { SDL_GameControllerButton button; uint32_t key; };
    constexpr Mapping mappings[] = {
        {SDL_CONTROLLER_BUTTON_DPAD_UP, LV_KEY_PREV},
        {SDL_CONTROLLER_BUTTON_DPAD_DOWN, LV_KEY_NEXT},
        {SDL_CONTROLLER_BUTTON_DPAD_LEFT, LV_KEY_LEFT},
        {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, LV_KEY_RIGHT},
        {SDL_CONTROLLER_BUTTON_A, LV_KEY_ENTER},
        {SDL_CONTROLLER_BUTTON_B, LV_KEY_ESC},
    };
    if (input->device != nullptr) {
        for (const auto& mapping : mappings) {
            if (SDL_GameControllerGetButton(input->device, mapping.button) != 0) {
                data->key = mapping.key;
                data->state = LV_INDEV_STATE_PRESSED;
                return;
            }
        }
    }
    if (input->r36s_keys != nullptr) {
        struct RawMapping { uint8_t button; uint32_t key; };
        // SDL 2.26.5 enumerates gpio-keys by Linux event key code, not by the
        // /dev/input/js0 button map (whose left/right indices are reversed).
        constexpr RawMapping r36s_mappings[] = {
            {8, LV_KEY_PREV}, {9, LV_KEY_NEXT}, {10, LV_KEY_LEFT}, {11, LV_KEY_RIGHT},
            {0, LV_KEY_ENTER}, {1, LV_KEY_ESC},
        };
        for (const auto& mapping : r36s_mappings) {
            if (SDL_JoystickGetButton(input->r36s_keys, mapping.button) != 0) {
                data->key = mapping.key;
                data->state = LV_INDEV_STATE_PRESSED;
                return;
            }
        }
    }
    data->state = LV_INDEV_STATE_RELEASED;
}

}  // namespace

int main() {
#ifdef NEXUS_UI_FBDEV
    ConsoleGraphicsMode console_graphics_mode;
#endif
    std::signal(SIGINT, request_stop);
    std::signal(SIGTERM, request_stop);
    lv_init();
#ifdef NEXUS_UI_FBDEV
    auto* display = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(display, "/dev/fb0");
#else
    lv_sdl_window_create(kWidth, kHeight);
    lv_sdl_mouse_create();
    auto* keyboard = lv_sdl_keyboard_create();
#endif
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    SDL_GameControllerEventState(SDL_ENABLE);

    {
        nexus::RpcClient rpc(nexus::default_socket_path());
        nexus::UiController ui(rpc);
#ifndef NEXUS_UI_FBDEV
        lv_indev_set_group(keyboard, ui.focus_group());
#endif

        ControllerInput controller_input;
        auto* controller = lv_indev_create();
        lv_indev_set_type(controller, LV_INDEV_TYPE_KEYPAD);
        lv_indev_set_read_cb(controller, read_game_controller);
        lv_indev_set_user_data(controller, &controller_input);
        lv_indev_set_group(controller, ui.focus_group());

        ui.start();
        auto last_memory_check = std::chrono::steady_clock::now();
        uint8_t reported_memory_level = 0;
        while (lv_display_get_default() != nullptr && !stop_requested) {
            lv_timer_handler();
            const auto now = std::chrono::steady_clock::now();
            if (now - last_memory_check >= std::chrono::seconds(5)) {
                report_lvgl_memory_if_low(reported_memory_level);
                last_memory_check = now;
            }
            SDL_Delay(5);
        }

        if (controller_input.device != nullptr) SDL_GameControllerClose(controller_input.device);
        if (controller_input.r36s_keys != nullptr) SDL_JoystickClose(controller_input.r36s_keys);
    }
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
#ifndef NEXUS_UI_FBDEV
    lv_sdl_quit();
#endif
    lv_deinit();
    return 0;
}

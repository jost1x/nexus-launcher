#include <SDL2/SDL.h>
#include <lvgl.h>

#include "platform/controller_input.h"
#include "platform/display_backend.h"
#include "platform/memory_monitor.h"
#include "rpc/rpc_client.h"
#include "ui/ui.h"

#include <csignal>

namespace {

volatile std::sig_atomic_t stop_requested = 0;

void request_stop(int) { stop_requested = 1; }

}  // namespace

int main() {
    std::signal(SIGINT, request_stop);
    std::signal(SIGTERM, request_stop);

    nexus::DisplayBackend display;
    nexus::RpcClient rpc(nexus::default_socket_path());
    nexus::UiController ui(rpc);
    nexus::ControllerInput controller(ui.focus_group());
    nexus::MemoryMonitor memory_monitor;

    if (display.keyboard() != nullptr) lv_indev_set_group(display.keyboard(), ui.focus_group());
    ui.start();

    while (display.available() && !stop_requested) {
        lv_timer_handler();
        memory_monitor.poll();
        SDL_Delay(5);
    }
    return 0;
}

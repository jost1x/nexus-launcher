#pragma once

#include <SDL2/SDL.h>
#include <lvgl.h>

namespace nexus {

class ControllerInput {
public:
    explicit ControllerInput(lv_group_t* focus_group);
    ~ControllerInput();
    ControllerInput(const ControllerInput&) = delete;
    ControllerInput& operator=(const ControllerInput&) = delete;

private:
    void open_devices();
    static void read(lv_indev_t* indev, lv_indev_data_t* data);

    SDL_GameController* controller_ = nullptr;
    SDL_Joystick* r36s_keys_ = nullptr;
    lv_indev_t* indev_ = nullptr;
};

}  // namespace nexus

#include "platform/controller_input.h"

#include <cstring>

namespace nexus {

ControllerInput::ControllerInput(lv_group_t* focus_group) {
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    SDL_GameControllerEventState(SDL_ENABLE);

    indev_ = lv_indev_create();
    lv_indev_set_type(indev_, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(indev_, read);
    lv_indev_set_user_data(indev_, this);
    lv_indev_set_group(indev_, focus_group);
}

ControllerInput::~ControllerInput() {
    if (indev_ != nullptr) lv_indev_delete(indev_);
    if (controller_ != nullptr) SDL_GameControllerClose(controller_);
    if (r36s_keys_ != nullptr) SDL_JoystickClose(r36s_keys_);
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}

void ControllerInput::open_devices() {
    if (controller_ != nullptr && !SDL_GameControllerGetAttached(controller_)) {
        SDL_GameControllerClose(controller_);
        controller_ = nullptr;
    }
    if (r36s_keys_ != nullptr && !SDL_JoystickGetAttached(r36s_keys_)) {
        SDL_JoystickClose(r36s_keys_);
        r36s_keys_ = nullptr;
    }

    for (int index = 0; index < SDL_NumJoysticks(); ++index) {
        const char* name = SDL_JoystickNameForIndex(index);
        const bool is_r36s_keys = name != nullptr && std::strcmp(name, "gpio-keys") == 0;
        if (is_r36s_keys) {
            if (r36s_keys_ == nullptr) r36s_keys_ = SDL_JoystickOpen(index);
            continue;
        }
        if (controller_ == nullptr && SDL_IsGameController(index)) {
            controller_ = SDL_GameControllerOpen(index);
        }
    }
}

void ControllerInput::read(lv_indev_t* indev, lv_indev_data_t* data) {
    auto* self = static_cast<ControllerInput*>(lv_indev_get_user_data(indev));
    if (self == nullptr) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }
    self->open_devices();
    SDL_GameControllerUpdate();
    SDL_JoystickUpdate();

    struct Mapping { SDL_GameControllerButton button; uint32_t key; };
    constexpr Mapping mappings[] = {
        {SDL_CONTROLLER_BUTTON_DPAD_UP, LV_KEY_UP},
        {SDL_CONTROLLER_BUTTON_DPAD_DOWN, LV_KEY_DOWN},
        {SDL_CONTROLLER_BUTTON_DPAD_LEFT, LV_KEY_LEFT},
        {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, LV_KEY_RIGHT},
        {SDL_CONTROLLER_BUTTON_A, LV_KEY_ENTER},
        {SDL_CONTROLLER_BUTTON_B, LV_KEY_ESC},
        {SDL_CONTROLLER_BUTTON_LEFTSHOULDER, LV_KEY_HOME},
        {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, LV_KEY_END},
    };
    if (self->controller_ != nullptr) {
        for (const auto& mapping : mappings) {
            if (SDL_GameControllerGetButton(self->controller_, mapping.button) != 0) {
                data->key = mapping.key;
                data->state = LV_INDEV_STATE_PRESSED;
                return;
            }
        }
    }
    if (self->r36s_keys_ != nullptr) {
        struct RawMapping { uint8_t button; uint32_t key; };
        // SDL 2.26.5 enumerates gpio-keys by Linux event key code, not the
        // /dev/input/js0 button map (whose left/right indices are reversed).
        constexpr RawMapping r36s_mappings[] = {
            {8, LV_KEY_UP}, {9, LV_KEY_DOWN}, {10, LV_KEY_LEFT}, {11, LV_KEY_RIGHT},
            // On the R36S gpio-keys device, the reported A/B indices are
            // opposite the expected confirm/back actions.
            {0, LV_KEY_ESC}, {1, LV_KEY_ENTER},
            {4, LV_KEY_HOME}, {5, LV_KEY_END},
        };
        for (const auto& mapping : r36s_mappings) {
            if (SDL_JoystickGetButton(self->r36s_keys_, mapping.button) != 0) {
                data->key = mapping.key;
                data->state = LV_INDEV_STATE_PRESSED;
                return;
            }
        }
    }
    data->state = LV_INDEV_STATE_RELEASED;
}

}  // namespace nexus

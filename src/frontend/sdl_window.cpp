#include "frontend/sdl_window.hpp"

#include <SDL.h>
#include <SDL_opengl.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

namespace vanguard8::frontend {

namespace {

[[nodiscard]] auto gamepad_button_name(const std::uint8_t button) -> std::string {
    const auto button_name = SDL_GameControllerGetStringForButton(
        static_cast<SDL_GameControllerButton>(button)
    );
    return button_name == nullptr ? std::string{} : std::string(button_name);
}

[[nodiscard]] auto key_name(const SDL_KeyboardEvent& event) -> std::string {
    const auto* scancode_name = SDL_GetScancodeName(event.keysym.scancode);
    return scancode_name == nullptr ? std::string{} : std::string(scancode_name);
}

[[nodiscard]] auto find_free_controller_slot(const std::array<void*, 2>& controllers) -> std::size_t {
    for (std::size_t index = 0; index < controllers.size(); ++index) {
        if (controllers[index] == nullptr) {
            return index;
        }
    }
    return controllers.size();
}

}  // namespace

void show_frontend_error_dialog(const std::string_view title, const std::string_view message) {
    const auto prior_init = SDL_WasInit(0);
    bool started_sdl = false;
    bool started_video_subsystem = false;

    if ((prior_init & SDL_INIT_VIDEO) == 0U) {
        if (prior_init == 0U) {
            if (SDL_Init(SDL_INIT_VIDEO) != 0) {
                return;
            }
            started_sdl = true;
        } else if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
            return;
        } else {
            started_video_subsystem = true;
        }
    }

    const std::string title_string(title);
    const std::string message_string(message);
    (void)SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR,
        title_string.c_str(),
        message_string.c_str(),
        nullptr
    );

    if (started_video_subsystem) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }
    if (started_sdl) {
        SDL_Quit();
    }
}

SdlWindowHost::~SdlWindowHost() { shutdown(); }

auto SdlWindowHost::create(const WindowConfig& config, std::string& error) -> bool {
    shutdown();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        error = SDL_GetError();
        return false;
    }
    sdl_initialized_ = true;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    auto flags = static_cast<Uint32>(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (config.fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }
    if (config.hidden) {
        flags |= SDL_WINDOW_HIDDEN;
    }

    const auto width = config.logical_width * config.scale;
    const auto height = config.logical_height * config.scale;
    window_ = SDL_CreateWindow(
        config.title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        flags
    );
    if (window_ == nullptr) {
        error = SDL_GetError();
        shutdown();
        return false;
    }

    gl_context_ = SDL_GL_CreateContext(static_cast<SDL_Window*>(window_));
    if (gl_context_ == nullptr) {
        error = SDL_GetError();
        shutdown();
        return false;
    }

    SDL_GL_SetSwapInterval(1);
    SDL_GameControllerEventState(SDL_ENABLE);
    const auto joystick_count = SDL_NumJoysticks();
    for (int device_index = 0; device_index < joystick_count; ++device_index) {
        open_game_controller(device_index);
    }
    return true;
}

void SdlWindowHost::drawable_size(int& width, int& height) const {
    width = 0;
    height = 0;
    if (window_ == nullptr) {
        return;
    }
    SDL_GL_GetDrawableSize(static_cast<SDL_Window*>(window_), &width, &height);
}

void SdlWindowHost::pump_events(std::vector<RuntimeEvent>& events) {
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
        case SDL_QUIT:
            events.push_back(RuntimeEvent{.type = RuntimeEventType::quit});
            break;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                events.push_back(RuntimeEvent{.type = RuntimeEventType::quit});
            }
            break;
        case SDL_KEYDOWN:
            if (event.key.repeat == 0) {
                events.push_back(RuntimeEvent{
                    .type = RuntimeEventType::key_down,
                    .control_name = key_name(event.key),
                });
            }
            break;
        case SDL_KEYUP:
            events.push_back(RuntimeEvent{
                .type = RuntimeEventType::key_up,
                .control_name = key_name(event.key),
            });
            break;
        case SDL_CONTROLLERDEVICEADDED:
            open_game_controller(event.cdevice.which);
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            close_game_controller(event.cdevice.which);
            break;
        case SDL_CONTROLLERBUTTONDOWN:
            if (const auto gamepad_index = gamepad_index_for_instance(event.cbutton.which); gamepad_index >= 0) {
                events.push_back(RuntimeEvent{
                    .type = RuntimeEventType::gamepad_button_down,
                    .control_name = gamepad_button_name(event.cbutton.button),
                    .gamepad_index = gamepad_index,
                });
            }
            break;
        case SDL_CONTROLLERBUTTONUP:
            if (const auto gamepad_index = gamepad_index_for_instance(event.cbutton.which); gamepad_index >= 0) {
                events.push_back(RuntimeEvent{
                    .type = RuntimeEventType::gamepad_button_up,
                    .control_name = gamepad_button_name(event.cbutton.button),
                    .gamepad_index = gamepad_index,
                });
            }
            break;
        default:
            break;
        }
    }
}

void SdlWindowHost::set_title(const std::string_view title) {
    if (window_ == nullptr) {
        return;
    }
    const std::string title_string(title);
    SDL_SetWindowTitle(static_cast<SDL_Window*>(window_), title_string.c_str());
}

void SdlWindowHost::set_fullscreen(const bool fullscreen) {
    if (window_ == nullptr) {
        return;
    }
    const auto flags = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0U;
    (void)SDL_SetWindowFullscreen(static_cast<SDL_Window*>(window_), flags);
}

void SdlWindowHost::present() {
    if (window_ == nullptr || gl_context_ == nullptr) {
        return;
    }
    SDL_GL_SwapWindow(static_cast<SDL_Window*>(window_));
}

void SdlWindowHost::shutdown() {
    for (std::size_t index = 0; index < controllers_.size(); ++index) {
        if (controllers_[index] != nullptr) {
            SDL_GameControllerClose(static_cast<SDL_GameController*>(controllers_[index]));
            controllers_[index] = nullptr;
            controller_instance_ids_[index] = -1;
        }
    }
    if (gl_context_ != nullptr) {
        SDL_GL_DeleteContext(gl_context_);
        gl_context_ = nullptr;
    }
    if (window_ != nullptr) {
        SDL_DestroyWindow(static_cast<SDL_Window*>(window_));
        window_ = nullptr;
    }
    if (sdl_initialized_) {
        SDL_Quit();
        sdl_initialized_ = false;
    }
}

void SdlWindowHost::open_game_controller(const int device_index) {
    if (find_free_controller_slot(controllers_) >= controllers_.size()) {
        return;
    }
    if (SDL_IsGameController(device_index) == SDL_FALSE) {
        return;
    }

    auto* controller = SDL_GameControllerOpen(device_index);
    if (controller == nullptr) {
        return;
    }

    auto* joystick = SDL_GameControllerGetJoystick(controller);
    if (joystick == nullptr) {
        SDL_GameControllerClose(controller);
        return;
    }

    const auto instance_id = static_cast<int>(SDL_JoystickInstanceID(joystick));
    if (gamepad_index_for_instance(instance_id) >= 0) {
        SDL_GameControllerClose(controller);
        return;
    }

    const auto slot = find_free_controller_slot(controllers_);
    if (slot >= controllers_.size()) {
        SDL_GameControllerClose(controller);
        return;
    }

    controllers_[slot] = controller;
    controller_instance_ids_[slot] = instance_id;
}

void SdlWindowHost::close_game_controller(const int instance_id) {
    const auto gamepad_index = gamepad_index_for_instance(instance_id);
    if (gamepad_index < 0) {
        return;
    }

    SDL_GameControllerClose(static_cast<SDL_GameController*>(controllers_[static_cast<std::size_t>(gamepad_index)]));
    controllers_[static_cast<std::size_t>(gamepad_index)] = nullptr;
    controller_instance_ids_[static_cast<std::size_t>(gamepad_index)] = -1;
}

auto SdlWindowHost::gamepad_index_for_instance(const int instance_id) const -> int {
    const auto found = std::find(controller_instance_ids_.begin(), controller_instance_ids_.end(), instance_id);
    if (found == controller_instance_ids_.end()) {
        return -1;
    }
    return static_cast<int>(std::distance(controller_instance_ids_.begin(), found));
}

}  // namespace vanguard8::frontend

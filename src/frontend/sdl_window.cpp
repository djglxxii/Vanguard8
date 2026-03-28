#include "frontend/sdl_window.hpp"

#include <SDL.h>
#include <SDL_opengl.h>

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

}  // namespace

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
    return true;
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
        case SDL_CONTROLLERBUTTONDOWN:
            events.push_back(RuntimeEvent{
                .type = RuntimeEventType::gamepad_button_down,
                .control_name = gamepad_button_name(event.cbutton.button),
                .gamepad_index = event.cbutton.which == 0 ? 0 : 1,
            });
            break;
        case SDL_CONTROLLERBUTTONUP:
            events.push_back(RuntimeEvent{
                .type = RuntimeEventType::gamepad_button_up,
                .control_name = gamepad_button_name(event.cbutton.button),
                .gamepad_index = event.cbutton.which == 0 ? 0 : 1,
            });
            break;
        default:
            break;
        }
    }
}

void SdlWindowHost::present() {
    if (window_ == nullptr || gl_context_ == nullptr) {
        return;
    }

    int drawable_width = 0;
    int drawable_height = 0;
    SDL_GL_GetDrawableSize(static_cast<SDL_Window*>(window_), &drawable_width, &drawable_height);
    glViewport(0, 0, drawable_width, drawable_height);
    glClearColor(0.07F, 0.07F, 0.09F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(static_cast<SDL_Window*>(window_));
}

void SdlWindowHost::shutdown() {
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

}  // namespace vanguard8::frontend

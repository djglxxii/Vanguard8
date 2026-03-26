#pragma once

#include "core/config.hpp"
#include "core/io/controller.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace vanguard8::frontend {

class InputManager {
  public:
    struct BindingTarget {
        core::io::Player player = core::io::Player::one;
        core::io::Button button = core::io::Button::up;
    };

    explicit InputManager(core::io::ControllerPorts& controller_ports);

    void configure(const core::InputBindings& bindings);
    void reset();

    [[nodiscard]] auto press_key(std::string_view key_name) -> bool;
    [[nodiscard]] auto release_key(std::string_view key_name) -> bool;
    [[nodiscard]] auto press_gamepad_button(int gamepad_index, std::string_view button_name) -> bool;
    [[nodiscard]] auto release_gamepad_button(int gamepad_index, std::string_view button_name) -> bool;

  private:
    core::io::ControllerPorts& controller_ports_;
    std::vector<std::pair<std::string, BindingTarget>> keyboard_bindings_;
    std::vector<std::pair<std::string, core::io::Button>> gamepad_bindings_;

    [[nodiscard]] static auto normalize_name(std::string_view name) -> std::string;
    [[nodiscard]] auto apply_keyboard(std::string_view key_name, bool pressed) -> bool;
    [[nodiscard]] auto apply_gamepad(int gamepad_index, std::string_view button_name, bool pressed) -> bool;
};

}  // namespace vanguard8::frontend

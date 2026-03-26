#include "core/io/controller.hpp"

namespace vanguard8::core::io {

void ControllerPorts::reset() { port_state_ = {released_state, released_state}; }

void ControllerPorts::set_button(const Player player, const Button button, const bool pressed) {
    auto& state = port_state_[static_cast<std::size_t>(player)];
    const auto mask = static_cast<std::uint8_t>(1U << static_cast<std::uint8_t>(button));
    if (pressed) {
        state = static_cast<std::uint8_t>(state & ~mask);
        return;
    }
    state = static_cast<std::uint8_t>(state | mask);
}

auto ControllerPorts::read(const Player player) const -> std::uint8_t {
    return port_state_[static_cast<std::size_t>(player)];
}

auto ControllerPorts::read_port(const std::uint16_t port) const -> std::uint8_t {
    switch (port) {
    case 0x00:
        return read(Player::one);
    case 0x01:
        return read(Player::two);
    default:
        return released_state;
    }
}

auto ControllerPorts::player_name(const Player player) -> std::string_view {
    switch (player) {
    case Player::one:
        return "p1";
    case Player::two:
        return "p2";
    }
    return "p1";
}

auto ControllerPorts::button_name(const Button button) -> std::string_view {
    switch (button) {
    case Button::up:
        return "up";
    case Button::down:
        return "down";
    case Button::left:
        return "left";
    case Button::right:
        return "right";
    case Button::a:
        return "a";
    case Button::b:
        return "b";
    case Button::select:
        return "select";
    case Button::start:
        return "start";
    }
    return "up";
}

}  // namespace vanguard8::core::io

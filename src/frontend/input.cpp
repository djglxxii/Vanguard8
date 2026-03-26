#include "frontend/input.hpp"

#include <algorithm>
#include <cctype>

namespace vanguard8::frontend {

namespace {

void add_keyboard_binding(
    std::vector<std::pair<std::string, InputManager::BindingTarget>>& bindings,
    std::string key_name,
    const core::io::Player player,
    const core::io::Button button
) {
    bindings.emplace_back(std::move(key_name), InputManager::BindingTarget{.player = player, .button = button});
}

void add_gamepad_binding(
    std::vector<std::pair<std::string, core::io::Button>>& bindings,
    std::string button_name,
    const core::io::Button button
) {
    bindings.emplace_back(std::move(button_name), button);
}

}  // namespace

InputManager::InputManager(core::io::ControllerPorts& controller_ports) : controller_ports_(controller_ports) {}

void InputManager::configure(const core::InputBindings& bindings) {
    keyboard_bindings_.clear();
    gamepad_bindings_.clear();

    add_keyboard_binding(keyboard_bindings_, normalize_name(bindings.p1_up), core::io::Player::one, core::io::Button::up);
    add_keyboard_binding(keyboard_bindings_, normalize_name(bindings.p1_down), core::io::Player::one, core::io::Button::down);
    add_keyboard_binding(keyboard_bindings_, normalize_name(bindings.p1_left), core::io::Player::one, core::io::Button::left);
    add_keyboard_binding(keyboard_bindings_, normalize_name(bindings.p1_right), core::io::Player::one, core::io::Button::right);
    add_keyboard_binding(keyboard_bindings_, normalize_name(bindings.p1_a), core::io::Player::one, core::io::Button::a);
    add_keyboard_binding(keyboard_bindings_, normalize_name(bindings.p1_b), core::io::Player::one, core::io::Button::b);
    add_keyboard_binding(keyboard_bindings_, normalize_name(bindings.p1_select), core::io::Player::one, core::io::Button::select);
    add_keyboard_binding(keyboard_bindings_, normalize_name(bindings.p1_start), core::io::Player::one, core::io::Button::start);

    add_keyboard_binding(keyboard_bindings_, normalize_name(bindings.p2_up), core::io::Player::two, core::io::Button::up);
    add_keyboard_binding(keyboard_bindings_, normalize_name(bindings.p2_down), core::io::Player::two, core::io::Button::down);
    add_keyboard_binding(keyboard_bindings_, normalize_name(bindings.p2_left), core::io::Player::two, core::io::Button::left);
    add_keyboard_binding(keyboard_bindings_, normalize_name(bindings.p2_right), core::io::Player::two, core::io::Button::right);
    add_keyboard_binding(keyboard_bindings_, normalize_name(bindings.p2_a), core::io::Player::two, core::io::Button::a);
    add_keyboard_binding(keyboard_bindings_, normalize_name(bindings.p2_b), core::io::Player::two, core::io::Button::b);
    add_keyboard_binding(keyboard_bindings_, normalize_name(bindings.p2_select), core::io::Player::two, core::io::Button::select);
    add_keyboard_binding(keyboard_bindings_, normalize_name(bindings.p2_start), core::io::Player::two, core::io::Button::start);

    add_gamepad_binding(gamepad_bindings_, normalize_name(bindings.gamepad_up), core::io::Button::up);
    add_gamepad_binding(gamepad_bindings_, normalize_name(bindings.gamepad_down), core::io::Button::down);
    add_gamepad_binding(gamepad_bindings_, normalize_name(bindings.gamepad_left), core::io::Button::left);
    add_gamepad_binding(gamepad_bindings_, normalize_name(bindings.gamepad_right), core::io::Button::right);
    add_gamepad_binding(gamepad_bindings_, normalize_name(bindings.gamepad_a), core::io::Button::a);
    add_gamepad_binding(gamepad_bindings_, normalize_name(bindings.gamepad_b), core::io::Button::b);
    add_gamepad_binding(gamepad_bindings_, normalize_name(bindings.gamepad_select), core::io::Button::select);
    add_gamepad_binding(gamepad_bindings_, normalize_name(bindings.gamepad_start), core::io::Button::start);
}

void InputManager::reset() { controller_ports_.reset(); }

auto InputManager::press_key(const std::string_view key_name) -> bool { return apply_keyboard(key_name, true); }

auto InputManager::release_key(const std::string_view key_name) -> bool {
    return apply_keyboard(key_name, false);
}

auto InputManager::press_gamepad_button(const int gamepad_index, const std::string_view button_name) -> bool {
    return apply_gamepad(gamepad_index, button_name, true);
}

auto InputManager::release_gamepad_button(const int gamepad_index, const std::string_view button_name) -> bool {
    return apply_gamepad(gamepad_index, button_name, false);
}

auto InputManager::normalize_name(const std::string_view name) -> std::string {
    std::string normalized;
    normalized.reserve(name.size());
    for (const auto ch : name) {
        if (!std::isspace(static_cast<unsigned char>(ch)) && ch != '_') {
            normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        }
    }
    return normalized;
}

auto InputManager::apply_keyboard(const std::string_view key_name, const bool pressed) -> bool {
    const auto normalized = normalize_name(key_name);
    bool matched = false;
    for (const auto& [binding, target] : keyboard_bindings_) {
        if (binding != normalized) {
            continue;
        }
        controller_ports_.set_button(target.player, target.button, pressed);
        matched = true;
    }
    return matched;
}

auto InputManager::apply_gamepad(const int gamepad_index, const std::string_view button_name, const bool pressed)
    -> bool {
    const auto normalized = normalize_name(button_name);
    const auto player = gamepad_index == 0 ? core::io::Player::one : core::io::Player::two;
    bool matched = false;
    for (const auto& [binding, button] : gamepad_bindings_) {
        if (binding != normalized) {
            continue;
        }
        controller_ports_.set_button(player, button, pressed);
        matched = true;
    }
    return matched;
}

}  // namespace vanguard8::frontend

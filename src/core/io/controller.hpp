#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace vanguard8::core::io {

enum class Player : std::size_t {
    one = 0,
    two = 1,
};

enum class Button : std::uint8_t {
    up = 7,
    down = 6,
    left = 5,
    right = 4,
    a = 3,
    b = 2,
    select = 1,
    start = 0,
};

struct ControllerPortsState {
    std::array<std::uint8_t, 2> port_state = {0xFF, 0xFF};
};

class ControllerPorts {
  public:
    static constexpr std::uint8_t released_state = 0xFF;

    void reset();
    void set_button(Player player, Button button, bool pressed);
    [[nodiscard]] auto read(Player player) const -> std::uint8_t;
    [[nodiscard]] auto read_port(std::uint16_t port) const -> std::uint8_t;
    [[nodiscard]] auto state_snapshot() const -> ControllerPortsState;
    void load_state(const ControllerPortsState& state);
    [[nodiscard]] static auto player_name(Player player) -> std::string_view;
    [[nodiscard]] static auto button_name(Button button) -> std::string_view;

  private:
    std::array<std::uint8_t, 2> port_state_ = {released_state, released_state};
};

}  // namespace vanguard8::core::io

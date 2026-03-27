#pragma once

#include "core/io/controller.hpp"
#include "core/replay/types.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace vanguard8::core::replay {

class Recorder {
  public:
    static constexpr std::array<std::uint8_t, 4> magic = {'V', '8', 'R', 'R'};
    static constexpr std::uint8_t format_version = 1;

    void reset();
    void begin_power_on(const std::vector<std::uint8_t>& rom_image);
    void begin_from_save_state(
        const std::vector<std::uint8_t>& rom_image,
        const std::vector<std::uint8_t>& anchor_save_state
    );
    void record_frame(std::uint32_t frame_number, const io::ControllerPorts& controller_ports);
    void record_frame(std::uint32_t frame_number, const io::ControllerPortsState& controller_state);
    [[nodiscard]] auto serialize() const -> std::vector<std::uint8_t>;
    [[nodiscard]] auto recording() const -> const Recording&;

  private:
    Recording recording_{};
    bool active_ = false;
};

}  // namespace vanguard8::core::replay

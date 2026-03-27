#pragma once

#include "core/io/controller.hpp"
#include "core/replay/types.hpp"

#include <cstddef>
#include <vector>

namespace vanguard8::core::replay {

class Replayer {
  public:
    void reset();
    void load(const std::vector<std::uint8_t>& bytes);
    [[nodiscard]] auto rom_matches(const std::vector<std::uint8_t>& rom_image) const -> bool;
    [[nodiscard]] auto frame_count() const -> std::size_t;
    [[nodiscard]] auto recording() const -> const Recording&;
    [[nodiscard]] auto frame(std::uint32_t frame_number) const -> const FrameRecord&;
    void apply_frame(io::ControllerPorts& controller_ports, std::uint32_t frame_number) const;

  private:
    Recording recording_{};
};

}  // namespace vanguard8::core::replay

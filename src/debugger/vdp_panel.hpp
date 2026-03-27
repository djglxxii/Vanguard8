#pragma once

#include <array>
#include <cstdint>

namespace vanguard8::core {
class Emulator;

namespace video {
class V9938;
}
}

namespace vanguard8::debugger {

enum class VdpTarget {
    a,
    b,
};

struct VdpPaletteEntrySnapshot {
    std::array<std::uint8_t, 2> raw{};
    std::array<std::uint8_t, 3> rgb{};
};

struct VdpPanelSnapshot {
    VdpTarget target = VdpTarget::a;
    std::array<std::uint8_t, 64> registers{};
    std::array<std::uint8_t, 10> status{};
    std::array<VdpPaletteEntrySnapshot, 16> palette{};
    bool int_pending = false;
    bool color_zero_transparent = false;
};

class VdpPanel {
  public:
    [[nodiscard]] auto snapshot(const core::Emulator& emulator, VdpTarget target) const
        -> VdpPanelSnapshot;
};

}  // namespace vanguard8::debugger

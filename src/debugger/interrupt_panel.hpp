#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace vanguard8::core {
class Emulator;
}

namespace vanguard8::debugger {

enum class InterruptLine {
    int0,
    int1,
};

enum class InterruptSource {
    hblank,
    vblank,
    vclk,
};

struct InterruptLogEntry {
    std::uint64_t master_cycle = 0;
    std::uint64_t cpu_tstates = 0;
    std::uint64_t frame_index = 0;
    int scanline = -1;
    InterruptLine line = InterruptLine::int0;
    InterruptSource source = InterruptSource::hblank;
    bool handled = false;
    std::optional<std::uint64_t> latency_tstates;
};

class InterruptPanel {
  public:
    [[nodiscard]] auto snapshot(const core::Emulator& emulator) const -> std::vector<InterruptLogEntry>;
};

}  // namespace vanguard8::debugger

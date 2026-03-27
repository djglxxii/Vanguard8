#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace vanguard8::core {
class Emulator;
}

namespace vanguard8::debugger {

enum class MemoryRegion {
    sram,
    rom_fixed,
    rom_bank,
    logical,
    vdp_a_vram,
    vdp_b_vram,
};

struct MemorySelection {
    MemoryRegion region = MemoryRegion::sram;
    std::size_t bank_index = 0;
    std::uint32_t start = 0;
    std::size_t length = 64;
};

struct MemoryViewSnapshot {
    MemoryRegion region = MemoryRegion::sram;
    std::string_view label;
    std::uint32_t base_address = 0;
    std::size_t length = 0;
    std::vector<std::uint8_t> bytes;
};

class MemoryPanel {
  public:
    [[nodiscard]] auto snapshot(
        const core::Emulator& emulator,
        const MemorySelection& selection
    ) const -> MemoryViewSnapshot;
};

}  // namespace vanguard8::debugger

#include "debugger/memory_panel.hpp"

#include "core/emulator.hpp"
#include "core/memory/cartridge.hpp"
#include "core/memory/sram.hpp"

#include <algorithm>

namespace vanguard8::debugger {

namespace {

auto clamp_length(
    const std::uint32_t start,
    const std::size_t requested,
    const std::size_t region_size
) -> std::size_t {
    if (start >= region_size) {
        return 0;
    }
    return std::min(requested, region_size - static_cast<std::size_t>(start));
}

}  // namespace

auto MemoryPanel::snapshot(
    const core::Emulator& emulator,
    const MemorySelection& selection
) const -> MemoryViewSnapshot {
    MemoryViewSnapshot snapshot{
        .region = selection.region,
        .base_address = selection.start,
    };

    const auto& bus = emulator.bus();
    switch (selection.region) {
    case MemoryRegion::sram: {
        snapshot.label = "SRAM";
        snapshot.length = clamp_length(selection.start, selection.length, core::memory::Sram::size);
        snapshot.bytes.reserve(snapshot.length);
        for (std::size_t offset = 0; offset < snapshot.length; ++offset) {
            const auto physical = core::memory::Sram::physical_base + selection.start + offset;
            snapshot.bytes.push_back(bus.sram().read(physical));
        }
        return snapshot;
    }
    case MemoryRegion::rom_fixed: {
        snapshot.label = "ROM Fixed";
        snapshot.length =
            clamp_length(selection.start, selection.length, core::memory::CartridgeSlot::fixed_region_size);
        snapshot.bytes.reserve(snapshot.length);
        for (std::size_t offset = 0; offset < snapshot.length; ++offset) {
            snapshot.bytes.push_back(
                bus.cartridge().read_fixed(static_cast<std::uint16_t>(selection.start + offset))
            );
        }
        return snapshot;
    }
    case MemoryRegion::rom_bank: {
        snapshot.label = "ROM Bank";
        snapshot.length =
            clamp_length(selection.start, selection.length, core::memory::CartridgeSlot::bank_size);
        snapshot.bytes.reserve(snapshot.length);
        for (std::size_t offset = 0; offset < snapshot.length; ++offset) {
            snapshot.bytes.push_back(bus.cartridge().read_switchable_bank(
                selection.bank_index,
                static_cast<std::uint16_t>(selection.start + offset)
            ));
        }
        return snapshot;
    }
    case MemoryRegion::logical: {
        snapshot.label = "Logical CPU Space";
        snapshot.length = clamp_length(selection.start, selection.length, 0x10000U);
        snapshot.bytes.reserve(snapshot.length);
        for (std::size_t offset = 0; offset < snapshot.length; ++offset) {
            snapshot.bytes.push_back(
                emulator.cpu().peek_logical(static_cast<std::uint16_t>(selection.start + offset))
            );
        }
        return snapshot;
    }
    case MemoryRegion::vdp_a_vram: {
        snapshot.label = "VDP-A VRAM";
        snapshot.length = clamp_length(selection.start, selection.length, core::video::V9938::vram_size);
        snapshot.bytes.reserve(snapshot.length);
        for (std::size_t offset = 0; offset < snapshot.length; ++offset) {
            snapshot.bytes.push_back(emulator.vdp_a().vram()[selection.start + offset]);
        }
        return snapshot;
    }
    case MemoryRegion::vdp_b_vram: {
        snapshot.label = "VDP-B VRAM";
        snapshot.length = clamp_length(selection.start, selection.length, core::video::V9938::vram_size);
        snapshot.bytes.reserve(snapshot.length);
        for (std::size_t offset = 0; offset < snapshot.length; ++offset) {
            snapshot.bytes.push_back(emulator.vdp_b().vram()[selection.start + offset]);
        }
        return snapshot;
    }
    }

    return snapshot;
}

}  // namespace vanguard8::debugger

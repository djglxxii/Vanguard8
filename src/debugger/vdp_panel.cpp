#include "debugger/vdp_panel.hpp"

#include "core/emulator.hpp"

namespace vanguard8::debugger {

auto VdpPanel::snapshot(const core::Emulator& emulator, const VdpTarget target) const
    -> VdpPanelSnapshot {
    const auto& vdp = target == VdpTarget::a ? emulator.vdp_a() : emulator.vdp_b();
    VdpPanelSnapshot snapshot{
        .target = target,
        .int_pending = vdp.int_pending(),
        .color_zero_transparent = vdp.color_zero_transparent(),
    };

    for (std::size_t index = 0; index < snapshot.registers.size(); ++index) {
        snapshot.registers[index] = vdp.register_value(static_cast<std::uint8_t>(index));
    }
    for (std::size_t index = 0; index < snapshot.status.size(); ++index) {
        snapshot.status[index] = vdp.status_value(static_cast<std::uint8_t>(index));
    }
    for (std::size_t index = 0; index < snapshot.palette.size(); ++index) {
        snapshot.palette[index] = VdpPaletteEntrySnapshot{
            .raw = vdp.palette_entry_raw(static_cast<std::uint8_t>(index)),
            .rgb = vdp.palette_entry_rgb(static_cast<std::uint8_t>(index)),
        };
    }

    return snapshot;
}

}  // namespace vanguard8::debugger

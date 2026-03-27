#include "debugger/interrupt_panel.hpp"

#include "core/emulator.hpp"

namespace vanguard8::debugger {

auto InterruptPanel::snapshot(const core::Emulator& emulator) const -> std::vector<InterruptLogEntry> {
    std::vector<InterruptLogEntry> entries;
    entries.reserve(emulator.event_log().size());

    for (const auto& event : emulator.event_log()) {
        switch (event.type) {
        case vanguard8::core::EventType::hblank:
            entries.push_back(InterruptLogEntry{
                .master_cycle = event.master_cycle,
                .cpu_tstates = event.cpu_tstates,
                .frame_index = event.frame_index,
                .scanline = event.scanline,
                .line = InterruptLine::int0,
                .source = InterruptSource::hblank,
            });
            break;
        case vanguard8::core::EventType::vblank:
            entries.push_back(InterruptLogEntry{
                .master_cycle = event.master_cycle,
                .cpu_tstates = event.cpu_tstates,
                .frame_index = event.frame_index,
                .scanline = event.scanline,
                .line = InterruptLine::int0,
                .source = InterruptSource::vblank,
            });
            break;
        case vanguard8::core::EventType::vclk:
            entries.push_back(InterruptLogEntry{
                .master_cycle = event.master_cycle,
                .cpu_tstates = event.cpu_tstates,
                .frame_index = event.frame_index,
                .scanline = event.scanline,
                .line = InterruptLine::int1,
                .source = InterruptSource::vclk,
            });
            break;
        case vanguard8::core::EventType::vblank_end:
            break;
        }
    }

    return entries;
}

}  // namespace vanguard8::debugger

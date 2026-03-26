#include <catch2/catch_test_macros.hpp>

#include "core/emulator.hpp"
#include "core/scheduler.hpp"

#include <algorithm>
#include <vector>

namespace {

auto collect_type(
    const std::vector<vanguard8::core::EventLogEntry>& events,
    const vanguard8::core::EventType type
) -> std::vector<vanguard8::core::EventLogEntry> {
    std::vector<vanguard8::core::EventLogEntry> filtered;
    std::copy_if(events.begin(), events.end(), std::back_inserter(filtered), [type](const auto& event) {
        return event.type == type;
    });
    return filtered;
}

void write_vdp_register(
    vanguard8::core::Bus& bus,
    const std::uint16_t control_port,
    const std::uint8_t reg,
    const std::uint8_t value
) {
    bus.write_port(control_port, value);
    bus.write_port(control_port, static_cast<std::uint8_t>(0x80U | reg));
}

}  // namespace

TEST_CASE("scheduler executes queued events in deterministic cycle order", "[scheduler]") {
    vanguard8::core::Scheduler scheduler;

    scheduler.schedule(vanguard8::core::EventType::vblank, 30);
    scheduler.schedule(vanguard8::core::EventType::hblank, 10, 0);
    scheduler.schedule(vanguard8::core::EventType::vclk, 20);
    scheduler.schedule(vanguard8::core::EventType::hblank, 20, 1);

    REQUIRE(scheduler.size() == 4);

    const auto first = scheduler.pop();
    const auto second = scheduler.pop();
    const auto third = scheduler.pop();
    const auto fourth = scheduler.pop();

    REQUIRE(first.has_value());
    REQUIRE(second.has_value());
    REQUIRE(third.has_value());
    REQUIRE(fourth.has_value());

    REQUIRE(first->master_cycle_due == 10);
    REQUIRE(second->master_cycle_due == 20);
    REQUIRE(third->master_cycle_due == 20);
    REQUIRE(fourth->master_cycle_due == 30);
    REQUIRE(second->type == vanguard8::core::EventType::vclk);
    REQUIRE(third->type == vanguard8::core::EventType::hblank);
}

TEST_CASE("frame events land at documented master-cycle positions", "[scheduler]") {
    vanguard8::core::Emulator emulator;
    emulator.set_vclk_rate(vanguard8::core::VclkRate::hz_4000);
    emulator.clear_event_log();
    emulator.run_frames(1);

    const auto& events = emulator.event_log();
    const auto hblanks = collect_type(events, vanguard8::core::EventType::hblank);
    const auto vblanks = collect_type(events, vanguard8::core::EventType::vblank);
    const auto vblank_end = collect_type(events, vanguard8::core::EventType::vblank_end);
    const auto vclks = collect_type(events, vanguard8::core::EventType::vclk);

    REQUIRE(hblanks.size() == vanguard8::core::timing::active_lines);
    REQUIRE(vblanks.size() == 1);
    REQUIRE(vblank_end.size() == 1);
    REQUIRE_FALSE(vclks.empty());

    REQUIRE(hblanks.front().master_cycle == vanguard8::core::timing::master_hblank_start);
    REQUIRE(hblanks.front().scanline == 0);
    REQUIRE(hblanks[1].master_cycle ==
            vanguard8::core::timing::master_per_line + vanguard8::core::timing::master_hblank_start);
    REQUIRE(hblanks.back().master_cycle ==
            ((static_cast<std::uint64_t>(vanguard8::core::timing::active_lines) - 1U) *
             vanguard8::core::timing::master_per_line) +
                vanguard8::core::timing::master_hblank_start);
    REQUIRE(vblanks.front().master_cycle ==
            static_cast<std::uint64_t>(vanguard8::core::timing::active_lines) *
                vanguard8::core::timing::master_per_line);
    REQUIRE(vblank_end.front().master_cycle == vanguard8::core::timing::master_per_frame);
    REQUIRE(vclks.front().master_cycle == 3580);
}

TEST_CASE("repeated runs produce stable event timing and interrupt-source logs", "[scheduler]") {
    vanguard8::core::Emulator first;
    first.set_vclk_rate(vanguard8::core::VclkRate::hz_6000);
    first.run_frames(2);

    vanguard8::core::Emulator second;
    second.set_vclk_rate(vanguard8::core::VclkRate::hz_6000);
    second.run_frames(2);

    REQUIRE(first.completed_frames() == 2);
    REQUIRE(second.completed_frames() == 2);
    REQUIRE(first.event_log().size() == second.event_log().size());
    REQUIRE(first.event_log_digest() == second.event_log_digest());

    for (std::size_t index = 0; index < first.event_log().size(); ++index) {
        REQUIRE(first.event_log()[index].type == second.event_log()[index].type);
        REQUIRE(first.event_log()[index].master_cycle == second.event_log()[index].master_cycle);
        REQUIRE(first.event_log()[index].scanline == second.event_log()[index].scanline);
    }
}

TEST_CASE("cpu cycle accounting stays monotonic through scheduled events", "[scheduler]") {
    vanguard8::core::Emulator emulator;
    emulator.set_vclk_rate(vanguard8::core::VclkRate::hz_8000);
    emulator.run_frames(1);

    const auto& events = emulator.event_log();
    REQUIRE_FALSE(events.empty());

    std::uint64_t previous_tstates = 0;
    std::uint64_t previous_cycles = 0;
    for (const auto& event : events) {
        REQUIRE(event.master_cycle >= previous_cycles);
        REQUIRE(event.cpu_tstates >= previous_tstates);
        previous_cycles = event.master_cycle;
        previous_tstates = event.cpu_tstates;
    }

    REQUIRE(emulator.cpu_tstates() == emulator.master_cycle() / vanguard8::core::timing::cpu_divider);
}

TEST_CASE("headless runtime controls pause run and single-frame step at frame boundaries", "[scheduler]") {
    vanguard8::core::Emulator emulator;
    emulator.pause();
    emulator.run_frames(2);

    REQUIRE(emulator.completed_frames() == 0);

    emulator.step_frame();
    REQUIRE(emulator.completed_frames() == 1);
    REQUIRE(emulator.paused());

    emulator.resume();
    emulator.run_frames(2);
    REQUIRE(emulator.completed_frames() == 3);
}

TEST_CASE("scheduler-driven VDP-A interrupts reach INT0 while VDP-B stays isolated", "[scheduler]") {
    vanguard8::core::Emulator emulator;

    write_vdp_register(emulator.mutable_bus(), 0x81, 1, 0x20);
    emulator.run_frames(1);

    REQUIRE(emulator.bus().int0_asserted());
    REQUIRE((emulator.vdp_a().status_value(0) & 0x80U) != 0U);

    vanguard8::core::Emulator isolated;
    write_vdp_register(isolated.mutable_bus(), 0x85, 1, 0x20);
    isolated.run_frames(1);

    REQUIRE_FALSE(isolated.bus().int0_asserted());
    REQUIRE((isolated.vdp_b().status_value(0) & 0x80U) != 0U);
}

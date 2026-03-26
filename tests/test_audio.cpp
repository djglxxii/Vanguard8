#include <catch2/catch_test_macros.hpp>

#include "core/bus.hpp"
#include "core/emulator.hpp"

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

void write_ay_register(vanguard8::core::Bus& bus, const std::uint8_t reg, const std::uint8_t value) {
    bus.write_port(0x50, reg);
    bus.write_port(0x51, value);
}

void program_deterministic_audio(vanguard8::core::Emulator& emulator) {
    auto& bus = emulator.mutable_bus();

    write_ay_register(bus, 0x00, 0x34);
    write_ay_register(bus, 0x01, 0x01);
    write_ay_register(bus, 0x07, 0x3E);
    write_ay_register(bus, 0x08, 0x0F);

    bus.write_port(0x61, 0x07);
    bus.write_port(0x60, 0x01);
}

}  // namespace

TEST_CASE("YM2151 status exposes busy timing after address and data writes", "[audio]") {
    vanguard8::core::Bus bus{};

    bus.write_port(0x40, 0x20);
    bus.write_port(0x41, 0x7F);

    bus.run_audio(8);
    REQUIRE((bus.read_port(0x40) & 0x80U) != 0U);

    bus.run_audio(128);

    REQUIRE((bus.read_port(0x40) & 0x80U) == 0U);
}

TEST_CASE("AY-3-8910 latch read and write behavior follows the documented ports", "[audio]") {
    vanguard8::core::Bus bus{};

    write_ay_register(bus, 0x07, 0xFF);
    REQUIRE(bus.read_port(0x51) == 0x3FU);

    write_ay_register(bus, 0x08, 0x1F);
    REQUIRE(bus.read_port(0x51) == 0x1FU);

    bus.write_port(0x50, 0x01);
    bus.write_port(0x51, 0xF3);
    REQUIRE(bus.read_port(0x51) == 0x03U);
}

TEST_CASE("MSM5205 control writes update scheduler VCLK cadence", "[audio]") {
    vanguard8::core::Emulator emulator;
    emulator.mutable_bus().write_port(0x60, 0x02);
    emulator.run_frames(1);

    const auto vclks = collect_type(emulator.event_log(), vanguard8::core::EventType::vclk);
    REQUIRE(vclks.size() > 2);
    REQUIRE(vclks.front().master_cycle == 1790);
    REQUIRE(vclks[1].master_cycle - vclks[0].master_cycle == 1790);

    vanguard8::core::Emulator stopped;
    stopped.mutable_bus().write_port(0x60, 0x83);
    stopped.run_frames(1);
    REQUIRE(collect_type(stopped.event_log(), vanguard8::core::EventType::vclk).empty());
}

TEST_CASE("repeated runs produce stable INT1 cadence and deterministic audio hashes", "[audio]") {
    vanguard8::core::Emulator first;
    program_deterministic_audio(first);
    first.run_frames(2);

    vanguard8::core::Emulator second;
    program_deterministic_audio(second);
    second.run_frames(2);

    REQUIRE(first.bus().msm5205().vclk_count() > 0U);
    REQUIRE(first.bus().msm5205().vclk_count() == second.bus().msm5205().vclk_count());
    REQUIRE(first.audio_output_sample_count() > 0U);
    REQUIRE(first.audio_output_sample_count() == second.audio_output_sample_count());
    REQUIRE(first.audio_output_digest() == second.audio_output_digest());
}

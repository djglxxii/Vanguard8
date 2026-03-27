#include <catch2/catch_test_macros.hpp>

#include "core/memory/cartridge.hpp"
#include "core/replay/recorder.hpp"
#include "core/replay/replayer.hpp"
#include "core/rewind.hpp"
#include "core/save_state.hpp"

#include <array>
#include <vector>

namespace {

auto make_idle_rom() -> std::vector<std::uint8_t> {
    auto rom = std::vector<std::uint8_t>(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);
    rom[0x0000] = 0x76;
    return rom;
}

auto controller_state_for_frame(const std::uint32_t frame) -> vanguard8::core::io::ControllerPortsState {
    auto controller1 = static_cast<std::uint8_t>(0xFFU);
    auto controller2 = static_cast<std::uint8_t>(0xFFU);

    controller1 = static_cast<std::uint8_t>(controller1 & ~(1U << (frame % 8U)));
    controller2 = static_cast<std::uint8_t>(controller2 & ~(1U << ((frame * 3U) % 8U)));

    if ((frame % 5U) == 0U) {
        controller1 = static_cast<std::uint8_t>(controller1 & 0xFEU);
    }
    if ((frame % 7U) == 0U) {
        controller2 = static_cast<std::uint8_t>(controller2 & 0xFDU);
    }

    return vanguard8::core::io::ControllerPortsState{
        .port_state = {controller1, controller2},
    };
}

void expect_same_runtime_state(
    const vanguard8::core::Emulator& lhs,
    const vanguard8::core::Emulator& rhs
) {
    REQUIRE(lhs.master_cycle() == rhs.master_cycle());
    REQUIRE(lhs.cpu_tstates() == rhs.cpu_tstates());
    REQUIRE(lhs.completed_frames() == rhs.completed_frames());
    REQUIRE(lhs.event_log_digest() == rhs.event_log_digest());
    REQUIRE(lhs.audio_output_digest() == rhs.audio_output_digest());
    REQUIRE(lhs.bus().controller_ports().state_snapshot().port_state ==
            rhs.bus().controller_ports().state_snapshot().port_state);
}

}  // namespace

TEST_CASE("rewind restores prior snapshots and resumes deterministically", "[rewind]") {
    const auto rom = make_idle_rom();

    vanguard8::core::Emulator emulator;
    emulator.load_rom_image(rom);
    emulator.set_vclk_rate(vanguard8::core::VclkRate::hz_6000);

    vanguard8::core::RewindBuffer rewind(8);
    std::vector<std::vector<std::uint8_t>> checkpoints;
    rewind.capture(emulator);
    checkpoints.push_back(vanguard8::core::SaveState::serialize(emulator));

    const std::array<vanguard8::core::io::ControllerPortsState, 6> inputs = {
        vanguard8::core::io::ControllerPortsState{.port_state = {0xFE, 0xFF}},
        vanguard8::core::io::ControllerPortsState{.port_state = {0xFD, 0xFF}},
        vanguard8::core::io::ControllerPortsState{.port_state = {0xFB, 0xFE}},
        vanguard8::core::io::ControllerPortsState{.port_state = {0xF7, 0xFD}},
        vanguard8::core::io::ControllerPortsState{.port_state = {0xEF, 0xFB}},
        vanguard8::core::io::ControllerPortsState{.port_state = {0xDF, 0xF7}},
    };

    for (std::size_t index = 0; index < inputs.size(); ++index) {
        emulator.mutable_bus().mutable_controller_ports().load_state(inputs[index]);
        emulator.run_frames(1);
        if ((emulator.completed_frames() % 2U) == 0U) {
            rewind.capture(emulator);
            checkpoints.push_back(vanguard8::core::SaveState::serialize(emulator));
        }
    }

    REQUIRE(emulator.completed_frames() == 6);
    REQUIRE(rewind.size() == 4);

    REQUIRE(rewind.rewind_step(emulator));
    REQUIRE(rewind.current_frame() == 4);
    vanguard8::core::Emulator frame4;
    vanguard8::core::SaveState::load(frame4, checkpoints[2]);
    expect_same_runtime_state(emulator, frame4);

    REQUIRE(rewind.rewind_step(emulator));
    REQUIRE(rewind.current_frame() == 2);
    vanguard8::core::Emulator frame2;
    vanguard8::core::SaveState::load(frame2, checkpoints[1]);
    expect_same_runtime_state(emulator, frame2);

    const std::array<vanguard8::core::io::ControllerPortsState, 2> resumed_inputs = {
        vanguard8::core::io::ControllerPortsState{.port_state = {0xBF, 0xEF}},
        vanguard8::core::io::ControllerPortsState{.port_state = {0x7F, 0xDF}},
    };

    for (const auto& input : resumed_inputs) {
        emulator.mutable_bus().mutable_controller_ports().load_state(input);
        frame2.mutable_bus().mutable_controller_ports().load_state(input);
        emulator.run_frames(1);
        frame2.run_frames(1);
    }

    expect_same_runtime_state(emulator, frame2);
}

TEST_CASE("embedded-save-state replays preserve deterministic frame progression", "[replay]") {
    const auto rom = make_idle_rom();

    vanguard8::core::Emulator recorded;
    recorded.load_rom_image(rom);
    recorded.set_vclk_rate(vanguard8::core::VclkRate::hz_4000);
    recorded.run_frames(1);
    const auto anchor = vanguard8::core::SaveState::serialize(recorded);

    vanguard8::core::replay::Recorder recorder;
    recorder.begin_from_save_state(rom, anchor);

    const std::array<vanguard8::core::io::ControllerPortsState, 4> inputs = {
        vanguard8::core::io::ControllerPortsState{.port_state = {0xFE, 0xFF}},
        vanguard8::core::io::ControllerPortsState{.port_state = {0xFD, 0xFE}},
        vanguard8::core::io::ControllerPortsState{.port_state = {0xFB, 0xFD}},
        vanguard8::core::io::ControllerPortsState{.port_state = {0xF7, 0xFB}},
    };

    for (std::size_t frame = 0; frame < inputs.size(); ++frame) {
        recorded.mutable_bus().mutable_controller_ports().load_state(inputs[frame]);
        recorder.record_frame(static_cast<std::uint32_t>(frame), recorded.bus().controller_ports());
        recorded.run_frames(1);
    }

    const auto bytes = recorder.serialize();
    REQUIRE(bytes.size() > 32);

    vanguard8::core::replay::Replayer replayer;
    replayer.load(bytes);
    REQUIRE(replayer.frame_count() == inputs.size());
    REQUIRE(replayer.recording().anchor_type == vanguard8::core::replay::AnchorType::embedded_save_state);
    REQUIRE(replayer.recording().anchor_save_state == anchor);
    REQUIRE(replayer.rom_matches(rom));

    vanguard8::core::Emulator replayed;
    vanguard8::core::SaveState::load(replayed, replayer.recording().anchor_save_state);

    for (std::size_t frame = 0; frame < inputs.size(); ++frame) {
        replayer.apply_frame(replayed.mutable_bus().mutable_controller_ports(), static_cast<std::uint32_t>(frame));
        REQUIRE(replayed.bus().controller_ports().state_snapshot().port_state == inputs[frame].port_state);
        replayed.run_frames(1);
    }

    expect_same_runtime_state(replayed, recorded);
}

TEST_CASE("long-running save-state anchored replay stays deterministic across restore boundaries", "[replay][integration]") {
    const auto rom = make_idle_rom();
    constexpr std::uint32_t frame_count = 180;
    constexpr std::uint32_t split_frame = 96;

    vanguard8::core::Emulator recorded;
    recorded.load_rom_image(rom);
    recorded.set_vclk_rate(vanguard8::core::VclkRate::hz_8000);
    recorded.run_frames(3);
    const auto anchor = vanguard8::core::SaveState::serialize(recorded);

    vanguard8::core::replay::Recorder recorder;
    recorder.begin_from_save_state(rom, anchor);

    for (std::uint32_t frame = 0; frame < frame_count; ++frame) {
        const auto input = controller_state_for_frame(frame);
        recorded.mutable_bus().mutable_controller_ports().load_state(input);
        recorder.record_frame(frame, input);
        recorded.run_frames(1);
    }

    const auto replay_bytes = recorder.serialize();

    vanguard8::core::replay::Replayer replayer;
    replayer.load(replay_bytes);
    REQUIRE(replayer.frame_count() == frame_count);
    REQUIRE(replayer.rom_matches(rom));

    vanguard8::core::Emulator replayed;
    vanguard8::core::SaveState::load(replayed, replayer.recording().anchor_save_state);

    for (std::uint32_t frame = 0; frame < split_frame; ++frame) {
        replayer.apply_frame(replayed.mutable_bus().mutable_controller_ports(), frame);
        replayed.run_frames(1);
    }

    auto resumed = vanguard8::core::Emulator{};
    vanguard8::core::SaveState::load(resumed, vanguard8::core::SaveState::serialize(replayed));

    for (std::uint32_t frame = split_frame; frame < frame_count; ++frame) {
        replayer.apply_frame(replayed.mutable_bus().mutable_controller_ports(), frame);
        replayer.apply_frame(resumed.mutable_bus().mutable_controller_ports(), frame);
        INFO("frame=" << frame);
        INFO("replayed master=" << replayed.master_cycle() << " completed=" << replayed.completed_frames());
        INFO("resumed master=" << resumed.master_cycle() << " completed=" << resumed.completed_frames());
        REQUIRE_NOTHROW(replayed.run_frames(1));
        REQUIRE_NOTHROW(resumed.run_frames(1));
    }

    expect_same_runtime_state(replayed, resumed);
    expect_same_runtime_state(replayed, recorded);
    REQUIRE(replayed.audio_output_sample_count() == recorded.audio_output_sample_count());
}

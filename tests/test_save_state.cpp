#include <catch2/catch_test_macros.hpp>

#include "core/save_state.hpp"
#include "core/memory/cartridge.hpp"

#include <vector>

TEST_CASE("save states round-trip core state and continue deterministically", "[save_state]") {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size * 2, 0x00);
    rom[0x0000] = 0x3E;
    rom[0x0001] = 0x04;
    rom[0x0002] = 0xED;
    rom[0x0003] = 0x39;
    rom[0x0004] = 0x39;
    rom[0x0005] = 0x76;
    rom[0x4000] = 0xA5;

    vanguard8::core::Emulator emulator;
    emulator.load_rom_image(rom);
    emulator.set_vclk_rate(vanguard8::core::VclkRate::hz_6000);
    emulator.mutable_bus().mutable_sram().write(vanguard8::core::memory::Sram::physical_base + 3, 0x5A);
    emulator.mutable_bus().mutable_controller_ports().set_button(
        vanguard8::core::io::Player::one,
        vanguard8::core::io::Button::a,
        true
    );
    emulator.mutable_bus().mutable_vdp_a().poke_vram(0x0010, 0xAB);
    emulator.mutable_bus().mutable_vdp_a().write_control(0x11);
    emulator.mutable_bus().mutable_vdp_a().write_control(0x80 | 23);
    emulator.mutable_bus().mutable_vdp_a().write_register(0x22);
    emulator.mutable_bus().mutable_vdp_a().write_palette(0x01);
    emulator.mutable_bus().mutable_vdp_a().write_palette(0x34);
    emulator.mutable_bus().mutable_vdp_a().write_palette(0x56);
    emulator.mutable_bus().write_port(0x40, 0x14);
    emulator.mutable_bus().write_port(0x41, 0x80);
    emulator.mutable_bus().write_port(0x50, 0x07);
    emulator.mutable_bus().write_port(0x51, 0x3E);
    emulator.mutable_bus().write_port(0x60, 0x02);
    emulator.mutable_bus().write_port(0x61, 0x0C);
    emulator.mutable_cpu().out0(0x33, 0xE0);
    emulator.mutable_cpu().out0(0x38, 0xF0);
    emulator.mutable_cpu().out0(0x39, 0x04);
    emulator.mutable_cpu().out0(0x3A, 0x48);
    emulator.mutable_cpu().out0(0x0E, 0x34);
    emulator.mutable_cpu().out0(0x0F, 0x12);
    emulator.run_frames(1);
    REQUIRE(emulator.vdp_a().register_value(23) == 0x11);

    const auto bytes = vanguard8::core::SaveState::serialize(emulator);
    REQUIRE(bytes.size() > 16);
    REQUIRE(bytes[0] == 'V');
    REQUIRE(bytes[1] == '8');
    REQUIRE(bytes[2] == 'S');
    REQUIRE(bytes[3] == 'T');

    vanguard8::core::Emulator restored;
    vanguard8::core::SaveState::load(restored, bytes);

    REQUIRE(restored.bus().cartridge().rom_image() == rom);
    REQUIRE(restored.bus().sram().read(vanguard8::core::memory::Sram::physical_base + 3) == 0x5A);
    REQUIRE(restored.bus().controller_ports().read(vanguard8::core::io::Player::one) != 0xFF);
    REQUIRE(restored.vdp_a().vram()[0x0010] == 0xAB);
    REQUIRE(restored.vdp_a().register_value(23) == 0x11);
    REQUIRE(restored.vdp_a().palette_entry_raw(1) == std::array<std::uint8_t, 2>{0x34, 0x56});
    REQUIRE(restored.bus().ym2151().latched_address() == 0x14);
    REQUIRE(restored.bus().ay8910().selected_register() == 0x07);
    REQUIRE(restored.bus().msm5205().control() == 0x02);
    REQUIRE(restored.cpu().state_snapshot().registers.bbr == 0x04);
    REQUIRE(restored.cpu().state_snapshot().registers.cbar == 0x48);
    REQUIRE(restored.cpu().state_snapshot().registers.cbr == 0xF0);
    REQUIRE(restored.cpu().state_snapshot().registers.rldr0 == 0x1234);

    emulator.run_frames(2);
    restored.run_frames(2);
    REQUIRE(restored.master_cycle() == emulator.master_cycle());
    REQUIRE(restored.cpu_tstates() == emulator.cpu_tstates());
    REQUIRE(restored.completed_frames() == emulator.completed_frames());
    REQUIRE(restored.event_log_digest() == emulator.event_log_digest());
    REQUIRE(restored.audio_output_digest() == emulator.audio_output_digest());
}

#include <catch2/catch_test_macros.hpp>

#include "core/emulator.hpp"
#include "core/memory/cartridge.hpp"
#include "debugger/cpu_panel.hpp"
#include "debugger/memory_panel.hpp"
#include "debugger/vdp_panel.hpp"
#include "frontend/video_fixture.hpp"

#include <vector>

TEST_CASE("cpu panel snapshots registers and the extracted disassembly surface", "[debugger]") {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);
    rom[0x0000] = 0x3E;
    rom[0x0001] = 0x42;
    rom[0x0002] = 0x21;
    rom[0x0003] = 0x34;
    rom[0x0004] = 0x12;
    rom[0x0005] = 0xED;
    rom[0x0006] = 0x47;
    rom[0x0007] = 0x76;

    vanguard8::core::Emulator emulator;
    emulator.load_rom_image(rom);

    vanguard8::debugger::CpuPanel panel;
    const auto snapshot = panel.snapshot(emulator);

    REQUIRE(snapshot.cpu.registers.pc == 0x0000);
    REQUIRE(snapshot.cpu.registers.cbar == 0xFF);
    REQUIRE(snapshot.cpu.registers.halted == false);
    REQUIRE(snapshot.flags.zero);
    REQUIRE_FALSE(snapshot.emulator_paused);
    REQUIRE(snapshot.disassembly.size() == 20);
    REQUIRE(snapshot.disassembly[0].mnemonic == "LD A,0x42");
    REQUIRE(snapshot.disassembly[0].current_pc);
    REQUIRE(snapshot.disassembly[1].mnemonic == "LD HL,0x1234");
    REQUIRE(snapshot.disassembly[2].mnemonic == "LD I,A");
    REQUIRE(snapshot.disassembly[3].mnemonic == "HALT");
}

TEST_CASE("cpu panel control queue respects pause resume and frame-step boundaries", "[debugger]") {
    vanguard8::core::Emulator emulator;
    vanguard8::debugger::CpuPanel panel;

    panel.queue_command(vanguard8::debugger::ExecutionCommand::pause);
    panel.apply_pending(emulator);
    REQUIRE(emulator.paused());
    REQUIRE(panel.pending_command_count() == 0);

    panel.queue_command(vanguard8::debugger::ExecutionCommand::step_frame);
    panel.apply_pending(emulator);
    REQUIRE(emulator.completed_frames() == 1);
    REQUIRE(emulator.paused());

    panel.queue_command(vanguard8::debugger::ExecutionCommand::resume);
    panel.apply_pending(emulator);
    REQUIRE_FALSE(emulator.paused());
}

TEST_CASE("memory and VDP panels expose ROM SRAM VRAM registers and palette state", "[debugger]") {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size * 2, 0x00);
    rom[0x0000] = 0xAA;
    rom[0x4000] = 0xBB;

    vanguard8::core::Emulator emulator;
    emulator.load_rom_image(rom);
    emulator.mutable_bus().mutable_sram().write(vanguard8::core::memory::Sram::physical_base, 0xCC);
    vanguard8::frontend::build_dual_vdp_fixture_frame(
        emulator.mutable_bus().mutable_vdp_a(),
        emulator.mutable_bus().mutable_vdp_b()
    );

    vanguard8::debugger::MemoryPanel memory_panel;
    const auto rom_fixed = memory_panel.snapshot(
        emulator,
        vanguard8::debugger::MemorySelection{
            .region = vanguard8::debugger::MemoryRegion::rom_fixed,
            .length = 4,
        }
    );
    const auto rom_bank = memory_panel.snapshot(
        emulator,
        vanguard8::debugger::MemorySelection{
            .region = vanguard8::debugger::MemoryRegion::rom_bank,
            .bank_index = 0,
            .length = 4,
        }
    );
    const auto sram = memory_panel.snapshot(
        emulator,
        vanguard8::debugger::MemorySelection{
            .region = vanguard8::debugger::MemoryRegion::sram,
            .length = 4,
        }
    );
    const auto vram = memory_panel.snapshot(
        emulator,
        vanguard8::debugger::MemorySelection{
            .region = vanguard8::debugger::MemoryRegion::vdp_a_vram,
            .length = 4,
        }
    );

    REQUIRE(rom_fixed.bytes[0] == 0xAA);
    REQUIRE(rom_bank.bytes[0] == 0xBB);
    REQUIRE(sram.bytes[0] == 0xCC);
    REQUIRE(vram.bytes.size() == 4);

    vanguard8::debugger::VdpPanel vdp_panel;
    const auto vdp = vdp_panel.snapshot(emulator, vanguard8::debugger::VdpTarget::a);
    REQUIRE(vdp.registers[8] == emulator.vdp_a().register_value(8));
    REQUIRE(vdp.status[2] == emulator.vdp_a().status_value(2));
    REQUIRE(vdp.palette[1].raw == emulator.vdp_a().palette_entry_raw(1));
    REQUIRE(vdp.palette[1].rgb == emulator.vdp_a().palette_entry_rgb(1));
}

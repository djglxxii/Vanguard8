#include <catch2/catch_test_macros.hpp>

#include "core/emulator.hpp"
#include "core/memory/cartridge.hpp"
#include "debugger/bank_panel.hpp"
#include "debugger/cpu_panel.hpp"
#include "debugger/interrupt_panel.hpp"

#include <vector>

TEST_CASE("PC and I/O breakpoints trigger deterministically on the extracted CPU path", "[debugger]") {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);
    rom[0x0000] = 0x3E;
    rom[0x0001] = 0x04;
    rom[0x0002] = 0xED;
    rom[0x0003] = 0x39;
    rom[0x0004] = 0x39;
    rom[0x0005] = 0x76;

    vanguard8::debugger::CpuPanel panel;

    vanguard8::core::Emulator first;
    first.load_rom_image(rom);
    panel.add_breakpoint(
        first,
        vanguard8::core::cpu::Breakpoint{
            .type = vanguard8::core::cpu::BreakpointType::pc,
            .address = 0x0002,
        }
    );
    const auto first_pc = panel.run_until_breakpoint_or_halt(first, 16);

    vanguard8::core::Emulator second;
    second.load_rom_image(rom);
    panel.add_breakpoint(
        second,
        vanguard8::core::cpu::Breakpoint{
            .type = vanguard8::core::cpu::BreakpointType::pc,
            .address = 0x0002,
        }
    );
    const auto second_pc = panel.run_until_breakpoint_or_halt(second, 16);

    REQUIRE(first_pc.breakpoint_hit);
    REQUIRE(second_pc.breakpoint_hit);
    REQUIRE(first_pc.executed_instructions == 1);
    REQUIRE(second_pc.executed_instructions == 1);
    REQUIRE(first_pc.hit->breakpoint.type == vanguard8::core::cpu::BreakpointType::pc);
    REQUIRE(second_pc.hit->breakpoint.type == vanguard8::core::cpu::BreakpointType::pc);
    REQUIRE(first_pc.hit->address == 0x0002);
    REQUIRE(second_pc.hit->address == 0x0002);

    first.mutable_cpu().clear_breakpoints();
    panel.clear_breakpoint_hits(first);
    panel.add_breakpoint(
        first,
        vanguard8::core::cpu::Breakpoint{
            .type = vanguard8::core::cpu::BreakpointType::io_write,
            .address = 0x0039,
            .value = 0x04,
        }
    );
    const auto io_hit = panel.run_until_breakpoint_or_halt(first, 16);
    REQUIRE(io_hit.breakpoint_hit);
    REQUIRE(io_hit.hit->breakpoint.type == vanguard8::core::cpu::BreakpointType::io_write);
    REQUIRE(io_hit.hit->address == 0x0039);
    REQUIRE(io_hit.hit->value == 0x04);
}

TEST_CASE("interrupt panel exposes scheduler-driven INT0 and INT1 history", "[debugger]") {
    vanguard8::core::Emulator emulator;
    emulator.set_vclk_rate(vanguard8::core::VclkRate::hz_4000);
    emulator.run_frames(1);

    vanguard8::debugger::InterruptPanel panel;
    const auto entries = panel.snapshot(emulator);

    REQUIRE_FALSE(entries.empty());
    bool saw_hblank = false;
    bool saw_vblank = false;
    bool saw_vclk = false;
    for (const auto& entry : entries) {
        if (entry.source == vanguard8::debugger::InterruptSource::hblank) {
            saw_hblank = true;
            REQUIRE(entry.line == vanguard8::debugger::InterruptLine::int0);
        }
        if (entry.source == vanguard8::debugger::InterruptSource::vblank) {
            saw_vblank = true;
            REQUIRE(entry.line == vanguard8::debugger::InterruptLine::int0);
        }
        if (entry.source == vanguard8::debugger::InterruptSource::vclk) {
            saw_vclk = true;
            REQUIRE(entry.line == vanguard8::debugger::InterruptLine::int1);
        }
    }
    REQUIRE(saw_hblank);
    REQUIRE(saw_vblank);
    REQUIRE(saw_vclk);
}

TEST_CASE("bank tracker logs BBR writes with decoded bank and legality", "[debugger]") {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);
    rom[0x0000] = 0x3E;
    rom[0x0001] = 0x04;
    rom[0x0002] = 0xED;
    rom[0x0003] = 0x39;
    rom[0x0004] = 0x39;
    rom[0x0005] = 0x3E;
    rom[0x0006] = 0xF0;
    rom[0x0007] = 0xED;
    rom[0x0008] = 0x39;
    rom[0x0009] = 0x39;
    rom[0x000A] = 0x76;

    vanguard8::core::Emulator emulator;
    emulator.load_rom_image(rom);
    emulator.mutable_cpu().run_until_halt(16);

    vanguard8::debugger::BankPanel panel;
    const auto entries = panel.snapshot(emulator);

    REQUIRE(entries.size() == 2);
    REQUIRE(entries[0].bbr == 0x04);
    REQUIRE(entries[0].bank == 0);
    REQUIRE(entries[0].legal);
    REQUIRE(entries[1].bbr == 0xF0);
    REQUIRE(entries[1].bank == 59);
    REQUIRE_FALSE(entries[1].legal);
}

#include <catch2/catch_test_macros.hpp>

#include "core/bus.hpp"
#include "core/cpu/z180_adapter.hpp"

#include <algorithm>
#include <vector>

namespace {

auto make_boot_test_rom() -> std::vector<std::uint8_t> {
    constexpr std::size_t rom_size = 0xC000;
    std::vector<std::uint8_t> rom(rom_size, 0x00);

    rom[0x0000] = 0x3E; rom[0x0001] = 0x48;            // LD A,0x48
    rom[0x0002] = 0xED; rom[0x0003] = 0x39; rom[0x0004] = 0x3A;  // OUT0 (0x3A),A
    rom[0x0005] = 0x3E; rom[0x0006] = 0xF0;            // LD A,0xF0
    rom[0x0007] = 0xED; rom[0x0008] = 0x39; rom[0x0009] = 0x38;  // OUT0 (0x38),A
    rom[0x000A] = 0x3E; rom[0x000B] = 0x04;            // LD A,0x04
    rom[0x000C] = 0xED; rom[0x000D] = 0x39; rom[0x000E] = 0x39;  // OUT0 (0x39),A
    rom[0x000F] = 0x3E; rom[0x0010] = 0x42;            // LD A,0x42
    rom[0x0011] = 0x32; rom[0x0012] = 0x00; rom[0x0013] = 0x80;  // LD (0x8000),A
    rom[0x0014] = 0x3E; rom[0x0015] = 0x08;            // LD A,0x08
    rom[0x0016] = 0xED; rom[0x0017] = 0x39; rom[0x0018] = 0x39;  // OUT0 (0x39),A
    rom[0x0019] = 0x3A; rom[0x001A] = 0x00; rom[0x001B] = 0x40;  // LD A,(0x4000)
    rom[0x001C] = 0x32; rom[0x001D] = 0x01; rom[0x001E] = 0x80;  // LD (0x8001),A
    rom[0x001F] = 0x76;                                            // HALT

    std::fill(rom.begin() + 0x4000, rom.begin() + 0x8000, 0xA0);
    std::fill(rom.begin() + 0x8000, rom.begin() + 0xC000, 0xB1);
    return rom;
}

}  // namespace

TEST_CASE("z180 adapter reset matches documented HD64180 defaults", "[cpu]") {
    vanguard8::core::Bus bus{};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    REQUIRE(cpu.pc() == 0x0000);
    REQUIRE(cpu.cbar() == 0xFF);
    REQUIRE(cpu.cbr() == 0x00);
    REQUIRE(cpu.bbr() == 0x00);
}

TEST_CASE("MMU boot mapping and bank-window translation follow CBAR CBR BBR", "[cpu]") {
    vanguard8::core::Bus bus{};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    cpu.out0(0x3A, 0x48);
    cpu.out0(0x38, 0xF0);
    cpu.out0(0x39, 0x04);

    REQUIRE(cpu.translate_logical_address(0x0000) == 0x00000);
    REQUIRE(cpu.translate_logical_address(0x3FFF) == 0x03FFF);
    REQUIRE(cpu.translate_logical_address(0x4000) == 0x04000);
    REQUIRE(cpu.translate_logical_address(0x7FFF) == 0x07FFF);
    REQUIRE(cpu.translate_logical_address(0x8000) == 0xF0000);
    REQUIRE(cpu.translate_logical_address(0xFFFF) == 0xF7FFF);
}

TEST_CASE("OUT0 writes to MMU registers affect bank translation", "[cpu]") {
    vanguard8::core::Bus bus{};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    cpu.out0(0x3A, 0x48);
    cpu.out0(0x39, 0x08);

    REQUIRE(cpu.in0(0x3A) == 0x48);
    REQUIRE(cpu.in0(0x39) == 0x08);
    REQUIRE(cpu.translate_logical_address(0x4000) == 0x08000);
    REQUIRE(cpu.translate_logical_address(0x7FFF) == 0x0BFFF);
}

TEST_CASE("Illegal BBR writes warn and alias the bank window into SRAM space", "[cpu]") {
    vanguard8::core::Bus bus{};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    cpu.out0(0x3A, 0x48);
    cpu.out0(0x39, 0xF0);

    REQUIRE(cpu.translate_logical_address(0x4000) == 0xF0000);
    REQUIRE_FALSE(bus.warnings().empty());
}

TEST_CASE("INT0 follows IM1 behavior", "[cpu]") {
    vanguard8::core::Bus bus{};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    cpu.set_interrupt_mode(1);
    cpu.set_iff1(true);
    bus.set_vdp_a_vblank(true);

    const auto service = cpu.service_pending_interrupt();
    REQUIRE(service.has_value());
    REQUIRE(service->source == vanguard8::core::cpu::InterruptSource::int0);
    REQUIRE(service->handler_address == 0x0038);
}

TEST_CASE("INT1 uses the I IL vectored path independent of IM mode", "[cpu]") {
    vanguard8::core::Bus bus{};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    cpu.out0(0x3A, 0x48);
    cpu.out0(0x38, 0xF0);
    cpu.out0(0x33, 0xF8);
    cpu.out0(0x34, 0x02);
    cpu.set_register_i(0x80);
    cpu.set_interrupt_mode(0);

    bus.write_memory(0xF00F8, 0x34);
    bus.write_memory(0xF00F9, 0x12);
    bus.set_int1(true);

    const auto service = cpu.service_pending_interrupt();
    REQUIRE(service.has_value());
    REQUIRE(service->source == vanguard8::core::cpu::InterruptSource::int1);
    REQUIRE(service->handler_address == 0x1234);
}

TEST_CASE("Test ROM can boot switch banks and read write SRAM correctly", "[cpu]") {
    const auto rom = make_boot_test_rom();
    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    cpu.run_until_halt(32);

    REQUIRE(cpu.cbar() == 0x48);
    REQUIRE(cpu.cbr() == 0xF0);
    REQUIRE(cpu.bbr() == 0x08);
    REQUIRE(bus.read_memory(0xF0000) == 0x42);
    REQUIRE(bus.read_memory(0xF0001) == 0xB1);
}

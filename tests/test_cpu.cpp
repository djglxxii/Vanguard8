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

void write_vdp_register(
    vanguard8::core::Bus& bus,
    const std::uint16_t control_port,
    const std::uint8_t reg,
    const std::uint8_t value
) {
    bus.write_port(control_port, value);
    bus.write_port(control_port, static_cast<std::uint8_t>(0x80U | reg));
}

void write_internal_16(
    vanguard8::core::cpu::Z180Adapter& cpu,
    const std::uint8_t low_port,
    const std::uint16_t value
) {
    cpu.out0(low_port, static_cast<std::uint8_t>(value & 0x00FFU));
    cpu.out0(low_port + 1U, static_cast<std::uint8_t>((value >> 8U) & 0x00FFU));
}

void write_internal_20(
    vanguard8::core::cpu::Z180Adapter& cpu,
    const std::uint8_t low_port,
    const std::uint32_t value
) {
    cpu.out0(low_port, static_cast<std::uint8_t>(value & 0x0000FFU));
    cpu.out0(low_port + 1U, static_cast<std::uint8_t>((value >> 8U) & 0x0000FFU));
    cpu.out0(low_port + 2U, static_cast<std::uint8_t>((value >> 16U) & 0x00000FU));
}

}  // namespace

TEST_CASE("z180 adapter reset matches documented HD64180 defaults", "[cpu]") {
    vanguard8::core::Bus bus{};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    REQUIRE(cpu.pc() == 0x0000);
    REQUIRE(cpu.cbar() == 0xFF);
    REQUIRE(cpu.cbr() == 0x00);
    REQUIRE(cpu.bbr() == 0x00);
    REQUIRE(cpu.in0(0x0C) == 0xFF);
    REQUIRE(cpu.in0(0x0D) == 0xFF);
    REQUIRE(cpu.in0(0x0E) == 0xFF);
    REQUIRE(cpu.in0(0x0F) == 0xFF);
    REQUIRE(cpu.in0(0x10) == 0x00);
    REQUIRE(cpu.in0(0x14) == 0xFF);
    REQUIRE(cpu.in0(0x15) == 0xFF);
    REQUIRE(cpu.in0(0x16) == 0xFF);
    REQUIRE(cpu.in0(0x17) == 0xFF);
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

TEST_CASE("DMA channel 0 copies between physical memory regions", "[cpu]") {
    vanguard8::core::Bus bus{};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    bus.write_memory(0xF0000, 0x12);
    bus.write_memory(0xF0001, 0x34);
    bus.write_memory(0xF0002, 0x56);
    bus.write_memory(0xF0003, 0x78);

    write_internal_20(cpu, 0x20, 0xF0000);
    write_internal_20(cpu, 0x23, 0xF0010);
    write_internal_16(cpu, 0x26, 4);

    cpu.execute_dma(vanguard8::core::cpu::DmaChannel::channel0);

    REQUIRE(bus.read_memory(0xF0010) == 0x12);
    REQUIRE(bus.read_memory(0xF0011) == 0x34);
    REQUIRE(bus.read_memory(0xF0012) == 0x56);
    REQUIRE(bus.read_memory(0xF0013) == 0x78);
}

TEST_CASE("DMA channel 1 reuses the bus port dispatch path used by CPU OUT", "[cpu]") {
    vanguard8::core::Bus bus{};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    bus.write_memory(0xF0000, 0x0A);
    write_internal_20(cpu, 0x28, 0xF0000);
    write_internal_16(cpu, 0x2B, 0x0050);
    write_internal_16(cpu, 0x2E, 1);

    bus.write_port(0x50, 0x03);
    REQUIRE(bus.ay8910().selected_register() == 0x03);

    cpu.execute_dma(vanguard8::core::cpu::DmaChannel::channel1);

    REQUIRE(bus.ay8910().selected_register() == 0x0A);
    REQUIRE(cpu.in0(0x2B) == 0x50);
    REQUIRE(cpu.in0(0x2C) == 0x00);
}

TEST_CASE("DMA execution refuses unsupported mode and control values", "[cpu]") {
    vanguard8::core::Bus bus{};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    bus.write_memory(0xF0000, 0x9A);
    write_internal_20(cpu, 0x20, 0xF0000);
    write_internal_20(cpu, 0x23, 0xF0010);
    write_internal_16(cpu, 0x26, 1);
    cpu.out0(0x31, 0x01);

    cpu.execute_dma(vanguard8::core::cpu::DmaChannel::channel0);

    REQUIRE(bus.read_memory(0xF0010) == 0x00);
    REQUIRE_FALSE(bus.warnings().empty());
}

TEST_CASE("INT0 follows IM1 behavior", "[cpu]") {
    vanguard8::core::Bus bus{};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    cpu.set_interrupt_mode(1);
    cpu.set_iff1(true);
    write_vdp_register(bus, 0x81, 1, 0x20);
    bus.mutable_vdp_a().set_vblank_flag(true);
    bus.sync_vdp_interrupt_lines();

    const auto service = cpu.service_pending_interrupt();
    REQUIRE(service.has_value());
    REQUIRE(service->source == vanguard8::core::cpu::InterruptSource::int0);
    REQUIRE(service->handler_address == 0x0038);
}

TEST_CASE("VDP-B interrupt state never reaches CPU INT0", "[cpu]") {
    vanguard8::core::Bus bus{};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    cpu.set_interrupt_mode(1);
    cpu.set_iff1(true);

    write_vdp_register(bus, 0x85, 1, 0x20);
    bus.mutable_vdp_b().set_vblank_flag(true);
    bus.sync_vdp_interrupt_lines();

    REQUIRE_FALSE(bus.int0_asserted());
    REQUIRE_FALSE(cpu.service_pending_interrupt().has_value());
}

TEST_CASE("INT1 uses the HD64180 I IL vectored path independent of IM mode", "[cpu]") {
    vanguard8::core::Bus bus{};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    cpu.out0(0x3A, 0x48);
    cpu.out0(0x38, 0xF0);
    cpu.out0(0x33, 0xF8);
    cpu.out0(0x34, 0x02);
    cpu.set_register_i(0x80);
    cpu.set_interrupt_mode(0);
    cpu.set_iff1(true);

    bus.write_memory(0xF00E0, 0x34);
    bus.write_memory(0xF00E1, 0x12);
    bus.set_int1(true);

    const auto service = cpu.service_pending_interrupt();
    REQUIRE(service.has_value());
    REQUIRE(service->source == vanguard8::core::cpu::InterruptSource::int1);
    REQUIRE(service->handler_address == 0x1234);
}

TEST_CASE("PRT0 decrements every 20 CPU clocks and sets TIF0 on timeout", "[cpu]") {
    vanguard8::core::Bus bus{};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    write_internal_16(cpu, 0x0C, 3);
    write_internal_16(cpu, 0x0E, 2);
    cpu.out0(0x10, 0x01);

    cpu.advance_tstates(19);
    REQUIRE(cpu.in0(0x0C) == 0x03);
    REQUIRE((cpu.in0(0x10) & 0x40) == 0x00);

    cpu.advance_tstates(1);
    REQUIRE(cpu.in0(0x0C) == 0x02);

    cpu.advance_tstates(20);
    REQUIRE(cpu.in0(0x0C) == 0x01);

    cpu.advance_tstates(20);
    REQUIRE((cpu.in0(0x10) & 0x40) == 0x40);
}

TEST_CASE("Reading TCR then TMDR0 clears TIF0", "[cpu]") {
    vanguard8::core::Bus bus{};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    write_internal_16(cpu, 0x0C, 1);
    write_internal_16(cpu, 0x0E, 1);
    cpu.out0(0x10, 0x01);
    cpu.advance_tstates(20);

    REQUIRE((cpu.in0(0x10) & 0x40) == 0x40);
    REQUIRE((cpu.in0(0x10) & 0x40) == 0x40);

    REQUIRE(cpu.in0(0x0C) == 0x01);
    REQUIRE((cpu.in0(0x10) & 0x40) == 0x00);
}

TEST_CASE("PRT0 and PRT1 use their documented vectored interrupt table entries", "[cpu]") {
    vanguard8::core::Bus bus{};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    cpu.out0(0x3A, 0x48);
    cpu.out0(0x38, 0xF0);
    cpu.out0(0x33, 0xE0);
    cpu.set_register_i(0x80);
    cpu.set_interrupt_mode(1);
    cpu.set_iff1(true);

    bus.write_memory(0xF00E4, 0x78);
    bus.write_memory(0xF00E5, 0x56);
    bus.write_memory(0xF00E6, 0xBC);
    bus.write_memory(0xF00E7, 0x9A);

    write_internal_16(cpu, 0x0C, 1);
    write_internal_16(cpu, 0x0E, 1);
    cpu.out0(0x10, 0x11);
    cpu.advance_tstates(20);

    const auto prt0_service = cpu.service_pending_interrupt();
    REQUIRE(prt0_service.has_value());
    REQUIRE(prt0_service->source == vanguard8::core::cpu::InterruptSource::prt0);
    REQUIRE(prt0_service->handler_address == 0x5678);

    cpu.set_iff1(true);
    REQUIRE((cpu.in0(0x10) & 0x40) == 0x40);
    REQUIRE(cpu.in0(0x0C) == 0x01);

    write_internal_16(cpu, 0x14, 1);
    write_internal_16(cpu, 0x16, 1);
    cpu.out0(0x10, 0x22);
    cpu.advance_tstates(20);

    const auto prt1_service = cpu.service_pending_interrupt();
    REQUIRE(prt1_service.has_value());
    REQUIRE(prt1_service->source == vanguard8::core::cpu::InterruptSource::prt1);
    REQUIRE(prt1_service->handler_address == 0x9ABC);
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

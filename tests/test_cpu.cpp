#include <catch2/catch_test_macros.hpp>

#include "core/bus.hpp"
#include "core/cpu/z180_adapter.hpp"

#include <algorithm>
#include <functional>
#include <vector>

namespace {

constexpr std::uint8_t flag_sign = 0x80;
constexpr std::uint8_t flag_zero = 0x40;
constexpr std::uint8_t flag_half = 0x10;
constexpr std::uint8_t flag_parity_overflow = 0x04;
constexpr std::uint8_t flag_subtract = 0x02;
constexpr std::uint8_t flag_carry = 0x01;

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

auto make_instruction_test_rom(std::initializer_list<std::uint8_t> program) -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);
    std::copy(program.begin(), program.end(), rom.begin());
    return rom;
}

auto make_halt_wake_test_rom() -> std::vector<std::uint8_t> {
    // HALT at 0x0000; the instruction at 0x0001 is the documented resume
    // target. 0x0038 holds a RETI so an INT0 IM1 service can return through
    // it. 0x0002 is a second HALT for bounded `run_until_halt`-style drivers.
    std::vector<std::uint8_t> rom(
        vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);
    rom[0x0000] = 0x76;  // HALT
    rom[0x0001] = 0x00;  // NOP (resume target)
    rom[0x0002] = 0x76;  // HALT
    rom[0x0038] = 0xED;  // RETI
    rom[0x0039] = 0x4D;
    return rom;
}

auto run_test_program(
    const std::vector<std::uint8_t>& rom,
    const vanguard8::third_party::z180::RegisterSnapshot& initial_state,
    const std::size_t max_instructions = 8
) -> vanguard8::core::cpu::CpuStateSnapshot {
    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    auto state = cpu.state_snapshot();
    state.registers = initial_state;
    cpu.load_state_snapshot(state);
    cpu.run_until_halt(max_instructions);
    return cpu.state_snapshot();
}

auto default_register_snapshot() -> vanguard8::third_party::z180::RegisterSnapshot {
    vanguard8::core::Bus bus{};
    vanguard8::core::cpu::Z180Adapter cpu{bus};
    return cpu.state_snapshot().registers;
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

TEST_CASE("HD64180 MLT multiplies the covered register pairs in place", "[cpu]") {
    auto initial = default_register_snapshot();
    initial.bc = 0x0304;
    const auto bc_result = run_test_program(make_instruction_test_rom({0xED, 0x4C, 0x76}), initial);
    REQUIRE(bc_result.registers.bc == 0x000C);

    initial = default_register_snapshot();
    initial.de = 0x0506;
    const auto de_result = run_test_program(make_instruction_test_rom({0xED, 0x5C, 0x76}), initial);
    REQUIRE(de_result.registers.de == 0x001E);

    initial = default_register_snapshot();
    initial.hl = 0x0708;
    const auto hl_result = run_test_program(make_instruction_test_rom({0xED, 0x6C, 0x76}), initial);
    REQUIRE(hl_result.registers.hl == 0x0038);

    initial = default_register_snapshot();
    initial.sp = 0x090A;
    const auto sp_result = run_test_program(make_instruction_test_rom({0xED, 0x7C, 0x76}), initial);
    REQUIRE(sp_result.registers.sp == 0x005A);
}

TEST_CASE("HD64180 TST updates flags from the masked result without changing A", "[cpu]") {
    auto initial = default_register_snapshot();
    initial.af = 0xF0FF;
    initial.bc = 0x0F00;

    const auto reg_result = run_test_program(make_instruction_test_rom({0xED, 0x04, 0x76}), initial);
    REQUIRE(static_cast<std::uint8_t>(reg_result.registers.af >> 8U) == 0xF0);
    REQUIRE((reg_result.registers.af & 0x00FFU) == static_cast<std::uint16_t>(flag_zero | flag_half | flag_parity_overflow));

    initial = default_register_snapshot();
    initial.af = 0x81FF;
    const auto imm_result = run_test_program(make_instruction_test_rom({0xED, 0x64, 0x80, 0x76}), initial);
    REQUIRE(static_cast<std::uint8_t>(imm_result.registers.af >> 8U) == 0x81);
    REQUIRE((imm_result.registers.af & 0x00FFU) == static_cast<std::uint16_t>(flag_sign | flag_half));
    REQUIRE((imm_result.registers.af & flag_zero) == 0U);
    REQUIRE((imm_result.registers.af & flag_subtract) == 0U);
    REQUIRE((imm_result.registers.af & flag_carry) == 0U);
}

TEST_CASE("HD64180 IN0 and OUT0 cover non-A register variants and non-MMU internal ports", "[cpu]") {
    auto initial = default_register_snapshot();
    initial.il = 0xA5;
    initial.itc = 0x12;

    const auto result = run_test_program(
        make_instruction_test_rom({
            0xED, 0x00, 0x33,  // IN0 B,(0x33)
            0xED, 0x08, 0x34,  // IN0 C,(0x34)
            0xED, 0x01, 0x38,  // OUT0 (0x38),B
            0xED, 0x09, 0x39,  // OUT0 (0x39),C
            0x76,
        }),
        initial,
        16
    );

    REQUIRE(static_cast<std::uint8_t>(result.registers.bc >> 8U) == 0xA5);
    REQUIRE(static_cast<std::uint8_t>(result.registers.bc & 0x00FFU) == 0x12);
    REQUIRE(result.registers.cbr == 0xA5);
    REQUIRE(result.registers.bbr == 0x12);
}

TEST_CASE("extracted CPU supports plain OUT XOR A and JR used by ROM bring-up", "[cpu]") {
    const auto rom = make_instruction_test_rom({
        0x3E, 0x0A,       // LD A,0x0A
        0xD3, 0x50,       // OUT (0x50),A
        0xAF,             // XOR A
        0x18, 0x01,       // JR +1
        0x00,             // NOP (skipped)
        0x76,             // HALT
    });

    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};
    cpu.run_until_halt(8);

    REQUIRE(bus.ay8910().selected_register() == 0x0A);
    REQUIRE(cpu.accumulator() == 0x00);
    REQUIRE(cpu.halted());
}

TEST_CASE("scheduled CPU covers LD B,n used by the Pac-Man boot path", "[cpu]") {
    const auto rom = make_instruction_test_rom({
        0x06, 0x12,  // LD B,0x12
        0x76,        // HALT
    });

    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    REQUIRE(cpu.peek_logical(cpu.pc()) == 0x06);
    REQUIRE(cpu.next_scheduled_tstates() == 7);
    REQUIRE(cpu.step_scheduled_instruction() == 7);
    REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.bc >> 8U) == 0x12);
    REQUIRE(cpu.pc() == 0x0002);
}

TEST_CASE("scheduled CPU covers LD C,n used by the PacManV8 boot path", "[cpu]") {
    const auto rom = make_instruction_test_rom({
        0x0E, 0x34,  // LD C,0x34
        0x76,        // HALT
    });

    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    REQUIRE(cpu.peek_logical(cpu.pc()) == 0x0E);
    REQUIRE(cpu.next_scheduled_tstates() == 7);
    REQUIRE(cpu.step_scheduled_instruction() == 7);
    REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.bc & 0x00FFU) == 0x34);
    REQUIRE(cpu.pc() == 0x0002);
}

TEST_CASE("scheduled CPU covers LD D,n used by the PacManV8 boot path", "[cpu]") {
    const auto rom = make_instruction_test_rom({
        0x16, 0x56,  // LD D,0x56
        0x76,        // HALT
    });

    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    REQUIRE(cpu.peek_logical(cpu.pc()) == 0x16);
    REQUIRE(cpu.next_scheduled_tstates() == 7);
    REQUIRE(cpu.step_scheduled_instruction() == 7);
    REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.de >> 8U) == 0x56);
    REQUIRE(cpu.pc() == 0x0002);
}

TEST_CASE("scheduled CPU covers LD E,n used by the PacManV8 T020 intermission panel path", "[cpu]") {
    const auto rom = make_instruction_test_rom({
        0x1E, 0x50,  // LD E,0x50
        0x76,        // HALT
    });

    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    auto state = cpu.state_snapshot();
    const auto preserved_flags = static_cast<std::uint8_t>(
        flag_sign | flag_zero | flag_half | flag_parity_overflow | flag_subtract | flag_carry
    );
    state.registers.af = static_cast<std::uint16_t>(0xAA00U | preserved_flags);
    state.registers.de = 0x4400;
    state.registers.hl = 0xBEEF;
    state.registers.bc = 0x2418;
    cpu.load_state_snapshot(state);

    REQUIRE(cpu.peek_logical(cpu.pc()) == 0x1E);
    REQUIRE(cpu.next_scheduled_tstates() == 7);
    REQUIRE(cpu.step_scheduled_instruction() == 7);

    const auto after = cpu.state_snapshot();
    REQUIRE(static_cast<std::uint8_t>(after.registers.de & 0x00FFU) == 0x50);
    REQUIRE(static_cast<std::uint8_t>(after.registers.de >> 8U) == 0x44);
    REQUIRE(after.registers.hl == 0xBEEF);
    REQUIRE(after.registers.bc == 0x2418);
    REQUIRE(static_cast<std::uint8_t>(after.registers.af & 0x00FFU) == preserved_flags);
    REQUIRE(cpu.pc() == 0x0002);
}

TEST_CASE("scheduled CPU covers LD H,n used by the PacManV8 T020 intermission panel path", "[cpu]") {
    const auto rom = make_instruction_test_rom({
        0x26, 0x7F,  // LD H,0x7F
        0x76,        // HALT
    });

    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    auto state = cpu.state_snapshot();
    const auto preserved_flags = static_cast<std::uint8_t>(
        flag_sign | flag_zero | flag_half | flag_parity_overflow | flag_subtract | flag_carry
    );
    state.registers.af = static_cast<std::uint16_t>(0xAA00U | preserved_flags);
    state.registers.hl = 0x00A5;
    state.registers.de = 0xCAFE;
    state.registers.bc = 0x1234;
    cpu.load_state_snapshot(state);

    REQUIRE(cpu.peek_logical(cpu.pc()) == 0x26);
    REQUIRE(cpu.next_scheduled_tstates() == 7);
    REQUIRE(cpu.step_scheduled_instruction() == 7);

    const auto after = cpu.state_snapshot();
    REQUIRE(static_cast<std::uint8_t>(after.registers.hl >> 8U) == 0x7F);
    REQUIRE(static_cast<std::uint8_t>(after.registers.hl & 0x00FFU) == 0xA5);
    REQUIRE(after.registers.de == 0xCAFE);
    REQUIRE(after.registers.bc == 0x1234);
    REQUIRE(static_cast<std::uint8_t>(after.registers.af & 0x00FFU) == preserved_flags);
    REQUIRE(cpu.pc() == 0x0002);
}

TEST_CASE("scheduled CPU covers LD L,n used by the PacManV8 T020 intermission panel path", "[cpu]") {
    const auto rom = make_instruction_test_rom({
        0x2E, 0xC3,  // LD L,0xC3
        0x76,        // HALT
    });

    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    auto state = cpu.state_snapshot();
    const auto preserved_flags = static_cast<std::uint8_t>(
        flag_sign | flag_zero | flag_half | flag_parity_overflow | flag_subtract | flag_carry
    );
    state.registers.af = static_cast<std::uint16_t>(0xAA00U | preserved_flags);
    state.registers.hl = 0x7F00;
    state.registers.de = 0xCAFE;
    state.registers.bc = 0x1234;
    cpu.load_state_snapshot(state);

    REQUIRE(cpu.peek_logical(cpu.pc()) == 0x2E);
    REQUIRE(cpu.next_scheduled_tstates() == 7);
    REQUIRE(cpu.step_scheduled_instruction() == 7);

    const auto after = cpu.state_snapshot();
    REQUIRE(static_cast<std::uint8_t>(after.registers.hl & 0x00FFU) == 0xC3);
    REQUIRE(static_cast<std::uint8_t>(after.registers.hl >> 8U) == 0x7F);
    REQUIRE(after.registers.de == 0xCAFE);
    REQUIRE(after.registers.bc == 0x1234);
    REQUIRE(static_cast<std::uint8_t>(after.registers.af & 0x00FFU) == preserved_flags);
    REQUIRE(cpu.pc() == 0x0002);
}

TEST_CASE("scheduled CPU covers the PacManV8 T020 LD D,n -> LD E,n -> LD A,n rectangle-fill prologue",
          "[cpu]") {
    // Mirrors the exact pattern at PacManV8 0x332C..0x3331:
    //   LD D,0x44  ; height
    //   LD E,0x50  ; width
    //   LD A,0x22  ; fill byte
    const auto rom = make_instruction_test_rom({
        0x16, 0x44,  // LD D,0x44
        0x1E, 0x50,  // LD E,0x50
        0x3E, 0x22,  // LD A,0x22
        0x76,        // HALT
    });

    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    REQUIRE(cpu.step_scheduled_instruction() == 7);
    REQUIRE(cpu.step_scheduled_instruction() == 7);
    REQUIRE(cpu.step_scheduled_instruction() == 7);

    const auto after = cpu.state_snapshot();
    REQUIRE(static_cast<std::uint8_t>(after.registers.de >> 8U) == 0x44);
    REQUIRE(static_cast<std::uint8_t>(after.registers.de & 0x00FFU) == 0x50);
    REQUIRE(static_cast<std::uint8_t>(after.registers.af >> 8U) == 0x22);
    REQUIRE(cpu.pc() == 0x0006);
}

TEST_CASE("scheduled CPU covers LD BC,nn used by the PacManV8 VDP-B bring-up path", "[cpu]") {
    const auto rom = make_instruction_test_rom({
        0x01, 0x34, 0x12,  // LD BC,0x1234
        0x76,              // HALT
    });

    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    auto state = cpu.state_snapshot();
    const auto preserved_flags = static_cast<std::uint8_t>(
        flag_sign | flag_zero | flag_half | flag_parity_overflow | flag_subtract | flag_carry
    );
    state.registers.af = static_cast<std::uint16_t>(0xAA00U | preserved_flags);
    state.registers.bc = 0xFFFF;
    cpu.load_state_snapshot(state);

    REQUIRE(cpu.peek_logical(cpu.pc()) == 0x01);
    REQUIRE(cpu.next_scheduled_tstates() == 10);
    REQUIRE(cpu.step_scheduled_instruction() == 10);
    REQUIRE(cpu.state_snapshot().registers.bc == 0x1234);
    REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) == preserved_flags);
    REQUIRE(cpu.pc() == 0x0003);
}

TEST_CASE("scheduled CPU covers OR n used by the PacManV8 VDP-B VRAM seek path", "[cpu]") {
    SECTION("typical: nonzero sign-clear result with even parity") {
        const auto rom = make_instruction_test_rom({
            0xF6, 0x33,  // OR 0x33
            0x76,        // HALT
        });

        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};

        auto state = cpu.state_snapshot();
        state.registers.af = static_cast<std::uint16_t>(
            0x0C00U | flag_half | flag_subtract | flag_carry
        );
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.peek_logical(cpu.pc()) == 0xF6);
        REQUIRE(cpu.next_scheduled_tstates() == 7);
        REQUIRE(cpu.step_scheduled_instruction() == 7);
        REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.af >> 8U) == 0x3F);
        REQUIRE(
            static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) ==
            flag_parity_overflow
        );
        REQUIRE(cpu.pc() == 0x0002);
    }

    SECTION("zero result sets Z and parity, clears S/H/N/C") {
        const auto rom = make_instruction_test_rom({
            0xF6, 0x00,  // OR 0x00
            0x76,        // HALT
        });

        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};

        auto state = cpu.state_snapshot();
        state.registers.af = static_cast<std::uint16_t>(
            0x0000U | flag_sign | flag_half | flag_subtract | flag_carry
        );
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.step_scheduled_instruction() == 7);
        REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.af >> 8U) == 0x00);
        REQUIRE(
            static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) ==
            static_cast<std::uint8_t>(flag_zero | flag_parity_overflow)
        );
    }

    SECTION("bit 7 result sets S, odd parity clears P/V") {
        const auto rom = make_instruction_test_rom({
            0xF6, 0x80,  // OR 0x80
            0x76,        // HALT
        });

        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};

        auto state = cpu.state_snapshot();
        state.registers.af = static_cast<std::uint16_t>(
            0x0000U | flag_zero | flag_half | flag_subtract | flag_carry
        );
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.step_scheduled_instruction() == 7);
        REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.af >> 8U) == 0x80);
        REQUIRE(
            static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) == flag_sign
        );
    }
}

TEST_CASE("scheduled CPU covers LD A,B used by the PacManV8 VDP-B VRAM seek path", "[cpu]") {
    const auto rom = make_instruction_test_rom({
        0x78,  // LD A,B
        0x76,  // HALT
    });

    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    auto state = cpu.state_snapshot();
    const auto preserved_flags = static_cast<std::uint8_t>(
        flag_sign | flag_zero | flag_half | flag_parity_overflow | flag_subtract | flag_carry
    );
    state.registers.af = static_cast<std::uint16_t>(0x3300U | preserved_flags);
    state.registers.bc = 0x7CDE;
    cpu.load_state_snapshot(state);

    REQUIRE(cpu.peek_logical(cpu.pc()) == 0x78);
    REQUIRE(cpu.next_scheduled_tstates() == 4);
    REQUIRE(cpu.step_scheduled_instruction() == 4);
    REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.af >> 8U) == 0x7C);
    REQUIRE(cpu.state_snapshot().registers.bc == 0x7CDE);
    REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) == preserved_flags);
    REQUIRE(cpu.pc() == 0x0001);
}

TEST_CASE("scheduled CPU covers LD A,C used by the PacManV8 VDP-B VRAM seek path", "[cpu]") {
    const auto rom = make_instruction_test_rom({
        0x79,  // LD A,C
        0x76,  // HALT
    });

    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    auto state = cpu.state_snapshot();
    const auto preserved_flags = static_cast<std::uint8_t>(
        flag_sign | flag_zero | flag_half | flag_parity_overflow | flag_subtract | flag_carry
    );
    state.registers.af = static_cast<std::uint16_t>(0x1100U | preserved_flags);
    state.registers.bc = 0xBBA5;
    cpu.load_state_snapshot(state);

    REQUIRE(cpu.peek_logical(cpu.pc()) == 0x79);
    REQUIRE(cpu.next_scheduled_tstates() == 4);
    REQUIRE(cpu.step_scheduled_instruction() == 4);
    REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.af >> 8U) == 0xA5);
    REQUIRE(cpu.state_snapshot().registers.bc == 0xBBA5);
    REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) == preserved_flags);
    REQUIRE(cpu.pc() == 0x0001);
}

TEST_CASE("scheduled CPU covers LD DE,nn used by the PacManV8 VDP-B framebuffer-load path", "[cpu]") {
    const auto rom = make_instruction_test_rom({
        0x11, 0xCD, 0xAB,  // LD DE,0xABCD
        0x76,              // HALT
    });

    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    auto state = cpu.state_snapshot();
    const auto preserved_flags = static_cast<std::uint8_t>(
        flag_sign | flag_zero | flag_half | flag_parity_overflow | flag_subtract | flag_carry
    );
    state.registers.af = static_cast<std::uint16_t>(0x5500U | preserved_flags);
    state.registers.de = 0xFFFF;
    cpu.load_state_snapshot(state);

    REQUIRE(cpu.peek_logical(cpu.pc()) == 0x11);
    REQUIRE(cpu.next_scheduled_tstates() == 10);
    REQUIRE(cpu.step_scheduled_instruction() == 10);
    REQUIRE(cpu.state_snapshot().registers.de == 0xABCD);
    REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) == preserved_flags);
    REQUIRE(cpu.pc() == 0x0003);
}

TEST_CASE("scheduled CPU covers LD A,D used by the PacManV8 VDP-B framebuffer copy path", "[cpu]") {
    const auto rom = make_instruction_test_rom({
        0x7A,  // LD A,D
        0x76,  // HALT
    });

    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    auto state = cpu.state_snapshot();
    const auto preserved_flags = static_cast<std::uint8_t>(
        flag_sign | flag_zero | flag_half | flag_parity_overflow | flag_subtract | flag_carry
    );
    state.registers.af = static_cast<std::uint16_t>(0x2200U | preserved_flags);
    state.registers.de = 0x9C40;
    cpu.load_state_snapshot(state);

    REQUIRE(cpu.peek_logical(cpu.pc()) == 0x7A);
    REQUIRE(cpu.next_scheduled_tstates() == 4);
    REQUIRE(cpu.step_scheduled_instruction() == 4);
    REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.af >> 8U) == 0x9C);
    REQUIRE(cpu.state_snapshot().registers.de == 0x9C40);
    REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) == preserved_flags);
    REQUIRE(cpu.pc() == 0x0001);
}

TEST_CASE("scheduled CPU covers OR E used by the PacManV8 VDP-B framebuffer copy path", "[cpu]") {
    SECTION("typical: nonzero sign-clear result with even parity") {
        const auto rom = make_instruction_test_rom({
            0xB3,  // OR E
            0x76,  // HALT
        });

        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};

        auto state = cpu.state_snapshot();
        state.registers.af = static_cast<std::uint16_t>(
            0x0C00U | flag_half | flag_subtract | flag_carry
        );
        state.registers.de = 0x0033;
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.peek_logical(cpu.pc()) == 0xB3);
        REQUIRE(cpu.next_scheduled_tstates() == 4);
        REQUIRE(cpu.step_scheduled_instruction() == 4);
        REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.af >> 8U) == 0x3F);
        REQUIRE(cpu.state_snapshot().registers.de == 0x0033);
        REQUIRE(
            static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) ==
            flag_parity_overflow
        );
        REQUIRE(cpu.pc() == 0x0001);
    }

    SECTION("zero result sets Z and parity, clears S/H/N/C") {
        const auto rom = make_instruction_test_rom({
            0xB3,  // OR E
            0x76,  // HALT
        });

        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};

        auto state = cpu.state_snapshot();
        state.registers.af = static_cast<std::uint16_t>(
            0x0000U | flag_sign | flag_half | flag_subtract | flag_carry
        );
        state.registers.de = 0x0000;
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.step_scheduled_instruction() == 4);
        REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.af >> 8U) == 0x00);
        REQUIRE(
            static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) ==
            static_cast<std::uint8_t>(flag_zero | flag_parity_overflow)
        );
    }

    SECTION("bit 7 result sets S, odd parity clears P/V") {
        const auto rom = make_instruction_test_rom({
            0xB3,  // OR E
            0x76,  // HALT
        });

        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};

        auto state = cpu.state_snapshot();
        state.registers.af = static_cast<std::uint16_t>(
            0x0000U | flag_zero | flag_half | flag_subtract | flag_carry
        );
        state.registers.de = 0x0080;
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.step_scheduled_instruction() == 4);
        REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.af >> 8U) == 0x80);
        REQUIRE(
            static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) == flag_sign
        );
    }
}

TEST_CASE("scheduled CPU covers DEC DE used by the PacManV8 VDP-B framebuffer copy path", "[cpu]") {
    SECTION("typical decrement leaves flags untouched and advances PC by one in 6 T-states") {
        const auto rom = make_instruction_test_rom({
            0x1B,  // DEC DE
            0x76,  // HALT
        });

        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};

        auto state = cpu.state_snapshot();
        const auto preserved_flags = static_cast<std::uint8_t>(
            flag_sign | flag_zero | flag_half | flag_parity_overflow | flag_subtract | flag_carry
        );
        state.registers.af = static_cast<std::uint16_t>(0x1100U | preserved_flags);
        state.registers.de = 0x1234;
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.peek_logical(cpu.pc()) == 0x1B);
        REQUIRE(cpu.next_scheduled_tstates() == 6);
        REQUIRE(cpu.step_scheduled_instruction() == 6);
        REQUIRE(cpu.state_snapshot().registers.de == 0x1233);
        REQUIRE(
            static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) == preserved_flags
        );
        REQUIRE(cpu.pc() == 0x0001);
    }

    SECTION("decrement from 0x0000 wraps to 0xFFFF without touching flags") {
        const auto rom = make_instruction_test_rom({
            0x1B,  // DEC DE
            0x76,  // HALT
        });

        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};

        auto state = cpu.state_snapshot();
        const auto preserved_flags = static_cast<std::uint8_t>(flag_carry);
        state.registers.af = static_cast<std::uint16_t>(0x0000U | preserved_flags);
        state.registers.de = 0x0000;
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.step_scheduled_instruction() == 6);
        REQUIRE(cpu.state_snapshot().registers.de == 0xFFFF);
        REQUIRE(
            static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) == preserved_flags
        );
    }
}

TEST_CASE("scheduled CPU covers RET Z used by the PacManV8 VDP-B framebuffer copy path", "[cpu]") {
    SECTION("taken path (Z=1) pops PC from SP, advances SP by two, costs 11 T-states, flags preserved") {
        const auto rom = make_instruction_test_rom({
            0xC8,  // RET Z
            0x76,  // HALT
        });

        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};

        // Boot MMU: logical 0x8000-0xFFFF maps to physical 0xF0000+ (SRAM).
        auto state = cpu.state_snapshot();
        state.registers.cbar = 0x48;
        state.registers.cbr = 0xF0;
        state.registers.bbr = 0x04;
        const auto preserved_flags = static_cast<std::uint8_t>(
            flag_sign | flag_zero | flag_half | flag_parity_overflow | flag_subtract | flag_carry
        );
        state.registers.af = static_cast<std::uint16_t>(0x0000U | preserved_flags);
        state.registers.sp = 0x8100;
        cpu.load_state_snapshot(state);

        // Return target: low byte 0x10, high byte 0x00 => PC <- 0x0010 (a HALT-free byte,
        // but we only inspect PC/SP/timing; the instruction fetch at 0x0010 does not run).
        bus.write_memory(0xF0100, 0x10);
        bus.write_memory(0xF0101, 0x00);

        REQUIRE(cpu.peek_logical(cpu.pc()) == 0xC8);
        REQUIRE(cpu.next_scheduled_tstates() == 11);
        REQUIRE(cpu.step_scheduled_instruction() == 11);
        REQUIRE(cpu.pc() == 0x0010);
        REQUIRE(cpu.state_snapshot().registers.sp == 0x8102);
        REQUIRE(
            static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) == preserved_flags
        );
    }

    SECTION("not-taken path (Z=0) advances PC by one, leaves SP unchanged, costs 5 T-states, flags preserved") {
        const auto rom = make_instruction_test_rom({
            0xC8,  // RET Z
            0x76,  // HALT
        });

        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};

        auto state = cpu.state_snapshot();
        const auto preserved_flags = static_cast<std::uint8_t>(
            flag_sign | flag_half | flag_parity_overflow | flag_subtract | flag_carry
        );  // Z cleared
        state.registers.af = static_cast<std::uint16_t>(0x0000U | preserved_flags);
        state.registers.sp = 0x8100;
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.peek_logical(cpu.pc()) == 0xC8);
        REQUIRE(cpu.next_scheduled_tstates() == 5);
        REQUIRE(cpu.step_scheduled_instruction() == 5);
        REQUIRE(cpu.pc() == 0x0001);
        REQUIRE(cpu.state_snapshot().registers.sp == 0x8100);
        REQUIRE(
            static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) == preserved_flags
        );
    }
}

TEST_CASE("scheduled CPU covers PUSH rr used by the PacManV8 VBlank handler prologue", "[cpu]") {
    // Boot MMU: logical 0x8000-0xFFFF maps to physical 0xF0000+ (SRAM).
    const auto install_sram_stack = [](vanguard8::core::cpu::Z180Adapter& cpu) {
        auto state = cpu.state_snapshot();
        state.registers.cbar = 0x48;
        state.registers.cbr = 0xF0;
        state.registers.bbr = 0x04;
        state.registers.sp = 0x8100;
        const auto preserved_flags = static_cast<std::uint8_t>(
            flag_sign | flag_zero | flag_half | flag_parity_overflow | flag_subtract | flag_carry
        );
        state.registers.af = static_cast<std::uint16_t>(0xAA00U | preserved_flags);
        cpu.load_state_snapshot(state);
        return preserved_flags;
    };

    SECTION("PUSH BC (0xC5) predecrements SP, writes BC to the stack, 11 T-states, flags preserved") {
        const auto rom = make_instruction_test_rom({0xC5, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        const auto preserved_flags = install_sram_stack(cpu);
        auto state = cpu.state_snapshot();
        state.registers.bc = 0x1234;
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.peek_logical(cpu.pc()) == 0xC5);
        REQUIRE(cpu.next_scheduled_tstates() == 11);
        REQUIRE(cpu.step_scheduled_instruction() == 11);
        REQUIRE(cpu.state_snapshot().registers.sp == 0x80FE);
        REQUIRE(bus.read_memory(0xF00FE) == 0x34);  // low byte at SP
        REQUIRE(bus.read_memory(0xF00FF) == 0x12);  // high byte at SP+1
        REQUIRE(cpu.state_snapshot().registers.bc == 0x1234);
        REQUIRE(
            static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) == preserved_flags
        );
        REQUIRE(cpu.pc() == 0x0001);
    }

    SECTION("PUSH DE (0xD5) predecrements SP, writes DE to the stack, 11 T-states, flags preserved") {
        const auto rom = make_instruction_test_rom({0xD5, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        const auto preserved_flags = install_sram_stack(cpu);
        auto state = cpu.state_snapshot();
        state.registers.de = 0x5678;
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.next_scheduled_tstates() == 11);
        REQUIRE(cpu.step_scheduled_instruction() == 11);
        REQUIRE(cpu.state_snapshot().registers.sp == 0x80FE);
        REQUIRE(bus.read_memory(0xF00FE) == 0x78);
        REQUIRE(bus.read_memory(0xF00FF) == 0x56);
        REQUIRE(cpu.state_snapshot().registers.de == 0x5678);
        REQUIRE(
            static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) == preserved_flags
        );
    }

    SECTION("PUSH HL (0xE5) predecrements SP, writes HL to the stack, 11 T-states, flags preserved") {
        const auto rom = make_instruction_test_rom({0xE5, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        const auto preserved_flags = install_sram_stack(cpu);
        auto state = cpu.state_snapshot();
        state.registers.hl = 0x9ABC;
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.next_scheduled_tstates() == 11);
        REQUIRE(cpu.step_scheduled_instruction() == 11);
        REQUIRE(cpu.state_snapshot().registers.sp == 0x80FE);
        REQUIRE(bus.read_memory(0xF00FE) == 0xBC);
        REQUIRE(bus.read_memory(0xF00FF) == 0x9A);
        REQUIRE(cpu.state_snapshot().registers.hl == 0x9ABC);
        REQUIRE(
            static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) == preserved_flags
        );
    }

    SECTION("PUSH AF (0xF5) remains covered at 11 T-states after the new opcodes land") {
        const auto rom = make_instruction_test_rom({0xF5, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        install_sram_stack(cpu);

        REQUIRE(cpu.next_scheduled_tstates() == 11);
        REQUIRE(cpu.step_scheduled_instruction() == 11);
        REQUIRE(cpu.state_snapshot().registers.sp == 0x80FE);
    }
}

TEST_CASE("scheduled CPU covers POP rr used by the PacManV8 VBlank handler epilogue", "[cpu]") {
    // Install SRAM stack with a two-byte value already staged at 0x8100/0x8101.
    const auto install_sram_stack_with_value =
        [](vanguard8::core::Bus& bus, vanguard8::core::cpu::Z180Adapter& cpu,
           std::uint8_t low_byte, std::uint8_t high_byte) {
            auto state = cpu.state_snapshot();
            state.registers.cbar = 0x48;
            state.registers.cbr = 0xF0;
            state.registers.bbr = 0x04;
            state.registers.sp = 0x8100;
            const auto preserved_flags = static_cast<std::uint8_t>(
                flag_sign | flag_zero | flag_half | flag_parity_overflow | flag_subtract | flag_carry
            );
            state.registers.af = static_cast<std::uint16_t>(0xAA00U | preserved_flags);
            cpu.load_state_snapshot(state);
            bus.write_memory(0xF0100, low_byte);
            bus.write_memory(0xF0101, high_byte);
            return preserved_flags;
        };

    SECTION("POP BC (0xC1) loads BC from the stack, post-increments SP, 10 T-states, flags preserved") {
        const auto rom = make_instruction_test_rom({0xC1, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        const auto preserved_flags = install_sram_stack_with_value(bus, cpu, 0x34, 0x12);

        REQUIRE(cpu.peek_logical(cpu.pc()) == 0xC1);
        REQUIRE(cpu.next_scheduled_tstates() == 10);
        REQUIRE(cpu.step_scheduled_instruction() == 10);
        REQUIRE(cpu.state_snapshot().registers.bc == 0x1234);
        REQUIRE(cpu.state_snapshot().registers.sp == 0x8102);
        REQUIRE(
            static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) == preserved_flags
        );
        REQUIRE(cpu.pc() == 0x0001);
    }

    SECTION("POP DE (0xD1) loads DE from the stack, post-increments SP, 10 T-states, flags preserved") {
        const auto rom = make_instruction_test_rom({0xD1, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        const auto preserved_flags = install_sram_stack_with_value(bus, cpu, 0x78, 0x56);

        REQUIRE(cpu.next_scheduled_tstates() == 10);
        REQUIRE(cpu.step_scheduled_instruction() == 10);
        REQUIRE(cpu.state_snapshot().registers.de == 0x5678);
        REQUIRE(cpu.state_snapshot().registers.sp == 0x8102);
        REQUIRE(
            static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) == preserved_flags
        );
    }

    SECTION("POP HL (0xE1) loads HL from the stack, post-increments SP, 10 T-states, flags preserved") {
        const auto rom = make_instruction_test_rom({0xE1, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        const auto preserved_flags = install_sram_stack_with_value(bus, cpu, 0xBC, 0x9A);

        REQUIRE(cpu.next_scheduled_tstates() == 10);
        REQUIRE(cpu.step_scheduled_instruction() == 10);
        REQUIRE(cpu.state_snapshot().registers.hl == 0x9ABC);
        REQUIRE(cpu.state_snapshot().registers.sp == 0x8102);
        REQUIRE(
            static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) == preserved_flags
        );
    }

    SECTION("POP AF (0xF1) remains covered at 10 T-states after the new opcodes land") {
        const auto rom = make_instruction_test_rom({0xF1, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        install_sram_stack_with_value(bus, cpu, 0x55, 0xAA);

        REQUIRE(cpu.next_scheduled_tstates() == 10);
        REQUIRE(cpu.step_scheduled_instruction() == 10);
        REQUIRE(cpu.state_snapshot().registers.af == 0xAA55);
        REQUIRE(cpu.state_snapshot().registers.sp == 0x8102);
    }
}

TEST_CASE("scheduled CPU covers the full PacManV8 VBlank prologue/epilogue PUSH/POP sequence", "[cpu]") {
    // Prologue (PC 0x0038..0x003B): PUSH AF; PUSH BC; PUSH DE; PUSH HL;
    // Epilogue (PC 0x003C..0x003F): POP HL;  POP DE;  POP BC;  POP AF;
    // Sequencing the four pushes then the four pops in reverse order must restore
    // AF/BC/DE/HL byte-for-byte and leave SP where it started.
    const auto rom = make_instruction_test_rom({
        0xF5,  // PUSH AF
        0xC5,  // PUSH BC
        0xD5,  // PUSH DE
        0xE5,  // PUSH HL
        0xE1,  // POP HL
        0xD1,  // POP DE
        0xC1,  // POP BC
        0xF1,  // POP AF
        0x76,  // HALT
    });
    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    auto state = cpu.state_snapshot();
    state.registers.cbar = 0x48;
    state.registers.cbr = 0xF0;
    state.registers.bbr = 0x04;
    state.registers.sp = 0x8100;
    state.registers.af = 0x11AA;
    state.registers.bc = 0x2233;
    state.registers.de = 0x4455;
    state.registers.hl = 0x6677;
    cpu.load_state_snapshot(state);

    cpu.run_until_halt(16);

    const auto final_state = cpu.state_snapshot();
    REQUIRE(cpu.halted());
    REQUIRE(final_state.registers.sp == 0x8100);
    REQUIRE(final_state.registers.af == 0x11AA);
    REQUIRE(final_state.registers.bc == 0x2233);
    REQUIRE(final_state.registers.de == 0x4455);
    REQUIRE(final_state.registers.hl == 0x6677);
}

TEST_CASE("scheduled CPU covers DEC B used by the Pac-Man palette VCLK path", "[cpu]") {
    const auto rom = make_instruction_test_rom({
        0x05,  // DEC B
        0x76,  // HALT
    });

    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    auto state = cpu.state_snapshot();
    state.registers.bc = 0x1000;
    state.registers.af = static_cast<std::uint16_t>(0x2200U | flag_carry);
    cpu.load_state_snapshot(state);

    REQUIRE(cpu.peek_logical(cpu.pc()) == 0x05);
    REQUIRE(cpu.next_scheduled_tstates() == 4);
    REQUIRE(cpu.step_scheduled_instruction() == 4);

    const auto result = cpu.state_snapshot();
    REQUIRE(static_cast<std::uint8_t>(result.registers.bc >> 8U) == 0x0F);
    REQUIRE(static_cast<std::uint8_t>(result.registers.bc & 0x00FFU) == 0x00);
    REQUIRE((result.registers.af & 0x00FFU) == static_cast<std::uint16_t>(flag_half | flag_subtract | flag_carry));
    REQUIRE(cpu.pc() == 0x0001);
}

TEST_CASE("scheduled CPU covers IM 1 and keeps INT1 mode-independent", "[cpu]") {
    const auto rom = make_instruction_test_rom({
        0xED, 0x56,  // IM 1
        0x76,        // HALT
    });

    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    cpu.out0(0x3A, 0x48);
    cpu.out0(0x38, 0xF0);
    cpu.out0(0x33, 0xE0);
    cpu.out0(0x34, 0x02);
    cpu.set_register_i(0x80);
    cpu.set_iff1(true);

    REQUIRE(cpu.interrupt_mode() == 0);
    REQUIRE(cpu.next_scheduled_tstates() == 8);
    REQUIRE(cpu.step_scheduled_instruction() == 8);
    REQUIRE(cpu.interrupt_mode() == 1);

    bus.write_memory(0xF00E0, 0x34);
    bus.write_memory(0xF00E1, 0x12);
    bus.set_int1(true);

    const auto service = cpu.service_pending_interrupt();
    REQUIRE(service.has_value());
    REQUIRE(service->source == vanguard8::core::cpu::InterruptSource::int1);
    REQUIRE(service->handler_address == 0x1234);
}

TEST_CASE("extracted CPU covers ROM audio nibble-fetch opcodes", "[cpu]") {
    const auto rom = make_instruction_test_rom({
        0x31, 0x00, 0xFF,             // LD SP,0xFF00
        0xF5,                         // PUSH AF
        0x2A, 0x00, 0x80,             // LD HL,(0x8000)
        0x7E,                         // LD A,(HL)
        0xE6, 0xF0,                   // AND 0xF0
        0x0F, 0x0F, 0x0F, 0x0F,       // RRCA x4
        0x23,                         // INC HL
        0x22, 0x00, 0x80,             // LD (0x8000),HL
        0xF1,                         // POP AF
        0x76,                         // HALT
    });

    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    auto state = cpu.state_snapshot();
    state.registers.af = 0x1234;
    state.registers.cbar = 0x48;
    state.registers.cbr = 0xF0;
    state.registers.bbr = 0x04;
    bus.write_memory(0xF0000, 0x10);
    bus.write_memory(0xF0001, 0x80);
    bus.write_memory(0xF0010, 0xB0);
    cpu.load_state_snapshot(state);

    cpu.run_until_halt(32);
    const auto final_state = cpu.state_snapshot();

    REQUIRE(final_state.registers.af == 0x1234);
    REQUIRE(final_state.registers.hl == 0x8011);
    REQUIRE(bus.read_memory(0xF0000) == 0x11);
    REQUIRE(bus.read_memory(0xF0001) == 0x80);
}

TEST_CASE("extracted CPU covers YM status polling branch opcodes used by ROM audio bring-up", "[cpu]") {
    const auto rom = make_instruction_test_rom({
        0xDB, 0x00,                   // IN A,(0x00)
        0xED, 0x64, 0x80,             // TST 0x80
        0x20, 0x02,                   // JR NZ,+2
        0x3E, 0x11,                   // LD A,0x11 (skipped)
        0x76,                         // HALT
    });

    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};
    bus.mutable_controller_ports().load_state(
        vanguard8::core::io::ControllerPortsState{.port_state = {0xFF, 0xFF}}
    );

    cpu.run_until_halt(16);

    REQUIRE(cpu.accumulator() == 0xFF);
    REQUIRE(cpu.halted());
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

TEST_CASE("EI defers vectored interrupt acceptance until after the following instruction", "[cpu]") {
    const auto rom = make_instruction_test_rom({
        0xFB,  // EI
        0x00,  // NOP
        0x76,  // HALT
    });

    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    cpu.out0(0x3A, 0x48);
    cpu.out0(0x38, 0xF0);
    cpu.out0(0x33, 0xE0);
    cpu.out0(0x34, 0x02);
    cpu.set_register_i(0x80);

    bus.write_memory(0xF00E0, 0x34);
    bus.write_memory(0xF00E1, 0x12);
    bus.set_int1(true);

    cpu.step_instruction();
    REQUIRE(cpu.pc() == 0x0001);
    REQUIRE_FALSE(cpu.service_pending_interrupt().has_value());

    cpu.step_instruction();
    REQUIRE(cpu.pc() == 0x0002);

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

TEST_CASE("scheduled CPU covers LD r,r' family used by the PacManV8 ROM run path", "[cpu]") {
    SECTION("LD D,A (0x57) copies A into D at 4 T-states with flags preserved") {
        const auto rom = make_instruction_test_rom({0x57, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        auto state = cpu.state_snapshot();
        state.registers.af = static_cast<std::uint16_t>(0xA500U | flag_carry | flag_zero);
        state.registers.de = 0x1234;
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.next_scheduled_tstates() == 4);
        REQUIRE(cpu.step_scheduled_instruction() == 4);
        const auto result = cpu.state_snapshot();
        REQUIRE(static_cast<std::uint8_t>(result.registers.de >> 8U) == 0xA5);
        REQUIRE(static_cast<std::uint8_t>(result.registers.de & 0x00FFU) == 0x34);
        REQUIRE(static_cast<std::uint8_t>(result.registers.af & 0x00FFU) == (flag_carry | flag_zero));
    }

    SECTION("LD B,C / LD H,L / LD E,B / LD C,H / LD L,D / LD A,H each copy in 4 T-states") {
        struct Case {
            std::uint8_t opcode;
            std::function<void(vanguard8::third_party::z180::RegisterSnapshot&)> seed;
            std::function<bool(const vanguard8::third_party::z180::RegisterSnapshot&)> check;
        };
        const std::vector<Case> cases = {
            {0x41,
             [](auto& s) { s.bc = 0x00AA; },
             [](const auto& s) { return (s.bc >> 8U) == 0xAA && (s.bc & 0xFFU) == 0xAA; }},
            {0x65,
             [](auto& s) { s.hl = 0xBB00; s.hl = static_cast<std::uint16_t>(s.hl | 0x0077U); },
             [](const auto& s) { return (s.hl >> 8U) == 0x77; }},
            {0x58,
             [](auto& s) { s.bc = 0xCC00; s.de = 0x0000; },
             [](const auto& s) { return (s.de & 0xFFU) == 0xCC; }},
            {0x4C,
             [](auto& s) { s.hl = 0xDD11; s.bc = 0x0000; },
             [](const auto& s) { return (s.bc & 0xFFU) == 0xDD; }},
            {0x6A,
             [](auto& s) { s.de = 0xEE22; s.hl = 0x0000; },
             [](const auto& s) { return (s.hl & 0xFFU) == 0xEE; }},
            {0x7C,
             [](auto& s) { s.hl = 0xFF33; s.af = 0x0000; },
             [](const auto& s) { return (s.af >> 8U) == 0xFF; }},
        };
        for (const auto& c : cases) {
            const auto rom = make_instruction_test_rom({c.opcode, 0x76});
            vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
            vanguard8::core::cpu::Z180Adapter cpu{bus};
            auto state = cpu.state_snapshot();
            c.seed(state.registers);
            cpu.load_state_snapshot(state);

            REQUIRE(cpu.next_scheduled_tstates() == 4);
            REQUIRE(cpu.step_scheduled_instruction() == 4);
            REQUIRE(c.check(cpu.state_snapshot().registers));
        }
    }

    SECTION("LD E,(HL) (0x5E) reads from (HL) in 7 T-states") {
        const auto rom = make_instruction_test_rom({0x5E, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        auto state = cpu.state_snapshot();
        state.registers.cbar = 0x48;
        state.registers.cbr = 0xF0;
        state.registers.bbr = 0x04;
        state.registers.hl = 0x8200;
        cpu.load_state_snapshot(state);
        bus.write_memory(0xF0200, 0x9C);

        REQUIRE(cpu.next_scheduled_tstates() == 7);
        REQUIRE(cpu.step_scheduled_instruction() == 7);
        REQUIRE((cpu.state_snapshot().registers.de & 0x00FFU) == 0x9C);
    }

    SECTION("LD (HL),B (0x70) writes B to (HL) in 7 T-states") {
        const auto rom = make_instruction_test_rom({0x70, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        auto state = cpu.state_snapshot();
        state.registers.cbar = 0x48;
        state.registers.cbr = 0xF0;
        state.registers.bbr = 0x04;
        state.registers.hl = 0x8300;
        state.registers.bc = 0x4200;
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.next_scheduled_tstates() == 7);
        REQUIRE(cpu.step_scheduled_instruction() == 7);
        REQUIRE(bus.read_memory(0xF0300) == 0x42);
    }
}

TEST_CASE("scheduled CPU covers INC r / DEC r families used by the PacManV8 ROM run path", "[cpu]") {
    SECTION("DEC A (0x3D) on 0x01 sets Z, preserves carry, at 4 T-states") {
        const auto rom = make_instruction_test_rom({0x3D, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        auto state = cpu.state_snapshot();
        state.registers.af = static_cast<std::uint16_t>(0x0100U | flag_carry);
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.next_scheduled_tstates() == 4);
        REQUIRE(cpu.step_scheduled_instruction() == 4);
        const auto af = cpu.state_snapshot().registers.af;
        REQUIRE((af >> 8U) == 0x00);
        const auto flags = static_cast<std::uint8_t>(af & 0x00FFU);
        REQUIRE((flags & flag_zero) == flag_zero);
        REQUIRE((flags & flag_subtract) == flag_subtract);
        REQUIRE((flags & flag_carry) == flag_carry);
    }

    SECTION("DEC A (0x3D) on 0x80 flags signed overflow and sets half-borrow") {
        const auto rom = make_instruction_test_rom({0x3D, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        auto state = cpu.state_snapshot();
        state.registers.af = 0x8000;
        cpu.load_state_snapshot(state);

        (void)cpu.step_scheduled_instruction();
        const auto af = cpu.state_snapshot().registers.af;
        REQUIRE((af >> 8U) == 0x7F);
        const auto flags = static_cast<std::uint8_t>(af & 0x00FFU);
        REQUIRE((flags & flag_parity_overflow) == flag_parity_overflow);
        REQUIRE((flags & flag_half) == flag_half);
        REQUIRE((flags & flag_sign) == 0);
    }

    SECTION("INC A (0x3C) on 0x7F flags overflow and half-carry") {
        const auto rom = make_instruction_test_rom({0x3C, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        auto state = cpu.state_snapshot();
        state.registers.af = static_cast<std::uint16_t>(0x7F00U | flag_carry);
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.next_scheduled_tstates() == 4);
        (void)cpu.step_scheduled_instruction();
        const auto af = cpu.state_snapshot().registers.af;
        REQUIRE((af >> 8U) == 0x80);
        const auto flags = static_cast<std::uint8_t>(af & 0x00FFU);
        REQUIRE((flags & flag_sign) == flag_sign);
        REQUIRE((flags & flag_half) == flag_half);
        REQUIRE((flags & flag_parity_overflow) == flag_parity_overflow);
        REQUIRE((flags & flag_subtract) == 0);
        REQUIRE((flags & flag_carry) == flag_carry);
    }

    SECTION("INC (HL) / DEC (HL) use 11 T-states and rewrite the byte at (HL)") {
        const auto rom = make_instruction_test_rom({0x34, 0x35, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        auto state = cpu.state_snapshot();
        state.registers.cbar = 0x48;
        state.registers.cbr = 0xF0;
        state.registers.bbr = 0x04;
        state.registers.hl = 0x8400;
        cpu.load_state_snapshot(state);
        bus.write_memory(0xF0400, 0x09);

        REQUIRE(cpu.next_scheduled_tstates() == 11);
        REQUIRE(cpu.step_scheduled_instruction() == 11);
        REQUIRE(bus.read_memory(0xF0400) == 0x0A);
        REQUIRE(cpu.next_scheduled_tstates() == 11);
        REQUIRE(cpu.step_scheduled_instruction() == 11);
        REQUIRE(bus.read_memory(0xF0400) == 0x09);
    }
}

TEST_CASE("scheduled CPU covers conditional JR Z / JP Z,nn / JP NZ,nn / RET NZ", "[cpu]") {
    SECTION("JR Z,e (0x28) taken costs 12, not-taken costs 7") {
        const auto rom_taken = make_instruction_test_rom({0x28, 0x04, 0x76});
        vanguard8::core::Bus bus_t{vanguard8::core::memory::CartridgeSlot(rom_taken)};
        vanguard8::core::cpu::Z180Adapter cpu_t{bus_t};
        auto state = cpu_t.state_snapshot();
        state.registers.af = static_cast<std::uint16_t>(0x0000U | flag_zero);
        cpu_t.load_state_snapshot(state);
        REQUIRE(cpu_t.next_scheduled_tstates() == 12);
        REQUIRE(cpu_t.step_scheduled_instruction() == 12);
        REQUIRE(cpu_t.pc() == 0x0006);

        vanguard8::core::Bus bus_nt{vanguard8::core::memory::CartridgeSlot(rom_taken)};
        vanguard8::core::cpu::Z180Adapter cpu_nt{bus_nt};
        state = cpu_nt.state_snapshot();
        state.registers.af = 0x0000;
        cpu_nt.load_state_snapshot(state);
        REQUIRE(cpu_nt.next_scheduled_tstates() == 7);
        REQUIRE(cpu_nt.step_scheduled_instruction() == 7);
        REQUIRE(cpu_nt.pc() == 0x0002);
    }

    SECTION("JP Z,nn (0xCA) taken and JP NZ,nn (0xC2) not-taken each cost 10 T-states") {
        const auto rom_z = make_instruction_test_rom({0xCA, 0x34, 0x12, 0x76});
        vanguard8::core::Bus bus_z{vanguard8::core::memory::CartridgeSlot(rom_z)};
        vanguard8::core::cpu::Z180Adapter cpu_z{bus_z};
        auto state = cpu_z.state_snapshot();
        state.registers.af = static_cast<std::uint16_t>(0x0000U | flag_zero);
        cpu_z.load_state_snapshot(state);
        REQUIRE(cpu_z.next_scheduled_tstates() == 10);
        REQUIRE(cpu_z.step_scheduled_instruction() == 10);
        REQUIRE(cpu_z.pc() == 0x1234);

        const auto rom_nz = make_instruction_test_rom({0xC2, 0x78, 0x56, 0x76});
        vanguard8::core::Bus bus_nz{vanguard8::core::memory::CartridgeSlot(rom_nz)};
        vanguard8::core::cpu::Z180Adapter cpu_nz{bus_nz};
        state = cpu_nz.state_snapshot();
        state.registers.af = static_cast<std::uint16_t>(0x0000U | flag_zero);
        cpu_nz.load_state_snapshot(state);
        REQUIRE(cpu_nz.next_scheduled_tstates() == 10);
        REQUIRE(cpu_nz.step_scheduled_instruction() == 10);
        REQUIRE(cpu_nz.pc() == 0x0003);
    }

    SECTION("RET NZ (0xC0) taken pops SP and costs 11, not-taken costs 5") {
        const auto rom = make_instruction_test_rom({0xC0, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        auto state = cpu.state_snapshot();
        state.registers.cbar = 0x48;
        state.registers.cbr = 0xF0;
        state.registers.bbr = 0x04;
        state.registers.sp = 0x8100;
        state.registers.af = 0x0000;
        cpu.load_state_snapshot(state);
        bus.write_memory(0xF0100, 0xCD);
        bus.write_memory(0xF0101, 0xAB);

        REQUIRE(cpu.next_scheduled_tstates() == 11);
        REQUIRE(cpu.step_scheduled_instruction() == 11);
        REQUIRE(cpu.pc() == 0xABCD);
        REQUIRE(cpu.state_snapshot().registers.sp == 0x8102);

        vanguard8::core::Bus bus2{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu2{bus2};
        state = cpu2.state_snapshot();
        state.registers.af = static_cast<std::uint16_t>(0x0000U | flag_zero);
        cpu2.load_state_snapshot(state);
        REQUIRE(cpu2.next_scheduled_tstates() == 5);
        REQUIRE(cpu2.step_scheduled_instruction() == 5);
        REQUIRE(cpu2.pc() == 0x0001);
    }
}

TEST_CASE("scheduled CPU covers DJNZ and carry-conditional JR used by the PacManV8 T020 path", "[cpu]") {
    SECTION("DJNZ (0x10) decrements B, takes the branch in 13 T-states, and leaves flags untouched") {
        const auto rom = make_instruction_test_rom({0x10, 0x02, 0x76, 0x00, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        auto state = cpu.state_snapshot();
        constexpr auto preserved_flags = static_cast<std::uint8_t>(
            flag_sign | flag_zero | flag_half | flag_parity_overflow | flag_subtract | flag_carry
        );
        state.registers.bc = 0x0200;
        state.registers.af = static_cast<std::uint16_t>(0x5500U | preserved_flags);
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.next_scheduled_tstates() == 13);
        REQUIRE(cpu.step_scheduled_instruction() == 13);
        REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.bc >> 8U) == 0x01);
        REQUIRE(cpu.pc() == 0x0004);
        REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) == preserved_flags);
    }

    SECTION("DJNZ (0x10) falls through in 8 T-states when B reaches zero") {
        const auto rom = make_instruction_test_rom({0x10, 0x02, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        auto state = cpu.state_snapshot();
        state.registers.bc = 0x0100;
        state.registers.af = 0xAA00;
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.next_scheduled_tstates() == 8);
        REQUIRE(cpu.step_scheduled_instruction() == 8);
        REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.bc >> 8U) == 0x00);
        REQUIRE(cpu.pc() == 0x0002);
        REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) == 0x00);
    }

    SECTION("JR NC,e (0x30) takes on carry clear in 12 T-states and falls through in 7 with carry set") {
        const auto rom = make_instruction_test_rom({0x30, 0x02, 0x76, 0x00, 0x76});
        vanguard8::core::Bus taken_bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter taken_cpu{taken_bus};
        auto state = taken_cpu.state_snapshot();
        constexpr auto preserved_flags = static_cast<std::uint8_t>(flag_sign | flag_zero | flag_parity_overflow);
        state.registers.af = static_cast<std::uint16_t>(0x0000U | preserved_flags);
        taken_cpu.load_state_snapshot(state);

        REQUIRE(taken_cpu.next_scheduled_tstates() == 12);
        REQUIRE(taken_cpu.step_scheduled_instruction() == 12);
        REQUIRE(taken_cpu.pc() == 0x0004);
        REQUIRE(static_cast<std::uint8_t>(taken_cpu.state_snapshot().registers.af & 0x00FFU) == preserved_flags);

        vanguard8::core::Bus fallthrough_bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter fallthrough_cpu{fallthrough_bus};
        state = fallthrough_cpu.state_snapshot();
        state.registers.af = static_cast<std::uint16_t>(0x0000U | (preserved_flags | flag_carry));
        fallthrough_cpu.load_state_snapshot(state);

        REQUIRE(fallthrough_cpu.next_scheduled_tstates() == 7);
        REQUIRE(fallthrough_cpu.step_scheduled_instruction() == 7);
        REQUIRE(fallthrough_cpu.pc() == 0x0002);
        REQUIRE(static_cast<std::uint8_t>(fallthrough_cpu.state_snapshot().registers.af & 0x00FFU) ==
                static_cast<std::uint8_t>(preserved_flags | flag_carry));
    }

    SECTION("JR C,e (0x38) takes on carry set in 12 T-states and falls through in 7 with carry clear") {
        const auto rom = make_instruction_test_rom({0x38, 0x02, 0x76, 0x00, 0x76});
        vanguard8::core::Bus taken_bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter taken_cpu{taken_bus};
        auto state = taken_cpu.state_snapshot();
        constexpr auto preserved_flags = static_cast<std::uint8_t>(flag_half | flag_carry);
        state.registers.af = static_cast<std::uint16_t>(0x0000U | preserved_flags);
        taken_cpu.load_state_snapshot(state);

        REQUIRE(taken_cpu.next_scheduled_tstates() == 12);
        REQUIRE(taken_cpu.step_scheduled_instruction() == 12);
        REQUIRE(taken_cpu.pc() == 0x0004);
        REQUIRE(static_cast<std::uint8_t>(taken_cpu.state_snapshot().registers.af & 0x00FFU) == preserved_flags);

        vanguard8::core::Bus fallthrough_bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter fallthrough_cpu{fallthrough_bus};
        state = fallthrough_cpu.state_snapshot();
        state.registers.af = static_cast<std::uint16_t>(0x0000U | flag_half);
        fallthrough_cpu.load_state_snapshot(state);

        REQUIRE(fallthrough_cpu.next_scheduled_tstates() == 7);
        REQUIRE(fallthrough_cpu.step_scheduled_instruction() == 7);
        REQUIRE(fallthrough_cpu.pc() == 0x0002);
        REQUIRE(static_cast<std::uint8_t>(fallthrough_cpu.state_snapshot().registers.af & 0x00FFU) == flag_half);
    }
}

TEST_CASE("scheduled CPU covers ADD A,n blocker exposed after YM busy-poll timing fix", "[cpu]") {
    SECTION("ADD A,n (0xC6) adds immediate data, clears N, and costs 7 T-states") {
        const auto rom = make_instruction_test_rom({0xC6, 0x22, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        auto state = cpu.state_snapshot();
        state.registers.af = static_cast<std::uint16_t>(0x1100U | flag_subtract | flag_carry);
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.next_scheduled_tstates() == 7);
        REQUIRE(cpu.step_scheduled_instruction() == 7);
        const auto af = cpu.state_snapshot().registers.af;
        REQUIRE((af >> 8U) == 0x33);
        const auto flags = static_cast<std::uint8_t>(af & 0x00FFU);
        REQUIRE((flags & flag_subtract) == 0);
        REQUIRE((flags & flag_carry) == 0);
        REQUIRE((flags & flag_zero) == 0);
        REQUIRE(cpu.pc() == 0x0002);
    }

    SECTION("ADD A,n (0xC6) reports half-carry, signed overflow, and carry") {
        const auto overflow_rom = make_instruction_test_rom({0xC6, 0x01, 0x76});
        vanguard8::core::Bus overflow_bus{vanguard8::core::memory::CartridgeSlot(overflow_rom)};
        vanguard8::core::cpu::Z180Adapter overflow_cpu{overflow_bus};
        auto state = overflow_cpu.state_snapshot();
        state.registers.af = 0x7F00;
        overflow_cpu.load_state_snapshot(state);

        (void)overflow_cpu.step_scheduled_instruction();
        auto af = overflow_cpu.state_snapshot().registers.af;
        REQUIRE((af >> 8U) == 0x80);
        auto flags = static_cast<std::uint8_t>(af & 0x00FFU);
        REQUIRE((flags & flag_sign) == flag_sign);
        REQUIRE((flags & flag_half) == flag_half);
        REQUIRE((flags & flag_parity_overflow) == flag_parity_overflow);
        REQUIRE((flags & flag_carry) == 0);

        const auto carry_rom = make_instruction_test_rom({0xC6, 0x01, 0x76});
        vanguard8::core::Bus carry_bus{vanguard8::core::memory::CartridgeSlot(carry_rom)};
        vanguard8::core::cpu::Z180Adapter carry_cpu{carry_bus};
        state = carry_cpu.state_snapshot();
        state.registers.af = 0xFF00;
        carry_cpu.load_state_snapshot(state);

        (void)carry_cpu.step_scheduled_instruction();
        af = carry_cpu.state_snapshot().registers.af;
        REQUIRE((af >> 8U) == 0x00);
        flags = static_cast<std::uint8_t>(af & 0x00FFU);
        REQUIRE((flags & flag_zero) == flag_zero);
        REQUIRE((flags & flag_half) == flag_half);
        REQUIRE((flags & flag_carry) == flag_carry);
        REQUIRE((flags & flag_subtract) == 0);
    }
}

TEST_CASE("scheduled CPU covers ADD A,r / ADD A,(HL) and 16-bit ADD/SBC families used by the PacManV8 T020 path", "[cpu]") {
    SECTION("ADD A,A (0x87) keeps the timed dispatcher on the intermission path at 4 T-states") {
        const auto rom = make_instruction_test_rom({0x87, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        auto state = cpu.state_snapshot();
        state.registers.af = static_cast<std::uint16_t>(0x8100U | flag_subtract);
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.next_scheduled_tstates() == 4);
        REQUIRE(cpu.step_scheduled_instruction() == 4);
        const auto af = cpu.state_snapshot().registers.af;
        REQUIRE((af >> 8U) == 0x02);
        const auto flags = static_cast<std::uint8_t>(af & 0x00FFU);
        REQUIRE((flags & flag_parity_overflow) == flag_parity_overflow);
        REQUIRE((flags & flag_carry) == flag_carry);
        REQUIRE((flags & flag_subtract) == 0);
    }

    SECTION("ADD A,(HL) (0x86) reads through the MMU in 7 T-states and updates H") {
        const auto rom = make_instruction_test_rom({0x86, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        auto state = cpu.state_snapshot();
        state.registers.cbar = 0x48;
        state.registers.cbr = 0xF0;
        state.registers.bbr = 0x04;
        state.registers.hl = 0x8200;
        state.registers.af = 0x0F00;
        cpu.load_state_snapshot(state);
        bus.write_memory(0xF0200, 0x01);

        REQUIRE(cpu.next_scheduled_tstates() == 7);
        REQUIRE(cpu.step_scheduled_instruction() == 7);
        const auto af = cpu.state_snapshot().registers.af;
        REQUIRE((af >> 8U) == 0x10);
        const auto flags = static_cast<std::uint8_t>(af & 0x00FFU);
        REQUIRE((flags & flag_half) == flag_half);
        REQUIRE((flags & flag_carry) == 0);
        REQUIRE((flags & flag_subtract) == 0);
    }

    SECTION("ADD HL,BC (0x09) preserves S/Z/PV, clears N, and updates H/C in 11 T-states") {
        const auto rom = make_instruction_test_rom({0x09, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        auto state = cpu.state_snapshot();
        state.registers.hl = 0x0FFF;
        state.registers.bc = 0x0001;
        state.registers.af = static_cast<std::uint16_t>(0x0000U |
                                                        flag_sign |
                                                        flag_zero |
                                                        flag_parity_overflow |
                                                        flag_subtract |
                                                        flag_carry);
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.next_scheduled_tstates() == 11);
        REQUIRE(cpu.step_scheduled_instruction() == 11);
        REQUIRE(cpu.state_snapshot().registers.hl == 0x1000);
        REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) ==
                static_cast<std::uint8_t>(flag_sign | flag_zero | flag_parity_overflow | flag_half));
    }

    SECTION("ADD HL,DE (0x19) reports carry in 11 T-states") {
        const auto rom = make_instruction_test_rom({0x19, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        auto state = cpu.state_snapshot();
        state.registers.hl = 0xFFFF;
        state.registers.de = 0x0001;
        state.registers.af = static_cast<std::uint16_t>(0x0000U | flag_zero);
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.next_scheduled_tstates() == 11);
        REQUIRE(cpu.step_scheduled_instruction() == 11);
        REQUIRE(cpu.state_snapshot().registers.hl == 0x0000);
        REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) ==
                static_cast<std::uint8_t>(flag_zero | flag_half | flag_carry));
    }

    SECTION("SBC HL,DE (ED 52) can produce a zero result with Z/N set in 15 T-states") {
        const auto rom = make_instruction_test_rom({0xED, 0x52, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        auto state = cpu.state_snapshot();
        state.registers.hl = 0x1234;
        state.registers.de = 0x1234;
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.next_scheduled_tstates() == 15);
        REQUIRE(cpu.step_scheduled_instruction() == 15);
        REQUIRE(cpu.state_snapshot().registers.hl == 0x0000);
        REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) ==
                static_cast<std::uint8_t>(flag_zero | flag_subtract));
    }

    SECTION("SBC HL,DE (ED 52) reports half-borrow and signed overflow without full borrow") {
        const auto rom = make_instruction_test_rom({0xED, 0x52, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        auto state = cpu.state_snapshot();
        state.registers.hl = 0x8000;
        state.registers.de = 0x0001;
        cpu.load_state_snapshot(state);

        (void)cpu.step_scheduled_instruction();
        REQUIRE(cpu.state_snapshot().registers.hl == 0x7FFF);
        REQUIRE(static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU) ==
                static_cast<std::uint8_t>(flag_half | flag_parity_overflow | flag_subtract));
    }
}

TEST_CASE("scheduled CPU covers CP n / CP r / CP (HL) flag semantics", "[cpu]") {
    SECTION("CP n (0xFE) with A==n sets Z and keeps A unchanged") {
        const auto rom = make_instruction_test_rom({0xFE, 0x42, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        auto state = cpu.state_snapshot();
        state.registers.af = 0x4200;
        cpu.load_state_snapshot(state);

        REQUIRE(cpu.next_scheduled_tstates() == 7);
        REQUIRE(cpu.step_scheduled_instruction() == 7);
        const auto af = cpu.state_snapshot().registers.af;
        REQUIRE((af >> 8U) == 0x42);
        const auto flags = static_cast<std::uint8_t>(af & 0x00FFU);
        REQUIRE((flags & flag_zero) == flag_zero);
        REQUIRE((flags & flag_subtract) == flag_subtract);
        REQUIRE((flags & flag_carry) == 0);
    }

    SECTION("CP n (0xFE) with A<n sets C and N") {
        const auto rom = make_instruction_test_rom({0xFE, 0x50, 0x76});
        vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
        vanguard8::core::cpu::Z180Adapter cpu{bus};
        auto state = cpu.state_snapshot();
        state.registers.af = 0x2000;
        cpu.load_state_snapshot(state);
        (void)cpu.step_scheduled_instruction();
        const auto flags = static_cast<std::uint8_t>(cpu.state_snapshot().registers.af & 0x00FFU);
        REQUIRE((flags & flag_carry) == flag_carry);
        REQUIRE((flags & flag_subtract) == flag_subtract);
        REQUIRE((flags & flag_zero) == 0);
    }

    SECTION("CP B (0xB8) costs 4 T-states; CP (HL) (0xBE) costs 7 T-states") {
        const auto rom_r = make_instruction_test_rom({0xB8, 0x76});
        vanguard8::core::Bus bus_r{vanguard8::core::memory::CartridgeSlot(rom_r)};
        vanguard8::core::cpu::Z180Adapter cpu_r{bus_r};
        auto state = cpu_r.state_snapshot();
        state.registers.af = 0x5500;
        state.registers.bc = 0x5500;
        cpu_r.load_state_snapshot(state);
        REQUIRE(cpu_r.next_scheduled_tstates() == 4);
        REQUIRE(cpu_r.step_scheduled_instruction() == 4);
        REQUIRE((cpu_r.state_snapshot().registers.af & flag_zero) == flag_zero);

        const auto rom_m = make_instruction_test_rom({0xBE, 0x76});
        vanguard8::core::Bus bus_m{vanguard8::core::memory::CartridgeSlot(rom_m)};
        vanguard8::core::cpu::Z180Adapter cpu_m{bus_m};
        state = cpu_m.state_snapshot();
        state.registers.cbar = 0x48;
        state.registers.cbr = 0xF0;
        state.registers.bbr = 0x04;
        state.registers.af = 0x7700;
        state.registers.hl = 0x8500;
        cpu_m.load_state_snapshot(state);
        bus_m.write_memory(0xF0500, 0x66);
        REQUIRE(cpu_m.next_scheduled_tstates() == 7);
        REQUIRE(cpu_m.step_scheduled_instruction() == 7);
        const auto flags = static_cast<std::uint8_t>(cpu_m.state_snapshot().registers.af & 0x00FFU);
        REQUIRE((flags & flag_carry) == 0);
        REQUIRE((flags & flag_subtract) == flag_subtract);
        REQUIRE((cpu_m.state_snapshot().registers.af >> 8U) == 0x77);
    }
}

TEST_CASE("HALT with interrupts disabled stays halted across repeated steps", "[cpu]") {
    const auto rom = make_halt_wake_test_rom();
    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    auto state = cpu.state_snapshot();
    state.registers.cbar = 0x48;
    state.registers.cbr = 0xF0;
    state.registers.bbr = 0x04;
    state.registers.sp = 0x8100;
    cpu.load_state_snapshot(state);
    cpu.set_interrupt_mode(1);
    cpu.set_iff1(false);

    cpu.step_instruction();
    REQUIRE(cpu.halted());
    REQUIRE(cpu.pc() == 0x0000);

    for (int i = 0; i < 8; ++i) {
        REQUIRE_FALSE(cpu.service_pending_interrupt().has_value());
        REQUIRE(cpu.halted());
        REQUIRE(cpu.pc() == 0x0000);
    }
}

TEST_CASE("HALT resumes at the instruction after HALT on INT0 IM1 wake", "[cpu]") {
    const auto rom = make_halt_wake_test_rom();
    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    auto state = cpu.state_snapshot();
    state.registers.cbar = 0x48;
    state.registers.cbr = 0xF0;
    state.registers.bbr = 0x04;
    state.registers.sp = 0x8100;
    cpu.load_state_snapshot(state);
    cpu.set_interrupt_mode(1);
    cpu.set_iff1(true);

    cpu.step_instruction();
    REQUIRE(cpu.halted());
    REQUIRE(cpu.pc() == 0x0000);

    write_vdp_register(bus, 0x81, 1, 0x20);
    bus.mutable_vdp_a().set_vblank_flag(true);
    bus.sync_vdp_interrupt_lines();
    REQUIRE(bus.int0_asserted());

    const auto service = cpu.service_pending_interrupt();
    REQUIRE(service.has_value());
    REQUIRE(service->source == vanguard8::core::cpu::InterruptSource::int0);
    REQUIRE_FALSE(cpu.halted());
    REQUIRE(cpu.pc() == 0x0038);

    const auto sp_after_push = cpu.state_snapshot().registers.sp;
    REQUIRE(sp_after_push == static_cast<std::uint16_t>(0x8100U - 2U));
    const auto return_lo = bus.read_memory(cpu.translate_logical_address(sp_after_push));
    const auto return_hi = bus.read_memory(cpu.translate_logical_address(
        static_cast<std::uint16_t>(sp_after_push + 1U)));
    const auto return_address = static_cast<std::uint16_t>(return_lo | (return_hi << 8));
    REQUIRE(return_address == 0x0001);

    cpu.step_instruction();  // executes RETI at 0x0038
    REQUIRE(cpu.pc() == 0x0001);
    REQUIRE_FALSE(cpu.halted());
    REQUIRE(cpu.state_snapshot().registers.sp == 0x8100);
}

TEST_CASE("HALT resumes at the instruction after HALT on INT1 IM2 wake", "[cpu]") {
    const auto rom = make_halt_wake_test_rom();
    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    auto state = cpu.state_snapshot();
    state.registers.cbar = 0x48;
    state.registers.cbr = 0xF0;
    state.registers.bbr = 0x04;
    state.registers.sp = 0x8100;
    cpu.load_state_snapshot(state);
    cpu.out0(0x33, 0xE0);
    cpu.out0(0x34, 0x02);
    cpu.set_register_i(0x80);
    cpu.set_interrupt_mode(2);
    cpu.set_iff1(true);

    // Vectored INT1 handler address 0x9000 (points back into ROM's 0x4000
    // bank, which is open-bus here — we only care about the resume target
    // bytes on the stack, not executing the handler body).
    bus.write_memory(0xF00E0, 0x00);
    bus.write_memory(0xF00E1, 0x90);

    cpu.step_instruction();
    REQUIRE(cpu.halted());
    REQUIRE(cpu.pc() == 0x0000);

    bus.set_int1(true);
    REQUIRE(bus.int1_asserted());

    const auto service = cpu.service_pending_interrupt();
    REQUIRE(service.has_value());
    REQUIRE(service->source == vanguard8::core::cpu::InterruptSource::int1);
    REQUIRE(service->handler_address == 0x9000);
    REQUIRE_FALSE(cpu.halted());
    REQUIRE(cpu.pc() == 0x9000);

    const auto sp_after_push = cpu.state_snapshot().registers.sp;
    const auto return_lo = bus.read_memory(cpu.translate_logical_address(sp_after_push));
    const auto return_hi = bus.read_memory(cpu.translate_logical_address(
        static_cast<std::uint16_t>(sp_after_push + 1U)));
    const auto return_address = static_cast<std::uint16_t>(return_lo | (return_hi << 8));
    REQUIRE(return_address == 0x0001);
}

TEST_CASE("HALT resumes at the instruction after HALT on PRT0 wake", "[cpu]") {
    const auto rom = make_halt_wake_test_rom();
    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    auto state = cpu.state_snapshot();
    state.registers.cbar = 0x48;
    state.registers.cbr = 0xF0;
    state.registers.bbr = 0x04;
    state.registers.sp = 0x8100;
    cpu.load_state_snapshot(state);
    cpu.out0(0x33, 0xE0);
    cpu.set_register_i(0x80);
    cpu.set_interrupt_mode(1);
    cpu.set_iff1(true);

    bus.write_memory(0xF00E4, 0x00);
    bus.write_memory(0xF00E5, 0xA0);

    cpu.step_instruction();
    REQUIRE(cpu.halted());
    REQUIRE(cpu.pc() == 0x0000);

    write_internal_16(cpu, 0x0C, 1);
    write_internal_16(cpu, 0x0E, 1);
    cpu.out0(0x10, 0x11);
    cpu.advance_tstates(20);

    const auto service = cpu.service_pending_interrupt();
    REQUIRE(service.has_value());
    REQUIRE(service->source == vanguard8::core::cpu::InterruptSource::prt0);
    REQUIRE(service->handler_address == 0xA000);
    REQUIRE_FALSE(cpu.halted());
    REQUIRE(cpu.pc() == 0xA000);

    const auto sp_after_push = cpu.state_snapshot().registers.sp;
    const auto return_lo = bus.read_memory(cpu.translate_logical_address(sp_after_push));
    const auto return_hi = bus.read_memory(cpu.translate_logical_address(
        static_cast<std::uint16_t>(sp_after_push + 1U)));
    REQUIRE(static_cast<std::uint16_t>(return_lo | (return_hi << 8)) == 0x0001);
}

TEST_CASE("Repeated HALT wake cycles each resume at the instruction after HALT", "[cpu]") {
    // Variant of the HALT-wake test that cycles HALT -> service -> RETI twice
    // to prove the fix is stable across back-to-back vblank-style wakes, the
    // observed shape of the PacManV8 idle_loop.
    std::vector<std::uint8_t> rom(
        vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);
    rom[0x0000] = 0x76;  // HALT
    rom[0x0001] = 0x18; rom[0x0002] = 0xFD;  // JR -3 (back to HALT)
    rom[0x0038] = 0xFB;  // EI
    rom[0x0039] = 0xED; rom[0x003A] = 0x4D;  // RETI

    vanguard8::core::Bus bus{vanguard8::core::memory::CartridgeSlot(rom)};
    vanguard8::core::cpu::Z180Adapter cpu{bus};

    auto state = cpu.state_snapshot();
    state.registers.cbar = 0x48;
    state.registers.cbr = 0xF0;
    state.registers.bbr = 0x04;
    state.registers.sp = 0x8100;
    cpu.load_state_snapshot(state);
    cpu.set_interrupt_mode(1);
    cpu.set_iff1(true);

    write_vdp_register(bus, 0x81, 1, 0x20);

    for (int cycle = 0; cycle < 2; ++cycle) {
        cpu.step_instruction();  // HALT
        REQUIRE(cpu.halted());
        REQUIRE(cpu.pc() == 0x0000);

        bus.mutable_vdp_a().set_vblank_flag(true);
        bus.sync_vdp_interrupt_lines();
        REQUIRE(bus.int0_asserted());

        const auto service = cpu.service_pending_interrupt();
        REQUIRE(service.has_value());
        REQUIRE(service->source == vanguard8::core::cpu::InterruptSource::int0);
        REQUIRE(cpu.pc() == 0x0038);

        bus.mutable_vdp_a().set_vblank_flag(false);
        bus.sync_vdp_interrupt_lines();

        cpu.step_instruction();  // EI
        cpu.step_instruction();  // RETI
        REQUIRE(cpu.pc() == 0x0001);
        REQUIRE(cpu.state_snapshot().registers.sp == 0x8100);
        REQUIRE_FALSE(cpu.halted());

        cpu.step_instruction();  // JR -3 (back to HALT at 0x0000)
        REQUIRE(cpu.pc() == 0x0000);
    }
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

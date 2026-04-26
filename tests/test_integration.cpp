#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "core/emulator.hpp"
#include "core/hash.hpp"
#include "core/memory/cartridge.hpp"
#include "core/video/compositor.hpp"
#include "core/video/v9938.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <tuple>
#include <vector>

namespace {

using vanguard8::core::Emulator;
using vanguard8::core::EventType;
using vanguard8::core::VclkRate;
using vanguard8::core::video::Compositor;
using vanguard8::core::video::V9938;

auto digest_hex(const vanguard8::core::Sha256Digest& digest) -> std::string {
    static constexpr char hex[] = "0123456789abcdef";
    std::string output;
    output.reserve(digest.size() * 2U);
    for (const auto byte : digest) {
        output.push_back(hex[(byte >> 4U) & 0x0FU]);
        output.push_back(hex[byte & 0x0FU]);
    }
    return output;
}

auto make_integration_rom() -> std::vector<std::uint8_t> {
    constexpr std::size_t rom_size = 0xC000;
    std::vector<std::uint8_t> rom(rom_size, 0x00);
    rom[0x0000] = 0x76;  // HALT in the fixed boot window.
    std::fill(rom.begin() + 0x4000, rom.begin() + 0x8000, 0xA1);
    std::fill(rom.begin() + 0x8000, rom.begin() + 0xC000, 0xB2);
    return rom;
}

auto make_frame_loop_rom() -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);

    rom[0x0000] = 0xF3;             // DI
    rom[0x0001] = 0x3E; rom[0x0002] = 0x01;  // LD A,0x01
    rom[0x0003] = 0xD3; rom[0x0004] = 0x82;  // OUT (0x82),A
    rom[0x0005] = 0x3E; rom[0x0006] = 0x07;  // LD A,0x07
    rom[0x0007] = 0xD3; rom[0x0008] = 0x82;  // OUT (0x82),A
    rom[0x0009] = 0x3E; rom[0x000A] = 0x00;  // LD A,0x00
    rom[0x000B] = 0xD3; rom[0x000C] = 0x82;  // OUT (0x82),A
    rom[0x000D] = 0x3E; rom[0x000E] = 0x01;  // LD A,0x01
    rom[0x000F] = 0xD3; rom[0x0010] = 0x81;  // OUT (0x81),A
    rom[0x0011] = 0x3E; rom[0x0012] = 0x87;  // LD A,0x87
    rom[0x0013] = 0xD3; rom[0x0014] = 0x81;  // OUT (0x81),A
    rom[0x0015] = 0xC3; rom[0x0016] = 0x15; rom[0x0017] = 0x00;  // JP 0x0015

    return rom;
}

auto make_high_address_graphic6_runtime_rom() -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);
    std::size_t pc = 0;

    auto emit = [&](const std::initializer_list<std::uint8_t> bytes) {
        for (const auto byte : bytes) {
            rom[pc++] = byte;
        }
    };
    auto emit_out_immediate = [&](const std::uint8_t port, const std::uint8_t value) {
        emit({0x3E, value, 0xD3, port});
    };
    auto emit_vdp_register_write = [&](const std::uint8_t control_port,
                                       const std::uint8_t reg,
                                       const std::uint8_t value) {
        emit_out_immediate(control_port, value);
        emit_out_immediate(control_port, static_cast<std::uint8_t>(0x80U | reg));
    };
    auto emit_vram_write = [&](const std::uint8_t control_port,
                               const std::uint8_t data_port,
                               const std::uint16_t address,
                               const std::uint8_t value) {
        emit_out_immediate(control_port, static_cast<std::uint8_t>(address & 0x00FFU));
        emit_out_immediate(
            control_port,
            static_cast<std::uint8_t>(0x40U | ((address >> 8U) & 0x3FU))
        );
        emit_out_immediate(data_port, value);
    };

    emit({0xF3});  // DI

    emit_vdp_register_write(0x81, 0, V9938::graphic6_mode_r0);
    emit_vdp_register_write(0x81, 1, static_cast<std::uint8_t>(V9938::graphic_mode_r1 | 0x40U));
    emit_vdp_register_write(0x81, 8, 0x20);
    emit_vdp_register_write(0x81, 14, 0x01);

    emit_vdp_register_write(0x85, 0, V9938::graphic4_mode_r0);
    emit_vdp_register_write(0x85, 1, static_cast<std::uint8_t>(V9938::graphic_mode_r1 | 0x40U));

    emit_out_immediate(0x82, 0x04);
    emit_out_immediate(0x82, 0x70);
    emit_out_immediate(0x82, 0x00);

    emit_out_immediate(0x86, 0x02);
    emit_out_immediate(0x86, 0x07);
    emit_out_immediate(0x86, 0x00);

    emit_vram_write(0x85, 0x84, 0x3200, 0x22);
    emit_vram_write(0x81, 0x80, 0x2400, 0x40);

    emit({0xC3, static_cast<std::uint8_t>(pc & 0x00FFU), static_cast<std::uint8_t>(pc >> 8U)});
    return rom;
}

auto make_int1_runtime_rom(const std::uint8_t first_handler_opcode) -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);

    rom[0x0000] = 0xF3;                                     // DI
    rom[0x0001] = 0x3E; rom[0x0002] = 0x48;                // LD A,0x48
    rom[0x0003] = 0xED; rom[0x0004] = 0x39; rom[0x0005] = 0x3A;  // OUT0 (0x3A),A
    rom[0x0006] = 0x3E; rom[0x0007] = 0xF0;                // LD A,0xF0
    rom[0x0008] = 0xED; rom[0x0009] = 0x39; rom[0x000A] = 0x38;  // OUT0 (0x38),A
    rom[0x000B] = 0x3E; rom[0x000C] = 0x04;                // LD A,0x04
    rom[0x000D] = 0xED; rom[0x000E] = 0x39; rom[0x000F] = 0x39;  // OUT0 (0x39),A
    rom[0x0010] = 0x3E; rom[0x0011] = 0x80;                // LD A,0x80
    rom[0x0012] = 0xED; rom[0x0013] = 0x47;                // LD I,A
    rom[0x0014] = 0x3E; rom[0x0015] = 0xE0;                // LD A,0xE0
    rom[0x0016] = 0xED; rom[0x0017] = 0x39; rom[0x0018] = 0x33;  // OUT0 (0x33),A
    rom[0x0019] = 0x3E; rom[0x001A] = 0x02;                // LD A,0x02
    rom[0x001B] = 0xED; rom[0x001C] = 0x39; rom[0x001D] = 0x34;  // OUT0 (0x34),A
    rom[0x001E] = 0x21; rom[0x001F] = 0x5A; rom[0x0020] = 0x00;  // LD HL,0x005A
    rom[0x0021] = 0x22; rom[0x0022] = 0xE0; rom[0x0023] = 0x80;  // LD (0x80E0),HL
    rom[0x0024] = 0xFB;                                     // EI
    rom[0x0025] = 0xC3; rom[0x0026] = 0x25; rom[0x0027] = 0x00;  // JP 0x0025

    rom[0x005A] = first_handler_opcode;
    rom[0x005B] = 0x3E; rom[0x005C] = 0x55;                // LD A,0x55
    rom[0x005D] = 0x32; rom[0x005E] = 0x02; rom[0x005F] = 0x81;  // LD (0x8102),A
    rom[0x0060] = 0xFB;                                     // EI
    rom[0x0061] = 0xED; rom[0x0062] = 0x4D;                // RETI

    return rom;
}

auto make_audio_window_runtime_rom() -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);

    rom[0x0000] = 0xF3;                                     // DI
    rom[0x0001] = 0x31; rom[0x0002] = 0x00; rom[0x0003] = 0xFF;  // LD SP,0xFF00
    rom[0x0004] = 0x3E; rom[0x0005] = 0x48;                // LD A,0x48
    rom[0x0006] = 0xED; rom[0x0007] = 0x39; rom[0x0008] = 0x3A;  // OUT0 (0x3A),A
    rom[0x0009] = 0x3E; rom[0x000A] = 0xF0;                // LD A,0xF0
    rom[0x000B] = 0xED; rom[0x000C] = 0x39; rom[0x000D] = 0x38;  // OUT0 (0x38),A
    rom[0x000E] = 0x3E; rom[0x000F] = 0x04;                // LD A,0x04
    rom[0x0010] = 0xED; rom[0x0011] = 0x39; rom[0x0012] = 0x39;  // OUT0 (0x39),A
    rom[0x0013] = 0x3E; rom[0x0014] = 0x80;                // LD A,0x80
    rom[0x0015] = 0xED; rom[0x0016] = 0x47;                // LD I,A
    rom[0x0017] = 0x3E; rom[0x0018] = 0xE0;                // LD A,0xE0
    rom[0x0019] = 0xED; rom[0x001A] = 0x39; rom[0x001B] = 0x33;  // OUT0 (0x33),A
    rom[0x001C] = 0x3E; rom[0x001D] = 0x02;                // LD A,0x02
    rom[0x001E] = 0xED; rom[0x001F] = 0x39; rom[0x0020] = 0x34;  // OUT0 (0x34),A
    rom[0x0021] = 0x21; rom[0x0022] = 0x60; rom[0x0023] = 0x00;  // LD HL,0x0060
    rom[0x0024] = 0x22; rom[0x0025] = 0xE0; rom[0x0026] = 0x80;  // LD (0x80E0),HL
    rom[0x0027] = 0x21; rom[0x0028] = 0x20; rom[0x0029] = 0x01;  // LD HL,0x0120
    rom[0x002A] = 0x22; rom[0x002B] = 0x08; rom[0x002C] = 0x81;  // LD (0x8108),HL
    rom[0x002D] = 0x3E; rom[0x002E] = 0x20;                // LD A,0x20
    rom[0x002F] = 0xD3; rom[0x0030] = 0x40;                // OUT (0x40),A
    rom[0x0031] = 0xDB; rom[0x0032] = 0x40;                // IN A,(0x40)
    rom[0x0033] = 0xED; rom[0x0034] = 0x64; rom[0x0035] = 0x80;  // TST 0x80
    rom[0x0036] = 0x20; rom[0x0037] = 0xF9;                // JR NZ,0x0031
    rom[0x0038] = 0x3E; rom[0x0039] = 0xC0;                // LD A,0xC0
    rom[0x003A] = 0xD3; rom[0x003B] = 0x41;                // OUT (0x41),A
    rom[0x003C] = 0x3E; rom[0x003D] = 0xCC;                // LD A,0xCC
    rom[0x003E] = 0x32; rom[0x003F] = 0x04; rom[0x0040] = 0x81;  // LD (0x8104),A
    rom[0x0041] = 0x3E; rom[0x0042] = 0x02;                // LD A,0x02
    rom[0x0043] = 0xD3; rom[0x0044] = 0x60;                // OUT (0x60),A
    rom[0x0045] = 0xFB;                                     // EI
    rom[0x0046] = 0xC3; rom[0x0047] = 0x46; rom[0x0048] = 0x00;  // JP 0x0046

    rom[0x0060] = 0xF5;                                     // PUSH AF
    rom[0x0061] = 0x2A; rom[0x0062] = 0x08; rom[0x0063] = 0x81;  // LD HL,(0x8108)
    rom[0x0064] = 0x7E;                                     // LD A,(HL)
    rom[0x0065] = 0xE6; rom[0x0066] = 0xF0;                // AND 0xF0
    rom[0x0067] = 0x0F;                                     // RRCA
    rom[0x0068] = 0x0F;                                     // RRCA
    rom[0x0069] = 0x0F;                                     // RRCA
    rom[0x006A] = 0x0F;                                     // RRCA
    rom[0x006B] = 0xD3; rom[0x006C] = 0x61;                // OUT (0x61),A
    rom[0x006D] = 0x32; rom[0x006E] = 0x02; rom[0x006F] = 0x81;  // LD (0x8102),A
    rom[0x0070] = 0x23;                                     // INC HL
    rom[0x0071] = 0x22; rom[0x0072] = 0x08; rom[0x0073] = 0x81;  // LD (0x8108),HL
    rom[0x0074] = 0x3E; rom[0x0075] = 0x83;                // LD A,0x83
    rom[0x0076] = 0xD3; rom[0x0077] = 0x60;                // OUT (0x60),A
    rom[0x0078] = 0xF1;                                     // POP AF
    rom[0x0079] = 0xFB;                                     // EI
    rom[0x007A] = 0xED; rom[0x007B] = 0x4D;                // RETI

    rom[0x0120] = 0xA0;                                     // First packed nibble = 0x0A

    return rom;
}

auto make_ym2151_busy_poll_runtime_rom() -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);

    rom[0x0000] = 0xF3;                                     // DI
    rom[0x0001] = 0x3E; rom[0x0002] = 0x20;                // LD A,0x20
    rom[0x0003] = 0xD3; rom[0x0004] = 0x40;                // OUT (0x40),A
    rom[0x0005] = 0x3E; rom[0x0006] = 0x7F;                // LD A,0x7F
    rom[0x0007] = 0xD3; rom[0x0008] = 0x41;                // OUT (0x41),A
    rom[0x0009] = 0xDB; rom[0x000A] = 0x40;                // IN A,(0x40)
    rom[0x000B] = 0xED; rom[0x000C] = 0x64; rom[0x000D] = 0x80;  // TST 0x80
    rom[0x000E] = 0x20; rom[0x000F] = 0xF9;                // JR NZ,0x0009
    rom[0x0010] = 0x3E; rom[0x0011] = 0x5A;                // LD A,0x5A
    rom[0x0012] = 0x32; rom[0x0013] = 0x00; rom[0x0014] = 0x81;  // LD (0x8100),A
    rom[0x0015] = 0x76;                                     // HALT

    return rom;
}

auto make_pacman_boot_background_blocker_rom() -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);

    rom[0x0000] = 0xC3; rom[0x0001] = 0x40; rom[0x0002] = 0x01;  // JP 0x0140

    rom[0x0140] = 0x06; rom[0x0141] = 0x12;                      // LD B,0x12
    rom[0x0142] = 0xC3; rom[0x0143] = 0x14; rom[0x0144] = 0x03;  // JP 0x0314

    rom[0x0314] = 0xED; rom[0x0315] = 0x56;                      // IM 1
    rom[0x0316] = 0x76;                                          // HALT

    return rom;
}

auto make_pacman_palette_vclk_blocker_rom() -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);

    rom[0x0000] = 0xC3; rom[0x0001] = 0xB9; rom[0x0002] = 0x02;  // JP 0x02B9
    rom[0x02B9] = 0x05;                                          // DEC B
    rom[0x02BA] = 0x76;                                          // HALT

    return rom;
}

auto make_pacmanv8_vdp_b_bring_up_blocker_rom() -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);

    rom[0x0000] = 0xC3; rom[0x0001] = 0xFD; rom[0x0002] = 0x01;  // JP 0x01FD

    rom[0x01FD] = 0x01; rom[0x01FE] = 0x78; rom[0x01FF] = 0x56;  // LD BC,0x5678
    rom[0x0200] = 0x76;                                          // HALT

    return rom;
}

auto make_pacmanv8_vdp_b_vram_seek_blocker_rom() -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);

    rom[0x0000] = 0xC3; rom[0x0001] = 0x3B; rom[0x0002] = 0x02;  // JP 0x023B

    rom[0x023B] = 0x01; rom[0x023C] = 0xA5; rom[0x023D] = 0xBB;  // LD BC,0xBBA5
    rom[0x023E] = 0x79;                                          // LD A,C
    rom[0x023F] = 0x76;                                          // HALT

    return rom;
}

auto make_pacmanv8_vdp_b_vram_seek_ld_a_b_blocker_rom() -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);

    rom[0x0000] = 0xC3; rom[0x0001] = 0x3B; rom[0x0002] = 0x02;  // JP 0x023B

    rom[0x023B] = 0x01; rom[0x023C] = 0xA5; rom[0x023D] = 0xBB;  // LD BC,0xBBA5
    rom[0x023E] = 0x78;                                          // LD A,B
    rom[0x023F] = 0x76;                                          // HALT

    return rom;
}

auto make_pacmanv8_vdp_b_vram_seek_or_n_blocker_rom() -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);

    rom[0x0000] = 0xC3; rom[0x0001] = 0x3F; rom[0x0002] = 0x02;  // JP 0x023F

    rom[0x023F] = 0x3E; rom[0x0240] = 0x30;                      // LD A,0x30
    rom[0x0241] = 0xF6; rom[0x0242] = 0x0C;                      // OR 0x0C
    rom[0x0243] = 0x76;                                          // HALT

    return rom;
}

auto make_pacmanv8_vdp_b_framebuffer_load_ld_de_nn_blocker_rom() -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);

    rom[0x0000] = 0xC3; rom[0x0001] = 0x0B; rom[0x0002] = 0x02;  // JP 0x020B

    rom[0x020B] = 0x11; rom[0x020C] = 0x00; rom[0x020D] = 0x40;  // LD DE,0x4000
    rom[0x020E] = 0x76;                                          // HALT

    return rom;
}

auto make_pacmanv8_vdp_b_framebuffer_copy_ld_a_d_blocker_rom() -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);

    rom[0x0000] = 0xC3; rom[0x0001] = 0x43; rom[0x0002] = 0x02;  // JP 0x0243

    rom[0x0243] = 0x11; rom[0x0244] = 0x78; rom[0x0245] = 0x56;  // LD DE,0x5678
    rom[0x0246] = 0x7A;                                          // LD A,D
    rom[0x0247] = 0x76;                                          // HALT

    return rom;
}

auto make_pacmanv8_vdp_b_framebuffer_copy_or_e_blocker_rom() -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);

    rom[0x0000] = 0xC3; rom[0x0001] = 0x43; rom[0x0002] = 0x02;  // JP 0x0243

    rom[0x0243] = 0x11; rom[0x0244] = 0x0C; rom[0x0245] = 0x30;  // LD DE,0x300C
    rom[0x0246] = 0x7A;                                          // LD A,D  (A=0x30)
    rom[0x0247] = 0xB3;                                          // OR E    (A |= 0x0C = 0x3C)
    rom[0x0248] = 0x76;                                          // HALT

    return rom;
}

auto make_pacmanv8_vdp_b_framebuffer_copy_ret_z_and_dec_de_blocker_rom()
    -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);

    rom[0x0000] = 0xC3; rom[0x0001] = 0x43; rom[0x0002] = 0x02;  // JP 0x0243

    rom[0x0243] = 0x11; rom[0x0244] = 0x0C; rom[0x0245] = 0x30;  // LD DE,0x300C
    rom[0x0246] = 0x7A;                                          // LD A,D  (A=0x30)
    rom[0x0247] = 0xB3;                                          // OR E    (A=0x3C, Z=0)
    rom[0x0248] = 0xC8;                                          // RET Z (not taken, Z=0)
    rom[0x0249] = 0x00; rom[0x024A] = 0x00;                      // NOP, NOP
    rom[0x024B] = 0x00; rom[0x024C] = 0x00;                      // NOP, NOP
    rom[0x024D] = 0x1B;                                          // DEC DE  (DE=0x300B)
    rom[0x024E] = 0x76;                                          // HALT

    return rom;
}

auto make_pacmanv8_vblank_push_pop_rr_blocker_rom() -> std::vector<std::uint8_t> {
    // Reproduces the PacManV8 VBlank handler prologue/epilogue that exposed
    // the missing register-pair stack opcodes documented in
    // /home/djglxxii/src/PacManV8/docs/field-manual/vanguard8-build-directory-skew-and-timed-opcodes.md.
    // The test driver jumps execution directly to 0x0038 (via state snapshot)
    // so the prologue/epilogue opcode PCs line up exactly with the evidence:
    //
    //   0038: F5        PUSH AF
    //   0039: C5        PUSH BC   ; first previously-missing opcode
    //   003A: D5        PUSH DE
    //   003B: E5        PUSH HL
    //   003C..003F: 00  NOP       ; stand-in for the audio_update_frame call
    //   0040: E1        POP HL
    //   0041: D1        POP DE
    //   0042: C1        POP BC
    //   0043: F1        POP AF
    //   0044: 76        HALT
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);

    rom[0x0038] = 0xF5;                                          // PUSH AF
    rom[0x0039] = 0xC5;                                          // PUSH BC
    rom[0x003A] = 0xD5;                                          // PUSH DE
    rom[0x003B] = 0xE5;                                          // PUSH HL
    rom[0x0040] = 0xE1;                                          // POP HL
    rom[0x0041] = 0xD1;                                          // POP DE
    rom[0x0042] = 0xC1;                                          // POP BC
    rom[0x0043] = 0xF1;                                          // POP AF
    rom[0x0044] = 0x76;                                          // HALT

    return rom;
}

auto make_pacmanv8_rom_run_opcode_coverage_rom() -> std::vector<std::uint8_t> {
    // Exercises every M31 opcode family in a single scripted ROM:
    //   0000: 3E 05    LD A,0x05
    //   0002: 3D       DEC A           ; INC/DEC r family; A=0x04, flags set
    //   0003: 3C       INC A           ; A=0x05
    //   0004: FE 05    CP n            ; A==n -> Z=1
    //   0006: 28 02    JR Z,+2         ; taken -> PC=0x000A
    //   0008: 76       HALT            ; should not be reached
    //   0009: 00       NOP
    //   000A: 06 42    LD B,0x42
    //   000C: 57       LD D,A          ; LD r,r' family: D=0x05
    //   000D: 48       LD C,B          ; LD r,r' family: C=0x42
    //   000E: CA 12 00 JP Z,0x0012     ; taken (Z still set from CP)
    //   0011: 76       HALT            ; should not be reached
    //   0012: 3E 00    LD A,0x00
    //   0014: B7       OR A            ; Z=1
    //   0015: C2 1A 00 JP NZ,0x001A    ; not taken (Z=1)
    //   0018: 18 02    JR e,+2         ; branch to 0x001C
    //   001A: 76       HALT            ; not reached
    //   001B: 00       NOP
    //   001C: 3E 10    LD A,0x10
    //   001E: BE       CP (HL)         ; HL=0x8000 memory=0x10 -> Z=1 after
    //   001F: 76       HALT
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);
    rom[0x0000] = 0x3E; rom[0x0001] = 0x05;
    rom[0x0002] = 0x3D;
    rom[0x0003] = 0x3C;
    rom[0x0004] = 0xFE; rom[0x0005] = 0x05;
    rom[0x0006] = 0x28; rom[0x0007] = 0x02;
    rom[0x0008] = 0x76;
    rom[0x0009] = 0x00;
    rom[0x000A] = 0x06; rom[0x000B] = 0x42;
    rom[0x000C] = 0x57;
    rom[0x000D] = 0x48;
    rom[0x000E] = 0xCA; rom[0x000F] = 0x12; rom[0x0010] = 0x00;
    rom[0x0011] = 0x76;
    rom[0x0012] = 0x3E; rom[0x0013] = 0x00;
    rom[0x0014] = 0xB7;
    rom[0x0015] = 0xC2; rom[0x0016] = 0x1A; rom[0x0017] = 0x00;
    rom[0x0018] = 0x18; rom[0x0019] = 0x02;
    rom[0x001A] = 0x76;
    rom[0x001B] = 0x00;
    rom[0x001C] = 0x3E; rom[0x001D] = 0x10;
    rom[0x001E] = 0xBE;
    rom[0x001F] = 0x76;
    return rom;
}

auto make_pacmanv8_intermission_opcode_coverage_rom() -> std::vector<std::uint8_t> {
    // Exercises the M34 opcode families in one scripted frame-loop ROM:
    //   0000: 06 02    LD B,0x02
    //   0002: 10 02    DJNZ +2        ; taken -> PC=0x0006, B=0x01
    //   0004: 76       HALT           ; skipped
    //   0006: 3E 80    LD A,0x80
    //   0008: 87       ADD A,A        ; A=0x00, C=1
    //   0009: 38 02    JR C,+2        ; taken -> PC=0x000D
    //   000B: 76       HALT           ; skipped
    //   000D: B7       OR A           ; clears C
    //   000E: 30 02    JR NC,+2       ; taken -> PC=0x0012
    //   0010: 76       HALT           ; skipped
    //   0012: 21 FF 0F LD HL,0x0FFF
    //   0015: 01 01 00 LD BC,0x0001
    //   0018: 11 01 00 LD DE,0x0001
    //   001B: 09       ADD HL,BC      ; HL=0x1000
    //   001C: 19       ADD HL,DE      ; HL=0x1001
    //   001D: ED 52    SBC HL,DE      ; HL=0x1000
    //   001F: 21 00 80 LD HL,0x8000
    //   0022: 3E 0F    LD A,0x0F
    //   0024: 86       ADD A,(HL)     ; memory=0x01 -> A=0x10
    //   0025: 76       HALT
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);
    rom[0x0000] = 0x06; rom[0x0001] = 0x02;
    rom[0x0002] = 0x10; rom[0x0003] = 0x02;
    rom[0x0004] = 0x76;
    rom[0x0005] = 0x00;
    rom[0x0006] = 0x3E; rom[0x0007] = 0x80;
    rom[0x0008] = 0x87;
    rom[0x0009] = 0x38; rom[0x000A] = 0x02;
    rom[0x000B] = 0x76;
    rom[0x000C] = 0x00;
    rom[0x000D] = 0xB7;
    rom[0x000E] = 0x30; rom[0x000F] = 0x02;
    rom[0x0010] = 0x76;
    rom[0x0011] = 0x00;
    rom[0x0012] = 0x21; rom[0x0013] = 0xFF; rom[0x0014] = 0x0F;
    rom[0x0015] = 0x01; rom[0x0016] = 0x01; rom[0x0017] = 0x00;
    rom[0x0018] = 0x11; rom[0x0019] = 0x01; rom[0x001A] = 0x00;
    rom[0x001B] = 0x09;
    rom[0x001C] = 0x19;
    rom[0x001D] = 0xED; rom[0x001E] = 0x52;
    rom[0x001F] = 0x21; rom[0x0020] = 0x00; rom[0x0021] = 0x80;
    rom[0x0022] = 0x3E; rom[0x0023] = 0x0F;
    rom[0x0024] = 0x86;
    rom[0x0025] = 0x76;
    return rom;
}

auto read_binary_file(const std::filesystem::path& path) -> std::vector<std::uint8_t> {
    std::ifstream input(path, std::ios::binary);
    return std::vector<std::uint8_t>(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>()
    );
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

void write_palette_entry(
    vanguard8::core::Bus& bus,
    const std::uint16_t palette_port,
    const std::uint8_t index,
    const std::uint8_t rg,
    const std::uint8_t b
) {
    bus.write_port(palette_port, index);
    bus.write_port(palette_port, rg);
    bus.write_port(palette_port, b);
}

void write_ay_register(vanguard8::core::Bus& bus, const std::uint8_t reg, const std::uint8_t value) {
    bus.write_port(0x50, reg);
    bus.write_port(0x51, value);
}

void program_integration_state(Emulator& emulator, const std::vector<std::uint8_t>& rom) {
    emulator.load_rom_image(rom);
    emulator.set_vclk_rate(VclkRate::hz_6000);
    emulator.clear_event_log();

    auto& bus = emulator.mutable_bus();
    auto& cpu = emulator.mutable_cpu();

    cpu.out0(0x3A, 0x48);
    cpu.out0(0x38, 0xF0);
    cpu.out0(0x39, 0x08);

    write_ay_register(bus, 0x00, 0x34);
    write_ay_register(bus, 0x01, 0x01);
    write_ay_register(bus, 0x07, 0x3E);
    write_ay_register(bus, 0x08, 0x0F);
    bus.write_port(0x61, 0x07);
    bus.write_port(0x60, 0x01);

    write_vdp_register(bus, 0x81, 0, V9938::graphic3_mode_r0);
    write_vdp_register(bus, 0x81, 1, static_cast<std::uint8_t>(V9938::graphic_mode_r1 | 0x20U | 0x40U));
    write_vdp_register(bus, 0x81, 8, 0x20);
    write_vdp_register(bus, 0x85, 0, V9938::graphic4_mode_r0);
    write_vdp_register(bus, 0x85, 1, static_cast<std::uint8_t>(V9938::graphic_mode_r1 | 0x20U | 0x40U));

    write_palette_entry(bus, 0x82, 0x00, 0x00, 0x00);
    write_palette_entry(bus, 0x82, 0x04, 0x70, 0x00);
    write_palette_entry(bus, 0x86, 0x02, 0x07, 0x00);

    bus.mutable_vdp_a().poke_vram(0x0000, 0x01);
    bus.mutable_vdp_a().poke_vram(static_cast<std::uint16_t>(V9938::graphic3_pattern_base + 0x0008), 0x80);
    bus.mutable_vdp_a().poke_vram(static_cast<std::uint16_t>(V9938::graphic3_color_base + 0x0008), 0x40);
    bus.mutable_vdp_b().poke_vram(0x0000, 0x22);

    bus.mutable_controller_ports().load_state(
        vanguard8::core::io::ControllerPortsState{.port_state = {0xFE, 0xFD}}
    );
}

auto first_two_pixels(const V9938::Framebuffer& frame)
    -> std::array<std::array<std::uint8_t, 3>, 2> {
    return {{
        {frame[0], frame[1], frame[2]},
        {frame[3], frame[4], frame[5]},
    }};
}

}  // namespace

TEST_CASE("bank switching interrupts audio and compositing stay deterministic together", "[integration]") {
    const auto rom = make_integration_rom();

    Emulator first;
    program_integration_state(first, rom);
    first.run_frames(1);

    REQUIRE(first.mutable_cpu().peek_logical(0x4000) == 0xB2);
    REQUIRE(first.cpu().bank_switch_log().size() == 1);
    REQUIRE(first.cpu().bank_switch_log().front().bbr == 0x08);
    REQUIRE(first.cpu().bank_switch_log().front().legal);
    REQUIRE(first.bus().int0_asserted());
    REQUIRE(first.bus().int1_asserted());
    REQUIRE(first.audio_output_sample_count() > 0U);

    const auto has_vblank = std::any_of(
        first.event_log().begin(),
        first.event_log().end(),
        [](const auto& event) { return event.type == EventType::vblank; }
    );
    const auto has_vclk = std::any_of(
        first.event_log().begin(),
        first.event_log().end(),
        [](const auto& event) { return event.type == EventType::vclk; }
    );

    REQUIRE(has_vblank);
    REQUIRE(has_vclk);

    const auto first_frame = Compositor::compose_dual_vdp(first.vdp_a(), first.vdp_b());
    const auto first_pixels = first_two_pixels(first_frame);
    REQUIRE(first_pixels[0] == first.vdp_a().palette_entry_rgb(4));
    REQUIRE(first_pixels[1] == first.vdp_b().palette_entry_rgb(2));

    Emulator second;
    program_integration_state(second, rom);
    second.run_frames(1);
    const auto second_frame = Compositor::compose_dual_vdp(second.vdp_a(), second.vdp_b());

    REQUIRE(second.mutable_cpu().peek_logical(0x4000) == 0xB2);
    REQUIRE(second.cpu().bank_switch_log().size() == 1);
    REQUIRE(second.event_log_digest() == first.event_log_digest());
    REQUIRE(second.audio_output_digest() == first.audio_output_digest());
    REQUIRE(second_frame == first_frame);
}

TEST_CASE("frame loop executes ROM instructions and reaches ROM-driven VDP state", "[integration]") {
    Emulator emulator;
    emulator.load_rom_image(make_frame_loop_rom());
    emulator.run_frames(1);

    REQUIRE(emulator.vdp_a().register_value(7) == 0x01);
    REQUIRE(emulator.vdp_a().palette_entry_raw(1) == std::array<std::uint8_t, 2>{0x07, 0x00});

    const auto frame = Compositor::compose_dual_vdp(emulator.vdp_a(), emulator.vdp_b());
    REQUIRE_FALSE(frame.empty());
    REQUIRE(std::array<std::uint8_t, 3>{frame[0], frame[1], frame[2]} == emulator.vdp_a().palette_entry_rgb(1));
}

TEST_CASE("frame loop writes high-address Graphic 6 HUD data without aliasing into low VRAM", "[integration]") {
    Emulator emulator;
    emulator.load_rom_image(make_high_address_graphic6_runtime_rom());
    emulator.run_frames(1);

    REQUIRE(emulator.vdp_a().vram()[0x6400] == 0x40);
    REQUIRE(emulator.vdp_a().vram()[0x2400] == 0x00);

    const auto frame = Compositor::compose_dual_vdp(emulator.vdp_a(), emulator.vdp_b());
    const auto pixel0_base = static_cast<std::size_t>(((100 * V9938::max_visible_width) + 0) * 3);
    const auto pixel1_base = static_cast<std::size_t>(((100 * V9938::max_visible_width) + 1) * 3);
    const auto pixel0 = std::array<std::uint8_t, 3>{
        frame[pixel0_base + 0],
        frame[pixel0_base + 1],
        frame[pixel0_base + 2],
    };
    const auto pixel1 = std::array<std::uint8_t, 3>{
        frame[pixel1_base + 0],
        frame[pixel1_base + 1],
        frame[pixel1_base + 2],
    };

    REQUIRE(pixel0 == emulator.vdp_a().palette_entry_rgb(4));
    REQUIRE(pixel1 == emulator.vdp_b().palette_entry_rgb(2));
}

TEST_CASE("frame loop can service a ROM-driven INT1 handler through the normal runtime path", "[integration]") {
    Emulator emulator;
    emulator.load_rom_image(make_int1_runtime_rom(0x00));
    emulator.set_vclk_rate(VclkRate::hz_8000);

    REQUIRE_NOTHROW(emulator.run_frames(1));
    REQUIRE(emulator.mutable_cpu().peek_logical(0x8102) == 0x55);
    REQUIRE(emulator.bus().msm5205().vclk_count() > 0U);
}

TEST_CASE("scheduled frame loop ages YM2151 busy state between ROM poll instructions", "[integration]") {
    Emulator first;
    first.load_rom_image(make_ym2151_busy_poll_runtime_rom());
    REQUIRE_NOTHROW(first.run_frames(1));

    Emulator second;
    second.load_rom_image(make_ym2151_busy_poll_runtime_rom());
    REQUIRE_NOTHROW(second.run_frames(1));

    REQUIRE(first.cpu().halted());
    REQUIRE(first.cpu().pc() == 0x0015);
    REQUIRE(static_cast<std::uint8_t>(first.cpu().state_snapshot().registers.af >> 8U) == 0x5A);
    REQUIRE(first.completed_frames() == 1);
    REQUIRE(first.completed_frames() == second.completed_frames());
    REQUIRE(first.cpu().state_snapshot().registers.af == second.cpu().state_snapshot().registers.af);
    REQUIRE(first.cpu().pc() == second.cpu().pc());
    REQUIRE(first.event_log_digest() == second.event_log_digest());
    REQUIRE(first.audio_output_sample_count() == second.audio_output_sample_count());
    REQUIRE(first.audio_output_digest() == second.audio_output_digest());
}

TEST_CASE("frame loop executes the first blocked audio window through YM polling and INT1 nibble feed", "[integration]") {
    Emulator emulator;
    emulator.load_rom_image(make_audio_window_runtime_rom());
    emulator.set_vclk_rate(VclkRate::hz_8000);

    REQUIRE_NOTHROW(emulator.run_frames(2));
    REQUIRE(emulator.mutable_cpu().peek_logical(0x8104) == 0xCC);
    REQUIRE(emulator.mutable_cpu().peek_logical(0x8102) == 0x0A);
    REQUIRE(emulator.mutable_cpu().peek_logical(0x8108) == 0x21);
    REQUIRE(emulator.mutable_cpu().peek_logical(0x8109) == 0x01);
    REQUIRE(emulator.bus().msm5205().latched_nibble() == 0x0A);
    REQUIRE(emulator.bus().msm5205().vclk_count() > 0U);
    REQUIRE(emulator.bus().ym2151().latched_address() == 0x20);
}

TEST_CASE("frame loop clears the Pac-Man boot-background opcode blockers at PCs 0x0140 and 0x0314", "[integration]") {
    Emulator emulator;
    emulator.load_rom_image(make_pacman_boot_background_blocker_rom());

    REQUIRE_NOTHROW(emulator.run_frames(1));
    REQUIRE(static_cast<std::uint8_t>(emulator.cpu().state_snapshot().registers.bc >> 8U) == 0x12);
    REQUIRE(emulator.cpu().interrupt_mode() == 1);
    REQUIRE(emulator.cpu().pc() == 0x0316);
    REQUIRE(emulator.cpu().halted());
}

TEST_CASE("frame loop clears the Pac-Man palette opcode blocker at PC 0x02B9 with VCLK stopped", "[integration]") {
    Emulator emulator;
    emulator.load_rom_image(make_pacman_palette_vclk_blocker_rom());
    emulator.set_vclk_rate(VclkRate::stopped);

    REQUIRE_NOTHROW(emulator.run_frames(1));
    REQUIRE(static_cast<std::uint8_t>(emulator.cpu().state_snapshot().registers.bc >> 8U) == 0xFF);
    REQUIRE(emulator.cpu().pc() == 0x02BA);
    REQUIRE(emulator.cpu().halted());
    REQUIRE(emulator.bus().msm5205().vclk_count() == 0U);
}

TEST_CASE("frame loop clears the Pac-Man palette opcode blocker at PC 0x02B9 with VCLK 4000", "[integration]") {
    Emulator emulator;
    emulator.load_rom_image(make_pacman_palette_vclk_blocker_rom());
    emulator.set_vclk_rate(VclkRate::hz_4000);

    REQUIRE_NOTHROW(emulator.run_frames(1));
    REQUIRE(static_cast<std::uint8_t>(emulator.cpu().state_snapshot().registers.bc >> 8U) == 0xFF);
    REQUIRE(emulator.cpu().pc() == 0x02BA);
    REQUIRE(emulator.cpu().halted());
    REQUIRE(emulator.bus().msm5205().vclk_count() > 0U);
}

TEST_CASE("frame loop clears the PacManV8 VDP-B bring-up opcode blocker at PC 0x01FD", "[integration]") {
    Emulator emulator;
    emulator.load_rom_image(make_pacmanv8_vdp_b_bring_up_blocker_rom());

    REQUIRE_NOTHROW(emulator.run_frames(1));
    REQUIRE(emulator.cpu().state_snapshot().registers.bc == 0x5678);
    REQUIRE(emulator.cpu().pc() == 0x0200);
    REQUIRE(emulator.cpu().halted());
}

TEST_CASE("frame loop clears the PacManV8 VDP-B VRAM seek opcode blocker at PC 0x023B", "[integration]") {
    Emulator emulator;
    emulator.load_rom_image(make_pacmanv8_vdp_b_vram_seek_blocker_rom());

    REQUIRE_NOTHROW(emulator.run_frames(1));
    REQUIRE(emulator.cpu().state_snapshot().registers.bc == 0xBBA5);
    REQUIRE(static_cast<std::uint8_t>(emulator.cpu().state_snapshot().registers.af >> 8U) == 0xA5);
    REQUIRE(emulator.cpu().pc() == 0x023F);
    REQUIRE(emulator.cpu().halted());
}

TEST_CASE("frame loop clears the PacManV8 VDP-B VRAM seek LD A,B opcode blocker at PC 0x023E", "[integration]") {
    Emulator emulator;
    emulator.load_rom_image(make_pacmanv8_vdp_b_vram_seek_ld_a_b_blocker_rom());

    REQUIRE_NOTHROW(emulator.run_frames(1));
    REQUIRE(emulator.cpu().state_snapshot().registers.bc == 0xBBA5);
    REQUIRE(static_cast<std::uint8_t>(emulator.cpu().state_snapshot().registers.af >> 8U) == 0xBB);
    REQUIRE(emulator.cpu().pc() == 0x023F);
    REQUIRE(emulator.cpu().halted());
}

TEST_CASE("frame loop clears the PacManV8 VDP-B VRAM seek OR n opcode blocker at PC 0x0241", "[integration]") {
    Emulator emulator;
    emulator.load_rom_image(make_pacmanv8_vdp_b_vram_seek_or_n_blocker_rom());

    REQUIRE_NOTHROW(emulator.run_frames(1));
    REQUIRE(static_cast<std::uint8_t>(emulator.cpu().state_snapshot().registers.af >> 8U) == 0x3C);
    REQUIRE(emulator.cpu().pc() == 0x0243);
    REQUIRE(emulator.cpu().halted());
}

TEST_CASE("frame loop clears the PacManV8 VDP-B framebuffer-load LD DE,nn opcode blocker at PC 0x020B", "[integration]") {
    Emulator emulator;
    emulator.load_rom_image(make_pacmanv8_vdp_b_framebuffer_load_ld_de_nn_blocker_rom());

    REQUIRE_NOTHROW(emulator.run_frames(1));
    REQUIRE(emulator.cpu().state_snapshot().registers.de == 0x4000);
    REQUIRE(emulator.cpu().pc() == 0x020E);
    REQUIRE(emulator.cpu().halted());
}

TEST_CASE("frame loop clears the PacManV8 VDP-B framebuffer copy LD A,D opcode blocker at PC 0x0246", "[integration]") {
    Emulator emulator;
    emulator.load_rom_image(make_pacmanv8_vdp_b_framebuffer_copy_ld_a_d_blocker_rom());

    REQUIRE_NOTHROW(emulator.run_frames(1));
    REQUIRE(emulator.cpu().state_snapshot().registers.de == 0x5678);
    REQUIRE(static_cast<std::uint8_t>(emulator.cpu().state_snapshot().registers.af >> 8U) == 0x56);
    REQUIRE(emulator.cpu().pc() == 0x0247);
    REQUIRE(emulator.cpu().halted());
}

TEST_CASE("frame loop clears the PacManV8 VDP-B framebuffer copy OR E opcode blocker at PC 0x0247", "[integration]") {
    Emulator emulator;
    emulator.load_rom_image(make_pacmanv8_vdp_b_framebuffer_copy_or_e_blocker_rom());

    REQUIRE_NOTHROW(emulator.run_frames(1));
    REQUIRE(static_cast<std::uint8_t>(emulator.cpu().state_snapshot().registers.af >> 8U) == 0x3C);
    REQUIRE(emulator.cpu().pc() == 0x0248);
    REQUIRE(emulator.cpu().halted());
}

TEST_CASE("frame loop clears the PacManV8 VDP-B framebuffer copy RET Z and DEC DE opcode blockers at PC 0x0248 and 0x024D", "[integration]") {
    Emulator emulator;
    emulator.load_rom_image(make_pacmanv8_vdp_b_framebuffer_copy_ret_z_and_dec_de_blocker_rom());

    REQUIRE_NOTHROW(emulator.run_frames(1));
    REQUIRE(emulator.cpu().state_snapshot().registers.de == 0x300B);
    REQUIRE(static_cast<std::uint8_t>(emulator.cpu().state_snapshot().registers.af >> 8U) == 0x3C);
    REQUIRE(emulator.cpu().pc() == 0x024E);
    REQUIRE(emulator.cpu().halted());
}

TEST_CASE("frame loop clears the PacManV8 VBlank handler PUSH/POP rr opcode blockers at PC 0x0039..0x0042", "[integration]") {
    Emulator emulator;
    emulator.load_rom_image(make_pacmanv8_vblank_push_pop_rr_blocker_rom());

    // Stage the boot MMU (CBAR=0x48, CBR=0xF0, BBR=0x04) so SP=0x8100 lands in
    // SRAM and the pushes are visible to the pops, and jump the PC to 0x0038
    // so the previously-failing opcode PCs match the field manual evidence.
    auto state = emulator.mutable_cpu().state_snapshot();
    state.registers.cbar = 0x48;
    state.registers.cbr = 0xF0;
    state.registers.bbr = 0x04;
    state.registers.sp = 0x8100;
    state.registers.pc = 0x0038;
    state.registers.af = 0x11AA;
    state.registers.bc = 0x2233;
    state.registers.de = 0x4455;
    state.registers.hl = 0x6677;
    emulator.mutable_cpu().load_state_snapshot(state);

    REQUIRE_NOTHROW(emulator.run_frames(1));

    const auto final_state = emulator.cpu().state_snapshot();
    REQUIRE(emulator.cpu().pc() == 0x0044);
    REQUIRE(emulator.cpu().halted());
    // After four pushes and four pops in reverse order, SP returns to 0x8100
    // and AF/BC/DE/HL match the values staged above.
    REQUIRE(final_state.registers.sp == 0x8100);
    REQUIRE(final_state.registers.af == 0x11AA);
    REQUIRE(final_state.registers.bc == 0x2233);
    REQUIRE(final_state.registers.de == 0x4455);
    REQUIRE(final_state.registers.hl == 0x6677);
}

TEST_CASE("frame loop clears the PacManV8 ROM run-time opcode blockers (M31)", "[integration]") {
    Emulator emulator;
    emulator.load_rom_image(make_pacmanv8_rom_run_opcode_coverage_rom());

    // Point HL at the 0x8000 SRAM region with a known byte so CP (HL) at PC 0x001E
    // can compare A=0x10 against memory=0x10 and set Z.
    auto state = emulator.mutable_cpu().state_snapshot();
    state.registers.cbar = 0x48;
    state.registers.cbr = 0xF0;
    state.registers.bbr = 0x04;
    state.registers.hl = 0x8000;
    emulator.mutable_cpu().load_state_snapshot(state);
    emulator.mutable_cpu().write_logical(0x8000, 0x10);

    REQUIRE_NOTHROW(emulator.run_frames(1));

    const auto final_state = emulator.cpu().state_snapshot();
    REQUIRE(emulator.cpu().halted());
    // Only the terminal HALT at PC 0x001F should trigger; the three intermediate
    // HALTs at 0x0008, 0x0011, and 0x001A must be skipped by the conditional
    // jumps/returns under test.
    REQUIRE(emulator.cpu().pc() == 0x001F);
    // Final A=0x10 from LD A,0x10 at PC 0x001C; Z set by CP (HL).
    REQUIRE((final_state.registers.af >> 8U) == 0x10);
    REQUIRE((final_state.registers.af & 0x0040U) == 0x0040U);
    // D received A via LD D,A (0x57) when A was 0x05.
    REQUIRE((final_state.registers.de >> 8U) == 0x05);
    // C received B via LD C,B (0x48) when B was 0x42.
    REQUIRE((final_state.registers.bc & 0x00FFU) == 0x42);
}

TEST_CASE("frame loop clears the PacManV8 T020 intermission opcode blocker families (M34)", "[integration]") {
    Emulator emulator;
    emulator.load_rom_image(make_pacmanv8_intermission_opcode_coverage_rom());

    auto state = emulator.mutable_cpu().state_snapshot();
    state.registers.cbar = 0x48;
    state.registers.cbr = 0xF0;
    state.registers.bbr = 0x04;
    emulator.mutable_cpu().load_state_snapshot(state);
    emulator.mutable_cpu().write_logical(0x8000, 0x01);

    REQUIRE_NOTHROW(emulator.run_frames(1));

    const auto final_state = emulator.cpu().state_snapshot();
    REQUIRE(emulator.cpu().halted());
    REQUIRE(emulator.cpu().pc() == 0x0025);
    REQUIRE(final_state.registers.bc == 0x0001);
    REQUIRE(final_state.registers.hl == 0x8000);
    REQUIRE((final_state.registers.af >> 8U) == 0x10);
}

TEST_CASE("PacManV8 T020 headless repro remains deterministic across repeat runs", "[integration]") {
    const auto rom_path = std::filesystem::path("/home/djglxxii/src/PacManV8/build/pacman.rom");
    if (!std::filesystem::is_regular_file(rom_path)) {
        SKIP("PacManV8 pacman.rom is not available at " << rom_path.string());
    }

    const auto rom = read_binary_file(rom_path);
    auto run = [&rom]() {
        Emulator emulator;
        emulator.load_rom_image(rom);
        emulator.run_frames(700);

        const auto frame = Compositor::compose_dual_vdp(emulator.vdp_a(), emulator.vdp_b());
        return std::tuple{
            emulator.cpu().pc(),
            emulator.event_log_digest(),
            digest_hex(vanguard8::core::sha256(frame)),
        };
    };

    const auto [first_pc, first_event_log_digest, first_frame_hash] = run();
    const auto [second_pc, second_event_log_digest, second_frame_hash] = run();

    REQUIRE(first_pc == second_pc);
    REQUIRE(first_event_log_digest == second_event_log_digest);
    REQUIRE(first_frame_hash == second_frame_hash);
}

TEST_CASE("previously-unsupported opcodes in INT1 handler execute without error under MAME core", "[integration]") {
    Emulator emulator;
    emulator.load_rom_image(make_int1_runtime_rom(0xDD));  // IX prefix — valid in MAME core
    emulator.set_vclk_rate(VclkRate::hz_8000);

    REQUIRE_NOTHROW(emulator.run_frames(1));
}

TEST_CASE("showcase milestone 7 late loop leaves VDP-A in Graphic 6 and produces a 512 wide frame", "[integration]") {
    const auto rom_path =
        std::filesystem::path(__FILE__).parent_path().parent_path() / "build/showcase/showcase.rom";
    if (!std::filesystem::is_regular_file(rom_path)) {
        SKIP("showcase ROM not built");
    }

    Emulator emulator;
    emulator.load_rom_image(read_binary_file(rom_path));
    emulator.run_frames(1800);

    INFO("VDP-A R0=" << static_cast<int>(emulator.vdp_a().register_value(0)));
    INFO("VDP-A R1=" << static_cast<int>(emulator.vdp_a().register_value(1)));
    INFO("VDP-A R8=" << static_cast<int>(emulator.vdp_a().register_value(8)));
    INFO("VDP-B R0=" << static_cast<int>(emulator.vdp_b().register_value(0)));
    INFO("VDP-B R23=" << static_cast<int>(emulator.vdp_b().register_value(23)));

    const auto frame = Compositor::compose_dual_vdp(emulator.vdp_a(), emulator.vdp_b());
    REQUIRE(emulator.vdp_a().register_value(0) == V9938::graphic6_mode_r0);
    REQUIRE(emulator.vdp_a().register_value(1) == 0x40);
    REQUIRE(emulator.vdp_a().register_value(8) == 0x20);
    REQUIRE(emulator.vdp_b().register_value(0) == V9938::graphic4_mode_r0);
    REQUIRE(frame.size() ==
            static_cast<std::size_t>(V9938::max_visible_width * V9938::visible_height * 3));
}

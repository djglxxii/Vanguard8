#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "core/emulator.hpp"
#include "core/memory/cartridge.hpp"
#include "core/video/compositor.hpp"
#include "core/video/v9938.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <vector>

namespace {

using vanguard8::core::Emulator;
using vanguard8::core::EventType;
using vanguard8::core::VclkRate;
using vanguard8::core::video::Compositor;
using vanguard8::core::video::V9938;

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

auto make_pacman_boot_background_blocker_rom() -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);

    rom[0x0000] = 0xC3; rom[0x0001] = 0x40; rom[0x0002] = 0x01;  // JP 0x0140

    rom[0x0140] = 0x06; rom[0x0141] = 0x12;                      // LD B,0x12
    rom[0x0142] = 0xC3; rom[0x0143] = 0x14; rom[0x0144] = 0x03;  // JP 0x0314

    rom[0x0314] = 0xED; rom[0x0315] = 0x56;                      // IM 1
    rom[0x0316] = 0x76;                                          // HALT

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

TEST_CASE("unsupported handler opcodes are reported at the handler PC after INT1 dispatch", "[integration]") {
    Emulator emulator;
    emulator.load_rom_image(make_int1_runtime_rom(0xDD));
    emulator.set_vclk_rate(VclkRate::hz_8000);

    REQUIRE_THROWS_MATCHES(
        emulator.run_frames(1),
        std::runtime_error,
        Catch::Matchers::MessageMatches(
            Catch::Matchers::ContainsSubstring("PC 0x5a", Catch::CaseSensitive::No)
        )
    );
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

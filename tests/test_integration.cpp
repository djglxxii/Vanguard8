#include <catch2/catch_test_macros.hpp>

#include "core/emulator.hpp"
#include "core/memory/cartridge.hpp"
#include "core/video/compositor.hpp"
#include "core/video/v9938.hpp"

#include <array>
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
    write_vdp_register(bus, 0x81, 1, static_cast<std::uint8_t>(V9938::graphic_mode_r1 | 0x20U));
    write_vdp_register(bus, 0x81, 8, 0x20);
    write_vdp_register(bus, 0x85, 0, V9938::graphic4_mode_r0);
    write_vdp_register(bus, 0x85, 1, static_cast<std::uint8_t>(V9938::graphic_mode_r1 | 0x20U));

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

#include <catch2/catch_test_macros.hpp>

#include "core/video/compositor.hpp"
#include "core/video/v9938.hpp"
#include "frontend/display.hpp"
#include "frontend/video_fixture.hpp"

#include <array>
#include <filesystem>

namespace {

using vanguard8::core::video::Compositor;
using vanguard8::core::video::LayerMask;
using vanguard8::core::video::V9938;

void write_register(V9938& vdp, const std::uint8_t reg, const std::uint8_t value) {
    vdp.write_control(value);
    vdp.write_control(static_cast<std::uint8_t>(0x80U | reg));
}

void seek_vram_write(V9938& vdp, const std::uint16_t address) {
    vdp.write_control(static_cast<std::uint8_t>(address & 0xFFU));
    vdp.write_control(static_cast<std::uint8_t>(0x40U | ((address >> 8) & 0x3FU)));
}

void write_graphic4_byte(V9938& vdp, const int line, const int byte_index, const std::uint8_t value) {
    const auto address = static_cast<std::uint16_t>(line * V9938::bytes_per_scanline + byte_index);
    seek_vram_write(vdp, address);
    vdp.write_data(value);
}

void write_mode2_sprite(
    V9938& vdp,
    const std::uint8_t sprite_index,
    const std::uint8_t y,
    const std::uint8_t x,
    const std::uint8_t pattern_number,
    const std::uint8_t color_index,
    const bool invisible = false
) {
    const auto attribute_base =
        static_cast<std::uint16_t>(V9938::graphic4_sprite_attribute_base + sprite_index * 8U);
    vdp.poke_vram(attribute_base + 0, y);
    vdp.poke_vram(attribute_base + 1, x);
    vdp.poke_vram(attribute_base + 2, pattern_number);
    vdp.poke_vram(attribute_base + 3, 0x00);
    vdp.poke_vram(attribute_base + 4, 0x00);
    vdp.poke_vram(attribute_base + 5, 0x00);
    vdp.poke_vram(attribute_base + 6, 0x00);
    vdp.poke_vram(attribute_base + 7, 0x00);

    const auto color_base =
        static_cast<std::uint16_t>(V9938::graphic4_sprite_color_base + sprite_index * 16U);
    for (int row = 0; row < 16; ++row) {
        vdp.poke_vram(
            static_cast<std::uint16_t>(color_base + row),
            static_cast<std::uint8_t>((invisible ? 0x40U : 0x00U) | color_index)
        );
    }
}

void write_sprite_pattern_rows(V9938& vdp, const std::uint8_t pattern_number, const std::uint8_t row_value) {
    const auto base =
        static_cast<std::uint16_t>(V9938::graphic4_sprite_pattern_base + pattern_number * 8U);
    for (int row = 0; row < 8; ++row) {
        vdp.poke_vram(static_cast<std::uint16_t>(base + row), row_value);
    }
}

void terminate_sprite_list(V9938& vdp, const std::uint8_t sprite_index) {
    vdp.poke_vram(
        static_cast<std::uint16_t>(V9938::graphic4_sprite_attribute_base + sprite_index * 8U),
        0xD0
    );
}

}  // namespace

TEST_CASE("VDP data and palette ports mutate VRAM and palette state", "[video]") {
    V9938 vdp;

    seek_vram_write(vdp, 0x0000);
    vdp.write_data(0xAB);
    vdp.write_data(0xCD);

    REQUIRE(vdp.vram()[0x0000] == 0xAB);
    REQUIRE(vdp.vram()[0x0001] == 0xCD);

    vdp.write_palette(0x03);
    vdp.write_palette(0x75);
    vdp.write_palette(0x06);
    REQUIRE(vdp.palette_entry_raw(3)[0] == 0x75);
    REQUIRE(vdp.palette_entry_raw(3)[1] == 0x06);
}

TEST_CASE("Palette decoding expands 3 bit RGB channels correctly", "[video]") {
    V9938 vdp;

    vdp.write_palette(0x01);
    vdp.write_palette(0x75);
    vdp.write_palette(0x06);

    const auto rgb = vdp.palette_entry_rgb(1);
    REQUIRE(rgb[0] == V9938::expand3to8(7));
    REQUIRE(rgb[1] == V9938::expand3to8(5));
    REQUIRE(rgb[2] == V9938::expand3to8(6));
}

TEST_CASE("Graphic 4 rendering follows VRAM addressing across scanlines", "[video]") {
    V9938 vdp;

    write_register(vdp, 23, 0x00);
    vdp.write_palette(0x01);
    vdp.write_palette(0x70);
    vdp.write_palette(0x00);
    vdp.write_palette(0x07);
    vdp.write_palette(0x00);

    seek_vram_write(vdp, 0x0000);
    vdp.write_data(0x12);
    for (int index = 1; index < V9938::bytes_per_scanline; ++index) {
        vdp.write_data(0x00);
    }
    vdp.write_data(0x21);

    vdp.tick_scanline(0);
    REQUIRE(vdp.background_line_buffer()[0] == 0x01);
    REQUIRE(vdp.background_line_buffer()[1] == 0x02);

    vdp.tick_scanline(1);
    REQUIRE(vdp.background_line_buffer()[0] == 0x02);
    REQUIRE(vdp.background_line_buffer()[1] == 0x01);
}

TEST_CASE("R#23 vertical scroll wraps within the VRAM page", "[video]") {
    V9938 vdp;

    seek_vram_write(vdp, 0x0000);
    for (int y = 0; y < 256; ++y) {
        for (int x = 0; x < V9938::visible_width; x += 2) {
            const auto color = static_cast<std::uint8_t>(y & 0x0FU);
            vdp.write_data(static_cast<std::uint8_t>((color << 4) | color));
        }
    }

    write_register(vdp, 23, 0x03);
    vdp.tick_scanline(0);
    REQUIRE(vdp.background_line_buffer()[0] == 0x03);

    write_register(vdp, 23, 0xFF);
    vdp.tick_scanline(1);
    REQUIRE(vdp.background_line_buffer()[0] == 0x00);
}

TEST_CASE("VDP-A color 0 transparency reveals VDP-B while nonzero VDP-A pixels override", "[video]") {
    V9938 vdp_a;
    V9938 vdp_b;

    vdp_a.write_palette(0x00);
    vdp_a.write_palette(0x00);
    vdp_a.write_palette(0x00);
    vdp_a.write_palette(0x00);
    vdp_a.write_palette(0x00);
    vdp_a.write_palette(0x00);
    vdp_a.write_palette(0x00);
    vdp_a.write_palette(0x00);
    vdp_a.write_palette(0x00);
    vdp_a.write_palette(0x70);
    vdp_a.write_palette(0x00);

    vdp_b.write_palette(0x00);
    vdp_b.write_palette(0x00);
    vdp_b.write_palette(0x00);
    vdp_b.write_palette(0x00);
    vdp_b.write_palette(0x00);
    vdp_b.write_palette(0x07);
    vdp_b.write_palette(0x00);

    write_register(vdp_a, 8, 0x20);

    write_graphic4_byte(vdp_a, 0, 0, 0x04);
    write_graphic4_byte(vdp_b, 0, 0, 0x22);

    const auto frame = Compositor::compose_dual_vdp(vdp_a, vdp_b);
    const auto pixel0 = std::array<std::uint8_t, 3>{frame[0], frame[1], frame[2]};
    const auto pixel1 = std::array<std::uint8_t, 3>{frame[3], frame[4], frame[5]};

    REQUIRE(pixel0 == vdp_b.palette_entry_rgb(2));
    REQUIRE(pixel1 == vdp_a.palette_entry_rgb(4));
}

TEST_CASE("Layer toggles can hide VDP-A background while leaving VDP-A sprites visible", "[video]") {
    V9938 vdp_a;
    V9938 vdp_b;

    vdp_a.write_palette(0x00);
    vdp_a.write_palette(0x00);
    vdp_a.write_palette(0x00);
    vdp_a.write_palette(0x00);
    vdp_a.write_palette(0x00);
    vdp_a.write_palette(0x00);
    vdp_a.write_palette(0x00);
    vdp_a.write_palette(0x00);
    vdp_a.write_palette(0x00);
    vdp_a.write_palette(0x70);
    vdp_a.write_palette(0x00);

    vdp_b.write_palette(0x00);
    vdp_b.write_palette(0x00);
    vdp_b.write_palette(0x00);
    vdp_b.write_palette(0x00);
    vdp_b.write_palette(0x00);
    vdp_b.write_palette(0x07);
    vdp_b.write_palette(0x00);

    write_register(vdp_a, 8, 0x20);
    write_graphic4_byte(vdp_a, 0, 0, 0x44);
    write_graphic4_byte(vdp_b, 0, 0, 0x22);

    write_mode2_sprite(vdp_a, 0, 0, 0, 0, 0x04);
    write_sprite_pattern_rows(vdp_a, 0, 0x80);
    terminate_sprite_list(vdp_a, 1);

    const auto frame = Compositor::compose_dual_vdp(
        vdp_a,
        vdp_b,
        LayerMask{.hide_vdp_a_bg = true}
    );

    const auto sprite_pixel = std::array<std::uint8_t, 3>{frame[0], frame[1], frame[2]};
    const auto background_pixel = std::array<std::uint8_t, 3>{frame[3], frame[4], frame[5]};

    REQUIRE(sprite_pixel == vdp_a.palette_entry_rgb(4));
    REQUIRE(background_pixel == vdp_b.palette_entry_rgb(2));
}

TEST_CASE("Sprite overflow and collision flags behave for covered mode-2 cases", "[video]") {
    V9938 vdp;

    for (std::uint8_t sprite_index = 0; sprite_index < 9; ++sprite_index) {
        write_mode2_sprite(vdp, sprite_index, 10, static_cast<std::uint8_t>(sprite_index * 4), sprite_index, 0x03);
        write_sprite_pattern_rows(vdp, sprite_index, 0xC0);
    }
    terminate_sprite_list(vdp, 9);

    vdp.tick_scanline(10);
    REQUIRE((vdp.status_value(0) & 0x40U) != 0U);

    V9938 collision_vdp;
    write_mode2_sprite(collision_vdp, 0, 20, 32, 0, 0x05);
    write_mode2_sprite(collision_vdp, 1, 20, 32, 1, 0x06);
    write_sprite_pattern_rows(collision_vdp, 0, 0x80);
    write_sprite_pattern_rows(collision_vdp, 1, 0x80);
    terminate_sprite_list(collision_vdp, 2);

    collision_vdp.tick_scanline(20);
    REQUIRE((collision_vdp.status_value(0) & 0x20U) != 0U);
    REQUIRE(collision_vdp.status_value(3) == 32);
    REQUIRE(collision_vdp.status_value(5) == 20);
}

TEST_CASE("Reading S#0 and S#1 clears the documented VDP-A interrupt flags", "[video]") {
    V9938 vdp_a;
    V9938 vdp_b;

    write_register(vdp_a, 1, 0x20);
    vdp_a.set_vblank_flag(true);
    REQUIRE(vdp_a.int_pending());
    REQUIRE((vdp_a.read_status() & 0x80U) != 0U);
    REQUIRE(!vdp_a.int_pending());

    write_register(vdp_a, 0, 0x10);
    write_register(vdp_a, 19, 12);
    write_register(vdp_a, 15, 1);
    vdp_a.tick_scanline(12);
    REQUIRE(vdp_a.int_pending());
    REQUIRE((vdp_a.read_status() & 0x01U) != 0U);
    REQUIRE(!vdp_a.int_pending());

    write_register(vdp_b, 1, 0x20);
    vdp_b.set_vblank_flag(true);
    REQUIRE(vdp_b.int_pending());
}

TEST_CASE("Headless and display upload paths match for a known dual-VDP frame", "[video]") {
    V9938 vdp_a;
    V9938 vdp_b;
    vanguard8::frontend::build_dual_vdp_fixture_frame(vdp_a, vdp_b);

    const auto frame = Compositor::compose_dual_vdp(vdp_a, vdp_b);
    vanguard8::frontend::Display display;
    display.upload_frame(frame);

    REQUIRE(display.uploaded_frame() == frame);

    const auto ppm = display.dump_ppm_string();
    REQUIRE(ppm.rfind("P6\n256 212\n255\n", 0) == 0);

    const auto output_path =
        std::filesystem::temp_directory_path() / "vanguard8-m05-frame-dump.ppm";
    display.dump_ppm_file(output_path);
    REQUIRE(std::filesystem::exists(output_path));
}

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

void set_graphic4_mode(V9938& vdp) {
    write_register(vdp, 0, V9938::graphic4_mode_r0);
    write_register(vdp, 1, static_cast<std::uint8_t>(V9938::graphic_mode_r1 | 0x40U));
    write_register(
        vdp,
        5,
        static_cast<std::uint8_t>(V9938::graphic4_sprite_attribute_base >> 7U)
    );
    write_register(
        vdp,
        6,
        static_cast<std::uint8_t>(V9938::graphic4_sprite_pattern_base >> 11U)
    );
    write_register(
        vdp,
        11,
        static_cast<std::uint8_t>((V9938::graphic4_sprite_attribute_base >> 15U) & 0x03U)
    );
}

void set_graphic3_mode(V9938& vdp) {
    write_register(vdp, 0, V9938::graphic3_mode_r0);
    write_register(vdp, 1, static_cast<std::uint8_t>(V9938::graphic_mode_r1 | 0x40U));
    write_register(
        vdp,
        5,
        static_cast<std::uint8_t>(V9938::graphic3_sprite_attribute_base >> 7U)
    );
    write_register(
        vdp,
        6,
        static_cast<std::uint8_t>(V9938::graphic3_sprite_pattern_base >> 11U)
    );
    write_register(
        vdp,
        11,
        static_cast<std::uint8_t>((V9938::graphic3_sprite_attribute_base >> 15U) & 0x03U)
    );
}

void write_command_word(V9938& vdp, const std::uint8_t low_reg, const std::uint16_t value) {
    write_register(vdp, low_reg, static_cast<std::uint8_t>(value & 0x00FFU));
    write_register(vdp, static_cast<std::uint8_t>(low_reg + 1U), static_cast<std::uint8_t>((value >> 8U) & 0x0003U));
}

void seek_vram_write(V9938& vdp, const std::uint16_t address) {
    vdp.write_control(static_cast<std::uint8_t>(address & 0xFFU));
    vdp.write_control(static_cast<std::uint8_t>(0x40U | ((address >> 8) & 0x3FU)));
}

void seek_vram_read(V9938& vdp, const std::uint16_t address) {
    vdp.write_control(static_cast<std::uint8_t>(address & 0xFFU));
    vdp.write_control(static_cast<std::uint8_t>((address >> 8) & 0x3FU));
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

void write_16x16_sprite_pattern_rows(
    V9938& vdp,
    const std::uint8_t pattern_number,
    const std::uint8_t left_row_value,
    const std::uint8_t right_row_value
) {
    const auto aligned_pattern = static_cast<std::uint8_t>(pattern_number & 0xFCU);
    const auto base = static_cast<std::uint16_t>(
        V9938::graphic4_sprite_pattern_base + static_cast<std::uint16_t>(aligned_pattern) * 8U
    );
    for (int row = 0; row < 8; ++row) {
        vdp.poke_vram(static_cast<std::uint16_t>(base + row), left_row_value);
        vdp.poke_vram(static_cast<std::uint16_t>(base + 8U + row), right_row_value);
        vdp.poke_vram(static_cast<std::uint16_t>(base + 16U + row), left_row_value);
        vdp.poke_vram(static_cast<std::uint16_t>(base + 24U + row), right_row_value);
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

TEST_CASE("VRAM reads follow the V9938 read-ahead latch behavior", "[video]") {
    V9938 vdp;

    vdp.poke_vram(0x1234, 0xAA);
    vdp.poke_vram(0x1235, 0xBB);
    vdp.poke_vram(0x1236, 0xCC);

    seek_vram_read(vdp, 0x1234);
    REQUIRE(vdp.read_data() == 0x00);
    REQUIRE(vdp.read_data() == 0xAA);
    REQUIRE(vdp.read_data() == 0xBB);

    seek_vram_read(vdp, 0x1235);
    REQUIRE(vdp.read_data() == 0xCC);
    REQUIRE(vdp.read_data() == 0xBB);
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

    set_graphic4_mode(vdp);
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

    set_graphic4_mode(vdp);
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

TEST_CASE("R#1 bit 6 blanks the display to the backdrop color", "[video]") {
    V9938 vdp;

    set_graphic4_mode(vdp);
    write_register(vdp, 7, 0x03);
    seek_vram_write(vdp, 0x0000);
    vdp.write_data(0x12);

    vdp.tick_scanline(0);
    REQUIRE(vdp.line_buffer()[0] == 0x01);
    REQUIRE(vdp.line_buffer()[1] == 0x02);

    write_register(vdp, 1, V9938::graphic_mode_r1);
    vdp.tick_scanline(0);
    REQUIRE(vdp.background_line_buffer()[0] == 0x03);
    REQUIRE(vdp.background_line_buffer()[1] == 0x03);
    REQUIRE(vdp.line_buffer()[0] == 0x03);
    REQUIRE(vdp.line_buffer()[1] == 0x03);
}

TEST_CASE("HMMM copies bytes and CE clears at the documented cycle boundary", "[video]") {
    V9938 vdp;

    vdp.poke_vram(0x0000, 0x12);
    vdp.poke_vram(0x0001, 0x34);

    write_command_word(vdp, 32, 0x0000);
    write_command_word(vdp, 34, 0x0000);
    write_command_word(vdp, 36, 0x0000);
    write_command_word(vdp, 38, 0x0001);
    write_command_word(vdp, 40, 0x0002);
    write_command_word(vdp, 42, 0x0001);
    write_register(vdp, 45, 0x00);
    write_register(vdp, 46, 0xD0);

    REQUIRE((vdp.status_value(2) & 0x01U) != 0U);
    REQUIRE(vdp.vram()[V9938::bytes_per_scanline] == 0x12);
    REQUIRE(vdp.vram()[V9938::bytes_per_scanline + 1] == 0x34);

    vdp.advance_command(69);
    REQUIRE((vdp.status_value(2) & 0x01U) != 0U);

    vdp.advance_command(1);
    REQUIRE((vdp.status_value(2) & 0x01U) == 0U);
}

TEST_CASE("PSET and POINT operate on the Graphic 4 command surface", "[video]") {
    V9938 vdp;

    write_command_word(vdp, 36, 0x0003);
    write_command_word(vdp, 38, 0x0004);
    write_register(vdp, 44, 0x0A);
    write_register(vdp, 46, 0x50);
    REQUIRE((vdp.status_value(2) & 0x01U) != 0U);
    vdp.advance_command(1);

    write_command_word(vdp, 32, 0x0003);
    write_command_word(vdp, 34, 0x0004);
    write_register(vdp, 46, 0x40);
    vdp.advance_command(1);

    REQUIRE((vdp.status_value(7) & 0x0FU) == 0x0AU);
}

TEST_CASE("HMMV fill and SRCH boundary status follow the documented command registers", "[video]") {
    V9938 vdp;

    write_command_word(vdp, 36, 0x0000);
    write_command_word(vdp, 38, 0x0002);
    write_command_word(vdp, 40, 0x0002);
    write_command_word(vdp, 42, 0x0002);
    write_register(vdp, 44, 0xAB);
    write_register(vdp, 46, 0xC0);
    vdp.advance_command(1);

    REQUIRE(vdp.vram()[2 * V9938::bytes_per_scanline] == 0xAB);
    REQUIRE(vdp.vram()[2 * V9938::bytes_per_scanline + 1] == 0xAB);
    REQUIRE(vdp.vram()[3 * V9938::bytes_per_scanline] == 0xAB);
    REQUIRE(vdp.vram()[3 * V9938::bytes_per_scanline + 1] == 0xAB);

    write_command_word(vdp, 32, 0x0000);
    write_command_word(vdp, 34, 0x0002);
    write_register(vdp, 44, 0x0B);
    write_register(vdp, 45, 0x02);
    write_register(vdp, 46, 0x60);
    vdp.advance_command(1);

    REQUIRE((vdp.status_value(2) & 0x10U) != 0U);
    REQUIRE(vdp.status_value(8) == 0x01);
}

TEST_CASE("LINE draws and HMMC streams through the data port while TR is asserted", "[video]") {
    V9938 vdp;

    write_command_word(vdp, 36, 0x0000);
    write_command_word(vdp, 38, 0x0000);
    write_command_word(vdp, 40, 0x0003);
    write_command_word(vdp, 42, 0x0000);
    write_register(vdp, 44, 0x05);
    write_register(vdp, 45, 0x00);
    write_register(vdp, 46, 0x70);
    vdp.advance_command(1);

    REQUIRE((vdp.vram()[0x0000] >> 4) == 0x05);
    REQUIRE((vdp.vram()[0x0000] & 0x0FU) == 0x05);
    REQUIRE((vdp.vram()[0x0001] >> 4) == 0x05);

    write_command_word(vdp, 36, 0x0000);
    write_command_word(vdp, 38, 0x0005);
    write_command_word(vdp, 40, 0x0002);
    write_command_word(vdp, 42, 0x0001);
    write_register(vdp, 45, 0x00);
    write_register(vdp, 46, 0xF0);

    REQUIRE((vdp.status_value(2) & 0x80U) != 0U);
    vdp.write_data(0xDE);
    REQUIRE((vdp.status_value(2) & 0x80U) != 0U);
    vdp.write_data(0xAD);
    REQUIRE((vdp.status_value(2) & 0x01U) == 0U);
    REQUIRE(vdp.vram()[5 * V9938::bytes_per_scanline] == 0xDE);
    REQUIRE(vdp.vram()[5 * V9938::bytes_per_scanline + 1] == 0xAD);
}

TEST_CASE("LMMV LMMM YMMM and LMMC operate on the covered Graphic 4 command paths", "[video]") {
    V9938 vdp;

    vdp.poke_vram(10 * V9938::bytes_per_scanline, 0x12);
    vdp.poke_vram(10 * V9938::bytes_per_scanline + 1, 0x34);

    write_command_word(vdp, 36, 0x0004);
    write_command_word(vdp, 38, 0x000A);
    write_command_word(vdp, 40, 0x0002);
    write_command_word(vdp, 42, 0x0001);
    write_register(vdp, 44, 0x03);
    write_register(vdp, 46, 0x80);
    vdp.advance_command(1);
    REQUIRE(vdp.background_line_buffer()[0] == 0x00); // keep test using VRAM/state only
    REQUIRE((vdp.vram()[10 * V9938::bytes_per_scanline + 2] >> 4) == 0x03);
    REQUIRE((vdp.vram()[10 * V9938::bytes_per_scanline + 2] & 0x0FU) == 0x03);

    write_command_word(vdp, 32, 0x0000);
    write_command_word(vdp, 34, 0x000A);
    write_command_word(vdp, 36, 0x0000);
    write_command_word(vdp, 38, 0x000B);
    write_command_word(vdp, 40, 0x0004);
    write_command_word(vdp, 42, 0x0001);
    write_register(vdp, 46, 0x90);
    vdp.advance_command(1);
    REQUIRE(vdp.vram()[11 * V9938::bytes_per_scanline] == 0x12);
    REQUIRE(vdp.vram()[11 * V9938::bytes_per_scanline + 1] == 0x34);

    write_command_word(vdp, 32, 0x0000);
    write_command_word(vdp, 34, 0x000B);
    write_command_word(vdp, 36, 0x0000);
    write_command_word(vdp, 38, 0x000C);
    write_command_word(vdp, 42, 0x0001);
    write_register(vdp, 46, 0xE0);
    vdp.advance_command(1);
    REQUIRE(vdp.vram()[12 * V9938::bytes_per_scanline] == 0x12);
    REQUIRE(vdp.vram()[12 * V9938::bytes_per_scanline + 1] == 0x34);

    write_command_word(vdp, 36, 0x0006);
    write_command_word(vdp, 38, 0x000D);
    write_command_word(vdp, 40, 0x0001);
    write_command_word(vdp, 42, 0x0001);
    write_register(vdp, 46, 0xB0);
    REQUIRE((vdp.status_value(2) & 0x80U) != 0U);
    vdp.write_data(0x0E);
    REQUIRE((vdp.status_value(2) & 0x01U) == 0U);
    REQUIRE((vdp.vram()[13 * V9938::bytes_per_scanline + 3] >> 4) == 0x0E);
}

TEST_CASE("VDP-A color 0 transparency reveals VDP-B while nonzero VDP-A pixels override", "[video]") {
    V9938 vdp_a;
    V9938 vdp_b;

    set_graphic4_mode(vdp_a);
    set_graphic4_mode(vdp_b);
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

    set_graphic4_mode(vdp_a);
    set_graphic4_mode(vdp_b);
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

    set_graphic4_mode(vdp);
    for (std::uint8_t sprite_index = 0; sprite_index < 9; ++sprite_index) {
        write_mode2_sprite(vdp, sprite_index, 10, static_cast<std::uint8_t>(sprite_index * 4), sprite_index, 0x03);
        write_sprite_pattern_rows(vdp, sprite_index, 0xC0);
    }
    terminate_sprite_list(vdp, 9);

    vdp.tick_scanline(10);
    REQUIRE((vdp.status_value(0) & 0x40U) != 0U);

    V9938 collision_vdp;
    set_graphic4_mode(collision_vdp);
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

TEST_CASE("Mode 2 sprites support 16x16 size and magnification", "[video]") {
    V9938 size_vdp;
    set_graphic4_mode(size_vdp);
    write_register(
        size_vdp,
        1,
        static_cast<std::uint8_t>(V9938::graphic_mode_r1 | 0x40U | 0x02U)
    );
    write_mode2_sprite(size_vdp, 0, 30, 40, 0x03, 0x07);
    write_16x16_sprite_pattern_rows(size_vdp, 0x03, 0xFF, 0xFF);
    terminate_sprite_list(size_vdp, 1);

    size_vdp.tick_scanline(45);
    REQUIRE(size_vdp.sprite_line_buffer()[39] == V9938::transparent_sprite_pixel);
    REQUIRE(size_vdp.sprite_line_buffer()[40] == 0x07);
    REQUIRE(size_vdp.sprite_line_buffer()[55] == 0x07);
    REQUIRE(size_vdp.sprite_line_buffer()[56] == V9938::transparent_sprite_pixel);

    V9938 magnified_vdp;
    set_graphic4_mode(magnified_vdp);
    write_register(
        magnified_vdp,
        1,
        static_cast<std::uint8_t>(V9938::graphic_mode_r1 | 0x40U | 0x03U)
    );
    write_mode2_sprite(magnified_vdp, 0, 50, 60, 0x00, 0x06);
    write_sprite_pattern_rows(magnified_vdp, 0x00, 0x80);
    terminate_sprite_list(magnified_vdp, 1);

    magnified_vdp.tick_scanline(51);
    REQUIRE(magnified_vdp.sprite_line_buffer()[60] == 0x06);
    REQUIRE(magnified_vdp.sprite_line_buffer()[61] == 0x06);
    REQUIRE(magnified_vdp.sprite_line_buffer()[62] == V9938::transparent_sprite_pixel);

    magnified_vdp.tick_scanline(52);
    REQUIRE(magnified_vdp.sprite_line_buffer()[60] == 0x06);
    REQUIRE(magnified_vdp.sprite_line_buffer()[61] == 0x06);
}

TEST_CASE("Mode 2 sprites use register-relative table bases", "[video]") {
    V9938 vdp;
    set_graphic4_mode(vdp);

    constexpr std::uint16_t relocated_attribute_base = 0x5200;
    constexpr std::uint16_t relocated_pattern_base = 0x4800;
    write_register(vdp, 5, static_cast<std::uint8_t>(relocated_attribute_base >> 7U));
    write_register(vdp, 6, static_cast<std::uint8_t>(relocated_pattern_base >> 11U));
    write_register(vdp, 11, static_cast<std::uint8_t>((relocated_attribute_base >> 15U) & 0x03U));

    vdp.poke_vram(relocated_attribute_base + 0, 70);
    vdp.poke_vram(relocated_attribute_base + 1, 80);
    vdp.poke_vram(relocated_attribute_base + 2, 0x05);
    vdp.poke_vram(relocated_attribute_base + 3, 0x00);
    for (int byte = 4; byte < 8; ++byte) {
        vdp.poke_vram(static_cast<std::uint16_t>(relocated_attribute_base + byte), 0x00);
    }

    const auto relocated_color_base = static_cast<std::uint16_t>(relocated_attribute_base - 0x0200U);
    for (int row = 0; row < 16; ++row) {
        vdp.poke_vram(static_cast<std::uint16_t>(relocated_color_base + row), 0x09);
    }

    const auto relocated_pattern_address =
        static_cast<std::uint16_t>(relocated_pattern_base + 0x05U * 8U);
    vdp.poke_vram(relocated_pattern_address, 0x80);
    vdp.poke_vram(V9938::graphic4_sprite_pattern_base, 0x00);
    terminate_sprite_list(vdp, 1);
    vdp.poke_vram(static_cast<std::uint16_t>(relocated_attribute_base + 8U), 0xD0);

    vdp.tick_scanline(70);
    REQUIRE(vdp.sprite_line_buffer()[80] == 0x09);
    REQUIRE(vdp.sprite_line_buffer()[81] == V9938::transparent_sprite_pixel);
}

TEST_CASE("Reading S#0 and S#1 clears the documented VDP-A interrupt flags", "[video]") {
    V9938 vdp_a;
    V9938 vdp_b;

    set_graphic4_mode(vdp_a);
    set_graphic4_mode(vdp_b);
    write_register(vdp_a, 1, static_cast<std::uint8_t>(V9938::graphic_mode_r1 | 0x20U));
    vdp_a.set_vblank_flag(true);
    REQUIRE(vdp_a.int_pending());
    REQUIRE((vdp_a.read_status() & 0x80U) != 0U);
    REQUIRE(!vdp_a.int_pending());

    write_register(vdp_a, 0, static_cast<std::uint8_t>(V9938::graphic4_mode_r0 | 0x10U));
    write_register(vdp_a, 19, 12);
    write_register(vdp_a, 15, 1);
    vdp_a.tick_scanline(12);
    REQUIRE(vdp_a.int_pending());
    REQUIRE((vdp_a.read_status() & 0x01U) != 0U);
    REQUIRE(!vdp_a.int_pending());

    write_register(vdp_b, 1, static_cast<std::uint8_t>(V9938::graphic_mode_r1 | 0x20U));
    vdp_b.set_vblank_flag(true);
    REQUIRE(vdp_b.int_pending());
}

TEST_CASE("Graphic 3 rendering follows the documented tile and color table layout", "[video]") {
    V9938 vdp;
    set_graphic3_mode(vdp);

    vdp.write_palette(0x01);
    vdp.write_palette(0x70);
    vdp.write_palette(0x00);
    vdp.write_palette(0x02);
    vdp.write_palette(0x07);
    vdp.write_palette(0x00);
    vdp.write_palette(0x03);
    vdp.write_palette(0x00);
    vdp.write_palette(0x07);
    write_register(vdp, 7, 0x03);

    vdp.poke_vram(0x0000, 0x01);
    vdp.poke_vram(static_cast<std::uint16_t>(V9938::graphic3_pattern_base + 0x0008), 0x80);
    vdp.poke_vram(static_cast<std::uint16_t>(V9938::graphic3_color_base + 0x0008), 0x12);
    vdp.poke_vram(0x0100, 0x01);
    vdp.poke_vram(static_cast<std::uint16_t>(V9938::graphic3_pattern_base + 0x0808), 0x80);
    vdp.poke_vram(static_cast<std::uint16_t>(V9938::graphic3_color_base + 0x0808), 0x23);

    vdp.tick_scanline(0);
    REQUIRE(vdp.background_line_buffer()[0] == 0x01);
    REQUIRE(vdp.background_line_buffer()[1] == 0x02);

    vdp.tick_scanline(64);
    REQUIRE(vdp.background_line_buffer()[0] == 0x02);
    REQUIRE(vdp.background_line_buffer()[1] == 0x03);

    vdp.tick_scanline(192);
    REQUIRE(vdp.background_line_buffer()[0] == 0x03);
    REQUIRE(vdp.background_line_buffer()[255] == 0x03);
}

TEST_CASE("Graphic 3 on VDP-A composes over Graphic 4 on VDP-B through color zero transparency", "[video]") {
    V9938 vdp_a;
    V9938 vdp_b;
    set_graphic3_mode(vdp_a);
    set_graphic4_mode(vdp_b);

    vdp_a.write_palette(0x00);
    vdp_a.write_palette(0x00);
    vdp_a.write_palette(0x00);
    vdp_a.write_palette(0x04);
    vdp_a.write_palette(0x70);
    vdp_a.write_palette(0x00);
    write_register(vdp_a, 8, 0x20);
    vdp_a.poke_vram(0x0000, 0x01);
    vdp_a.poke_vram(static_cast<std::uint16_t>(V9938::graphic3_pattern_base + 0x0008), 0x80);
    vdp_a.poke_vram(static_cast<std::uint16_t>(V9938::graphic3_color_base + 0x0008), 0x40);

    vdp_b.write_palette(0x00);
    vdp_b.write_palette(0x00);
    vdp_b.write_palette(0x00);
    vdp_b.write_palette(0x02);
    vdp_b.write_palette(0x07);
    vdp_b.write_palette(0x00);
    write_graphic4_byte(vdp_b, 0, 0, 0x22);

    const auto frame = Compositor::compose_dual_vdp(vdp_a, vdp_b);
    const auto pixel0 = std::array<std::uint8_t, 3>{frame[0], frame[1], frame[2]};
    const auto pixel1 = std::array<std::uint8_t, 3>{frame[3], frame[4], frame[5]};

    REQUIRE(pixel0 == vdp_a.palette_entry_rgb(4));
    REQUIRE(pixel1 == vdp_b.palette_entry_rgb(2));
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

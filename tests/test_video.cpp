#include <catch2/catch_test_macros.hpp>

#include "core/video/compositor.hpp"
#include "core/video/v9938.hpp"
#include "frontend/display.hpp"

#include <filesystem>

namespace {

void write_register(vanguard8::core::video::V9938& vdp, const std::uint8_t reg, const std::uint8_t value) {
    vdp.write_control(value);
    vdp.write_control(static_cast<std::uint8_t>(0x80U | reg));
}

}  // namespace

TEST_CASE("VDP data and palette ports mutate VRAM and palette state", "[video]") {
    vanguard8::core::video::V9938 vdp;

    vdp.write_control(0x00);
    vdp.write_control(0x40);
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
    vanguard8::core::video::V9938 vdp;

    vdp.write_palette(0x01);
    vdp.write_palette(0x75);
    vdp.write_palette(0x06);

    const auto rgb = vdp.palette_entry_rgb(1);
    REQUIRE(rgb[0] == vanguard8::core::video::V9938::expand3to8(7));
    REQUIRE(rgb[1] == vanguard8::core::video::V9938::expand3to8(5));
    REQUIRE(rgb[2] == vanguard8::core::video::V9938::expand3to8(6));
}

TEST_CASE("Graphic 4 rendering follows VRAM addressing across scanlines", "[video]") {
    vanguard8::core::video::V9938 vdp;

    write_register(vdp, 23, 0x00);
    vdp.write_palette(0x01);
    vdp.write_palette(0x70);
    vdp.write_palette(0x00);
    vdp.write_palette(0x02);
    vdp.write_palette(0x07);
    vdp.write_palette(0x00);

    vdp.write_control(0x00);
    vdp.write_control(0x40);
    vdp.write_data(0x12);  // line 0, pixels 0/1
    for (int index = 1; index < vanguard8::core::video::V9938::bytes_per_scanline; ++index) {
        vdp.write_data(0x00);
    }
    vdp.write_data(0x21);  // line 1, pixels 0/1

    vdp.tick_scanline(0);
    REQUIRE(vdp.line_buffer()[0] == 0x01);
    REQUIRE(vdp.line_buffer()[1] == 0x02);

    vdp.tick_scanline(1);
    REQUIRE(vdp.line_buffer()[0] == 0x02);
    REQUIRE(vdp.line_buffer()[1] == 0x01);
}

TEST_CASE("R#23 vertical scroll wraps within the VRAM page", "[video]") {
    vanguard8::core::video::V9938 vdp;

    vdp.write_control(0x00);
    vdp.write_control(0x40);
    for (int y = 0; y < 256; ++y) {
        for (int x = 0; x < vanguard8::core::video::V9938::visible_width; x += 2) {
            const auto color = static_cast<std::uint8_t>(y & 0x0FU);
            vdp.write_data(static_cast<std::uint8_t>((color << 4) | color));
        }
    }

    write_register(vdp, 23, 0x03);
    vdp.tick_scanline(0);
    REQUIRE(vdp.line_buffer()[0] == 0x03);

    write_register(vdp, 23, 0xFF);
    vdp.tick_scanline(1);
    REQUIRE(vdp.line_buffer()[0] == 0x00);
}

TEST_CASE("Headless and display upload paths match for a known frame", "[video]") {
    vanguard8::core::video::V9938 vdp;
    vdp.write_palette(0x00);
    vdp.write_palette(0x00);
    vdp.write_palette(0x00);
    vdp.write_palette(0x01);
    vdp.write_palette(0x70);
    vdp.write_palette(0x00);

    vdp.write_control(0x00);
    vdp.write_control(0x40);
    for (int y = 0; y < vanguard8::core::video::V9938::visible_height; ++y) {
        for (int x = 0; x < vanguard8::core::video::V9938::visible_width; x += 2) {
            vdp.write_data(0x11);
        }
    }

    const auto frame = vanguard8::core::video::Compositor::compose_single_vdp(vdp);
    vanguard8::frontend::Display display;
    display.upload_frame(frame);

    REQUIRE(display.uploaded_frame() == frame);

    const auto ppm = display.dump_ppm_string();
    REQUIRE(ppm.rfind("P6\n256 212\n255\n", 0) == 0);

    const auto output_path =
        std::filesystem::temp_directory_path() / "vanguard8-m04-frame-dump.ppm";
    display.dump_ppm_file(output_path);
    REQUIRE(std::filesystem::exists(output_path));
}

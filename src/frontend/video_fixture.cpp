#include "frontend/video_fixture.hpp"

namespace vanguard8::frontend {

namespace {

void write_register(
    core::video::V9938& vdp,
    const std::uint8_t reg,
    const std::uint8_t value
) {
    vdp.write_control(value);
    vdp.write_control(static_cast<std::uint8_t>(0x80U | reg));
}

void write_mode2_sprite(
    core::video::V9938& vdp,
    const std::uint8_t sprite_index,
    const std::uint8_t y,
    const std::uint8_t x,
    const std::uint8_t pattern_number,
    const std::uint8_t color_index
) {
    const auto attribute_base = static_cast<std::uint16_t>(
        core::video::V9938::graphic4_sprite_attribute_base + sprite_index * 8U
    );
    vdp.poke_vram(attribute_base + 0, y);
    vdp.poke_vram(attribute_base + 1, x);
    vdp.poke_vram(attribute_base + 2, pattern_number);
    vdp.poke_vram(attribute_base + 3, 0x00);
    vdp.poke_vram(attribute_base + 4, 0x00);
    vdp.poke_vram(attribute_base + 5, 0x00);
    vdp.poke_vram(attribute_base + 6, 0x00);
    vdp.poke_vram(attribute_base + 7, 0x00);

    const auto color_base = static_cast<std::uint16_t>(
        core::video::V9938::graphic4_sprite_color_base + sprite_index * 16U
    );
    for (int row = 0; row < 16; ++row) {
        vdp.poke_vram(static_cast<std::uint16_t>(color_base + row), color_index);
    }

    const auto pattern_base = static_cast<std::uint16_t>(
        core::video::V9938::graphic4_sprite_pattern_base + pattern_number * 8U
    );
    for (int row = 0; row < 8; ++row) {
        vdp.poke_vram(static_cast<std::uint16_t>(pattern_base + row), 0x3CU);
    }
}

}  // namespace

void build_video_fixture_frame(core::video::V9938& vdp) {
    vdp.reset();

    vdp.write_palette(0x00);
    vdp.write_palette(0x00);
    vdp.write_palette(0x00);
    vdp.write_palette(0x01);
    vdp.write_palette(0x70);
    vdp.write_palette(0x00);
    vdp.write_palette(0x02);
    vdp.write_palette(0x07);
    vdp.write_palette(0x00);
    vdp.write_palette(0x03);
    vdp.write_palette(0x00);
    vdp.write_palette(0x07);

    vdp.write_control(0x00);
    vdp.write_control(0x40);
    for (int y = 0; y < 256; ++y) {
        for (int x = 0; x < core::video::V9938::visible_width; x += 2) {
            const auto high = static_cast<std::uint8_t>(((y / 32) + (x / 64)) % 4);
            const auto low = static_cast<std::uint8_t>(((y / 8) + 1) % 4);
            vdp.write_data(static_cast<std::uint8_t>((high << 4) | low));
        }
    }

    vdp.write_control(0x03);
    vdp.write_control(0x97);  // R#23 = 3
}

void build_dual_vdp_fixture_frame(core::video::V9938& vdp_a, core::video::V9938& vdp_b) {
    build_video_fixture_frame(vdp_b);
    build_video_fixture_frame(vdp_a);

    write_register(vdp_a, 8, 0x20);

    vdp_a.write_palette(0x70);
    vdp_a.write_palette(0x07);

    vdp_a.write_control(0x00);
    vdp_a.write_control(0x40);
    for (int y = 0; y < core::video::V9938::visible_height; ++y) {
        for (int x = 0; x < core::video::V9938::visible_width; x += 2) {
            const auto high = static_cast<std::uint8_t>((x / 32) % 2 == 0 ? 0x00 : 0x04);
            const auto low = static_cast<std::uint8_t>((y / 16) % 2 == 0 ? 0x00 : 0x04);
            vdp_a.write_data(static_cast<std::uint8_t>((high << 4) | low));
        }
    }

    write_mode2_sprite(vdp_a, 0, 40, 48, 0, 0x04);

    vdp_a.poke_vram(static_cast<std::uint16_t>(core::video::V9938::graphic4_sprite_attribute_base + 8U), 0xD0);
}

}  // namespace vanguard8::frontend

#include "frontend/video_fixture.hpp"

namespace vanguard8::frontend {

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

}  // namespace vanguard8::frontend

#pragma once

#include "core/video/v9938.hpp"

#include <array>

namespace vanguard8::core::video {

struct LayerMask {
    bool hide_vdp_a_bg = false;
    bool hide_vdp_a_sprites = false;
    bool hide_vdp_b_bg = false;
    bool hide_vdp_b_sprites = false;
};

class Compositor {
  public:
    [[nodiscard]] static auto compose_single_vdp(const V9938& vdp) -> V9938::Framebuffer {
        V9938& mutable_vdp = const_cast<V9938&>(vdp);
        V9938::Framebuffer frame(
            static_cast<std::size_t>(V9938::visible_width * V9938::visible_height * 3),
            0x00
        );

        for (int line = 0; line < V9938::visible_height; ++line) {
            mutable_vdp.tick_scanline(line);
            for (int x = 0; x < V9938::visible_width; ++x) {
                const auto palette_index = mutable_vdp.line_buffer()[x];
                const auto rgb = mutable_vdp.palette_entry_rgb(palette_index);
                const auto base =
                    static_cast<std::size_t>((line * V9938::visible_width + x) * 3);
                frame[base + 0] = rgb[0];
                frame[base + 1] = rgb[1];
                frame[base + 2] = rgb[2];
            }
        }

        return frame;
    }

    [[nodiscard]] static auto compose_dual_vdp(
        const V9938& vdp_a,
        const V9938& vdp_b,
        const LayerMask& layer_mask = {}
    ) -> V9938::Framebuffer {
        V9938& mutable_vdp_a = const_cast<V9938&>(vdp_a);
        V9938& mutable_vdp_b = const_cast<V9938&>(vdp_b);
        V9938::Framebuffer frame(
            static_cast<std::size_t>(V9938::visible_width * V9938::visible_height * 3),
            0x00
        );

        for (int line = 0; line < V9938::visible_height; ++line) {
            mutable_vdp_a.tick_scanline(line);
            mutable_vdp_b.tick_scanline(line);

            for (int x = 0; x < V9938::visible_width; ++x) {
                const auto final_a = resolve_pixel(
                    mutable_vdp_a,
                    x,
                    layer_mask.hide_vdp_a_bg,
                    layer_mask.hide_vdp_a_sprites
                );
                const auto final_b = resolve_pixel(
                    mutable_vdp_b,
                    x,
                    layer_mask.hide_vdp_b_bg,
                    layer_mask.hide_vdp_b_sprites
                );

                const auto rgb =
                    final_a.opaque ? mutable_vdp_a.palette_entry_rgb(final_a.palette_index)
                                   : mutable_vdp_b.palette_entry_rgb(final_b.palette_index);
                const auto base =
                    static_cast<std::size_t>((line * V9938::visible_width + x) * 3);
                frame[base + 0] = rgb[0];
                frame[base + 1] = rgb[1];
                frame[base + 2] = rgb[2];
            }
        }

        return frame;
    }

  private:
    struct ResolvedPixel {
        std::uint8_t palette_index = 0;
        bool opaque = false;
    };

    [[nodiscard]] static auto resolve_pixel(
        const V9938& vdp,
        int x,
        bool hide_background,
        bool hide_sprites
    ) -> ResolvedPixel {
        if (!hide_sprites) {
            const auto sprite = vdp.sprite_line_buffer()[x];
            if (sprite != V9938::transparent_sprite_pixel) {
                return {.palette_index = sprite, .opaque = true};
            }
        }

        if (hide_background) {
            return {};
        }

        const auto background = vdp.background_line_buffer()[x];
        return {
            .palette_index = background,
            .opaque = !vdp.color_zero_transparent() || background != 0,
        };
    }
};

}  // namespace vanguard8::core::video

#include "core/video/v9938.hpp"

#include <algorithm>

namespace vanguard8::core::video {

V9938::V9938() { reset(); }

void V9938::reset() {
    vram_.fill(0x00);
    reg_.fill(0x00);
    status_.fill(0x00);
    for (auto& entry : palette_) {
        entry = {0x00, 0x00};
    }
    vram_addr_ = 0;
    control_latch_ = 0;
    addr_latch_full_ = false;
    indirect_register_ = 0x20;
    status_reg_select_ = 0;
    palette_index_ = 0;
    palette_phase_ = 0;
    background_line_buffer_.fill(0x00);
    sprite_line_buffer_.fill(transparent_sprite_pixel);
    line_buffer_.fill(0x00);
    reg_[9] = 0x80;  // 212-line operation recommended by the spec baseline.
}

auto V9938::read_data() -> std::uint8_t {
    const auto value = vram_[vram_addr_];
    vram_addr_ = static_cast<std::uint16_t>(vram_addr_ + 1U);
    return value;
}

void V9938::write_data(const std::uint8_t value) {
    vram_[vram_addr_] = value;
    vram_addr_ = static_cast<std::uint16_t>(vram_addr_ + 1U);
}

auto V9938::read_status() -> std::uint8_t {
    addr_latch_full_ = false;

    const auto index = static_cast<std::uint8_t>(status_reg_select_ % status_.size());
    const auto value = status_[index];
    if (index == 0) {
        status_[0] = static_cast<std::uint8_t>(status_[0] & 0x40U);
    } else if (index == 1) {
        status_[1] = static_cast<std::uint8_t>(status_[1] & ~0x01U);
    }
    update_int_state();
    return value;
}

void V9938::write_control(const std::uint8_t value) {
    if (!addr_latch_full_) {
        control_latch_ = value;
        addr_latch_full_ = true;
        return;
    }

    addr_latch_full_ = false;

    if ((value & 0x80U) != 0U) {
        write_register_value(static_cast<std::uint8_t>(value & 0x3FU), control_latch_);
        return;
    }

    // Keep the milestone implementation narrow: treat the two-byte control sequence
    // as a 14-bit address load, which is sufficient for the active Graphic 4 writes
    // covered by the tests and fixture frame paths.
    vram_addr_ =
        static_cast<std::uint16_t>(control_latch_ | (static_cast<std::uint16_t>(value & 0x3FU) << 8));
}

void V9938::write_palette(const std::uint8_t value) {
    if (palette_phase_ == 0) {
        palette_index_ = static_cast<std::uint8_t>(value & 0x0FU);
        palette_phase_ = 1;
        return;
    }

    if (palette_phase_ == 1) {
        palette_[palette_index_][0] = value;
        palette_phase_ = 2;
        return;
    }

    palette_[palette_index_][1] = value;
    palette_index_ = static_cast<std::uint8_t>((palette_index_ + 1U) & 0x0FU);
    palette_phase_ = 1;
}

void V9938::write_register(const std::uint8_t value) {
    write_register_value(indirect_register_, value);
    indirect_register_ = static_cast<std::uint8_t>(indirect_register_ + 1U);
}

void V9938::poke_vram(const std::uint16_t address, const std::uint8_t value) { vram_[address] = value; }

void V9938::tick_scanline(const int line) {
    status_[0] = static_cast<std::uint8_t>(status_[0] & ~0x40U);
    render_graphic4_background_scanline(line);
    render_mode2_sprites_for_scanline(line);

    for (int x = 0; x < visible_width; ++x) {
        line_buffer_[x] =
            sprite_line_buffer_[x] == transparent_sprite_pixel ? background_line_buffer_[x]
                                                               : sprite_line_buffer_[x];
    }

    if (line == reg_[19]) {
        status_[1] = static_cast<std::uint8_t>(status_[1] | 0x01U);
    }
    update_int_state();
}

auto V9938::render_graphic4_frame() -> Framebuffer {
    Framebuffer frame(static_cast<std::size_t>(visible_width * visible_height * 3), 0x00);

    for (int line = 0; line < visible_height; ++line) {
        tick_scanline(line);
        for (int x = 0; x < visible_width; ++x) {
            const auto rgb = current_palette_rgb(line_buffer_[x]);
            const auto base = static_cast<std::size_t>((line * visible_width + x) * 3);
            frame[base + 0] = rgb[0];
            frame[base + 1] = rgb[1];
            frame[base + 2] = rgb[2];
        }
    }

    return frame;
}

void V9938::set_vblank_flag(const bool asserted) {
    if (asserted) {
        status_[0] = static_cast<std::uint8_t>(status_[0] | 0x80U);
    } else {
        status_[0] = static_cast<std::uint8_t>(status_[0] & ~0x80U);
    }
    update_int_state();
}

auto V9938::int_pending() const -> bool {
    return vblank_irq_pending() || hblank_irq_pending();
}

auto V9938::vblank_irq_pending() const -> bool {
    return (reg_[1] & 0x20U) != 0U && (status_[0] & 0x80U) != 0U;
}

auto V9938::hblank_irq_pending() const -> bool {
    return (reg_[0] & 0x10U) != 0U && (status_[1] & 0x01U) != 0U;
}

auto V9938::register_value(const std::uint8_t index) const -> std::uint8_t { return reg_[index]; }

auto V9938::status_value(const std::uint8_t index) const -> std::uint8_t { return status_[index]; }

auto V9938::palette_entry_raw(const std::uint8_t index) const -> std::array<std::uint8_t, 2> {
    return palette_[index & 0x0FU];
}

auto V9938::palette_entry_rgb(const std::uint8_t index) const -> std::array<std::uint8_t, 3> {
    return current_palette_rgb(index & 0x0FU);
}

auto V9938::line_buffer() const -> const LineBuffer& { return line_buffer_; }

auto V9938::background_line_buffer() const -> const LineBuffer& { return background_line_buffer_; }

auto V9938::sprite_line_buffer() const -> const LineBuffer& { return sprite_line_buffer_; }

auto V9938::vram() const -> const std::array<std::uint8_t, vram_size>& { return vram_; }

auto V9938::color_zero_transparent() const -> bool { return (reg_[8] & 0x20U) != 0U; }

auto V9938::expand3to8(const std::uint8_t value) -> std::uint8_t {
    const auto clipped = static_cast<std::uint8_t>(value & 0x07U);
    return static_cast<std::uint8_t>((clipped << 5) | (clipped << 2) | (clipped >> 1));
}

void V9938::write_register_value(const std::uint8_t index, const std::uint8_t value) {
    reg_[index] = value;
    if (index == 15) {
        status_reg_select_ = static_cast<std::uint8_t>(value % status_.size());
    }
    update_int_state();
}

auto V9938::graphic4_byte_address(const int line, const int x) const -> std::uint16_t {
    const auto scrolled_line =
        static_cast<std::uint16_t>((static_cast<unsigned>(line) + vertical_scroll()) & 0xFFU);
    return static_cast<std::uint16_t>(scrolled_line * bytes_per_scanline + (x / 2));
}

auto V9938::vertical_scroll() const -> std::uint8_t { return reg_[23]; }

void V9938::update_int_state() {}

void V9938::render_graphic4_background_scanline(const int line) {
    for (int x = 0; x < visible_width; ++x) {
        const auto address = graphic4_byte_address(line, x);
        const auto byte = vram_[address];
        background_line_buffer_[x] =
            static_cast<std::uint8_t>((x % 2) == 0 ? (byte >> 4) : (byte & 0x0FU));
    }
}

void V9938::render_mode2_sprites_for_scanline(const int line) {
    sprite_line_buffer_.fill(transparent_sprite_pixel);

    std::array<int, visible_width> first_owner{};
    first_owner.fill(-1);

    int visible_sprite_count = 0;
    bool collision_recorded = false;

    for (std::uint8_t sprite_index = 0; sprite_index < 32; ++sprite_index) {
        const auto sat_base = static_cast<std::uint16_t>(graphic4_sprite_attribute_base + sprite_index * 8U);
        const auto y = vram_[sat_base + 0];
        if (y == 0xD0U) {
            break;
        }

        const auto x = vram_[sat_base + 1];
        const auto pattern_number = vram_[sat_base + 2];
        const auto color_flags = vram_[sat_base + 3];
        const auto early_clock = (color_flags & 0x80U) != 0U;
        const auto sprite_left = early_clock ? static_cast<int>(x) - 32 : static_cast<int>(x);
        const auto sprite_top = static_cast<int>(y);
        constexpr int sprite_size = 8;
        const auto sprite_row = line - sprite_top;

        if (sprite_row < 0 || sprite_row >= sprite_size) {
            continue;
        }

        ++visible_sprite_count;
        if (visible_sprite_count > 8) {
            status_[0] = static_cast<std::uint8_t>(status_[0] | 0x40U);
            continue;
        }

        const auto color = sprite_color_for_line(sprite_index, sprite_row);
        if (!color.has_value()) {
            continue;
        }

        const auto pattern = sprite_pattern_byte(pattern_number, sprite_row);
        for (int bit = 0; bit < 8; ++bit) {
            if ((pattern & (0x80U >> bit)) == 0U) {
                continue;
            }

            const auto pixel_x = sprite_left + bit;
            if (pixel_x < 0 || pixel_x >= visible_width) {
                continue;
            }

            if (first_owner[pixel_x] >= 0 && !collision_recorded) {
                status_[0] = static_cast<std::uint8_t>(status_[0] | 0x20U);
                status_[3] = static_cast<std::uint8_t>(pixel_x & 0xFF);
                status_[4] = static_cast<std::uint8_t>((pixel_x >> 8) & 0x01);
                status_[5] = static_cast<std::uint8_t>(line & 0xFF);
                status_[6] = static_cast<std::uint8_t>((line >> 8) & 0x01);
                collision_recorded = true;
            }

            if (first_owner[pixel_x] >= 0) {
                continue;
            }

            first_owner[pixel_x] = sprite_index;
            sprite_line_buffer_[pixel_x] = *color;
        }
    }
}

auto V9938::sprite_color_for_line(const std::uint8_t sprite_index, const int row) const
    -> std::optional<std::uint8_t> {
    const auto color_address =
        static_cast<std::uint16_t>(graphic4_sprite_color_base + sprite_index * 16U + row);
    const auto color_entry = vram_[color_address];
    if ((color_entry & 0x40U) != 0U) {
        return std::nullopt;
    }
    return static_cast<std::uint8_t>(color_entry & 0x0FU);
}

auto V9938::sprite_pattern_byte(const std::uint8_t pattern_number, const int row) const
    -> std::uint8_t {
    // TODO(spec): Milestone 5 locks the covered sprite tests to the recommended
    // Graphic 4 VRAM layout and 8x8 Mode-2 pattern fetch path. General base-register
    // relocation and larger sprite geometries should follow only after the repo docs
    // carry the needed alignment and addressing detail.
    const auto pattern_address =
        static_cast<std::uint16_t>(graphic4_sprite_pattern_base + pattern_number * 8U + row);
    return vram_[pattern_address];
}

auto V9938::current_palette_rgb(const std::uint8_t index) const -> std::array<std::uint8_t, 3> {
    const auto& entry = palette_[index & 0x0FU];
    return {
        expand3to8(static_cast<std::uint8_t>((entry[0] >> 4) & 0x07U)),
        expand3to8(static_cast<std::uint8_t>(entry[0] & 0x07U)),
        expand3to8(static_cast<std::uint8_t>(entry[1] & 0x07U)),
    };
}

}  // namespace vanguard8::core::video

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
    return status_[status_reg_select_ % status_.size()];
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

void V9938::tick_scanline(const int line) {
    for (int x = 0; x < visible_width; ++x) {
        const auto address = graphic4_byte_address(line, x);
        const auto byte = vram_[address];
        line_buffer_[x] = static_cast<std::uint8_t>((x % 2) == 0 ? (byte >> 4) : (byte & 0x0FU));
    }
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

auto V9938::register_value(const std::uint8_t index) const -> std::uint8_t { return reg_[index]; }

auto V9938::palette_entry_raw(const std::uint8_t index) const -> std::array<std::uint8_t, 2> {
    return palette_[index & 0x0FU];
}

auto V9938::palette_entry_rgb(const std::uint8_t index) const -> std::array<std::uint8_t, 3> {
    return current_palette_rgb(index & 0x0FU);
}

auto V9938::line_buffer() const -> const LineBuffer& { return line_buffer_; }

auto V9938::vram() const -> const std::array<std::uint8_t, vram_size>& { return vram_; }

auto V9938::expand3to8(const std::uint8_t value) -> std::uint8_t {
    const auto clipped = static_cast<std::uint8_t>(value & 0x07U);
    return static_cast<std::uint8_t>((clipped << 5) | (clipped << 2) | (clipped >> 1));
}

void V9938::write_register_value(const std::uint8_t index, const std::uint8_t value) {
    reg_[index] = value;
    if (index == 15) {
        status_reg_select_ = static_cast<std::uint8_t>(value % status_.size());
    }
}

auto V9938::graphic4_byte_address(const int line, const int x) const -> std::uint16_t {
    const auto scrolled_line =
        static_cast<std::uint16_t>((static_cast<unsigned>(line) + vertical_scroll()) & 0xFFU);
    return static_cast<std::uint16_t>(scrolled_line * bytes_per_scanline + (x / 2));
}

auto V9938::vertical_scroll() const -> std::uint8_t { return reg_[23]; }

auto V9938::current_palette_rgb(const std::uint8_t index) const -> std::array<std::uint8_t, 3> {
    const auto& entry = palette_[index & 0x0FU];
    return {
        expand3to8(static_cast<std::uint8_t>((entry[0] >> 4) & 0x07U)),
        expand3to8(static_cast<std::uint8_t>(entry[0] & 0x07U)),
        expand3to8(static_cast<std::uint8_t>(entry[1] & 0x07U)),
    };
}

}  // namespace vanguard8::core::video

#include "core/video/v9938.hpp"

#include <algorithm>

namespace vanguard8::core::video {

namespace {

constexpr std::uint8_t status_ce = 0x01;
constexpr std::uint8_t status_bd = 0x10;
constexpr std::uint8_t status_tr = 0x80;
constexpr std::uint8_t arg_maj_bit = 0x01;
constexpr std::uint8_t arg_eq_bit = 0x02;
constexpr std::uint8_t arg_dix_bit = 0x04;
constexpr std::uint8_t arg_diy_bit = 0x08;

}  // namespace

V9938::V9938() { reset(); }

void V9938::reset() {
    vram_.fill(0x00);
    reg_.fill(0x00);
    status_.fill(0x00);
    for (auto& entry : palette_) {
        entry = {0x00, 0x00};
    }
    vram_addr_ = 0;
    read_ahead_latch_ = 0;
    control_latch_ = 0;
    addr_latch_full_ = false;
    indirect_register_ = 0x20;
    status_reg_select_ = 0;
    palette_index_ = 0;
    palette_phase_ = 0;
    command_ = {};
    background_line_buffer_.fill(0x00);
    sprite_line_buffer_.fill(transparent_sprite_pixel);
    line_buffer_.fill(0x00);
    reg_[0] = graphic4_mode_r0;
    reg_[1] = graphic_mode_r1;
    reg_[5] = static_cast<std::uint8_t>(graphic4_sprite_attribute_base >> 7U);
    reg_[6] = static_cast<std::uint8_t>(graphic4_sprite_pattern_base >> 11U);
    reg_[9] = 0x80;  // 212-line operation recommended by the spec baseline.
    reg_[11] = static_cast<std::uint8_t>((graphic4_sprite_attribute_base >> 15U) & 0x03U);
}

auto V9938::read_data() -> std::uint8_t {
    const auto value = read_ahead_latch_;
    read_ahead_latch_ = vram_[vram_addr_];
    increment_vram_addr();
    return value;
}

void V9938::write_data(const std::uint8_t value) {
    if (command_.active && command_.transfer_ready) {
        stream_command_value(value);
        return;
    }

    vram_[vram_addr_] = value;
    increment_vram_addr();
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

    const auto address = cpu_vram_address_from_latch(value);
    if ((value & 0x40U) == 0U) {
        // VRAM reads are one access behind the address latch. Loading a read address
        // changes the source address for the next read-ahead refill but preserves the
        // previously buffered byte, so the first read after an address load is the
        // required dummy read.
        set_vram_addr(address);
        return;
    }

    set_vram_addr(address);
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

void V9938::advance_command(const std::uint64_t master_cycles) {
    if (!command_.active) {
        return;
    }

    if (master_cycles >= command_.cycles_remaining) {
        finish_command();
        return;
    }

    command_.cycles_remaining -= master_cycles;
}

void V9938::tick_scanline(const int line) {
    const auto mode = current_display_mode();
    const auto width = current_visible_width();
    status_[0] = static_cast<std::uint8_t>(status_[0] & ~0x40U);
    if (!display_enabled()) {
        background_line_buffer_.fill(backdrop_color());
        sprite_line_buffer_.fill(transparent_sprite_pixel);
        line_buffer_.fill(backdrop_color());
    } else {
        switch (mode) {
        case DisplayMode::graphic1:
            render_graphic1_background_scanline(line);
            break;
        case DisplayMode::graphic2:
            render_graphic2_background_scanline(line);
            break;
        case DisplayMode::graphic3:
            render_graphic3_background_scanline(line);
            break;
        case DisplayMode::graphic4:
            render_graphic4_background_scanline(line);
            break;
        case DisplayMode::graphic6:
            render_graphic6_background_scanline(line);
            break;
        case DisplayMode::unsupported:
            background_line_buffer_.fill(backdrop_color());
            break;
        }
        switch (mode) {
        case DisplayMode::graphic1:
        case DisplayMode::graphic2:
            render_mode1_sprites_for_scanline(line);
            break;
        case DisplayMode::graphic3:
        case DisplayMode::graphic4:
            render_mode2_sprites_for_scanline(line);
            break;
        case DisplayMode::graphic6:
        case DisplayMode::unsupported:
            sprite_line_buffer_.fill(transparent_sprite_pixel);
            break;
        }

        for (int x = 0; x < width; ++x) {
            line_buffer_[x] =
                sprite_line_buffer_[x] == transparent_sprite_pixel ? background_line_buffer_[x]
                                                                   : sprite_line_buffer_[x];
        }
        for (int x = width; x < max_visible_width; ++x) {
            line_buffer_[x] = backdrop_color();
        }
    }

    if (line == reg_[19]) {
        status_[1] = static_cast<std::uint8_t>(status_[1] | 0x01U);
    }
    update_int_state();
}

auto V9938::render_graphic4_frame() -> Framebuffer {
    const auto width = current_visible_width();
    Framebuffer frame(static_cast<std::size_t>(width * visible_height * 3), 0x00);

    for (int line = 0; line < visible_height; ++line) {
        tick_scanline(line);
        for (int x = 0; x < width; ++x) {
            const auto rgb = current_palette_rgb(line_buffer_[x]);
            const auto base = static_cast<std::size_t>((line * width + x) * 3);
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

auto V9938::state_snapshot() const -> State {
    return State{
        .vram = vram_,
        .reg = reg_,
        .status = status_,
        .palette = palette_,
        .vram_addr = vram_addr_,
        .read_ahead_latch = read_ahead_latch_,
        .control_latch = control_latch_,
        .addr_latch_full = addr_latch_full_,
        .indirect_register = indirect_register_,
        .status_reg_select = status_reg_select_,
        .palette_index = palette_index_,
        .palette_phase = palette_phase_,
        .command =
            CommandStateSnapshot{
                .type = static_cast<std::uint8_t>(command_.type),
                .active = command_.active,
                .transfer_ready = command_.transfer_ready,
                .cycles_remaining = command_.cycles_remaining,
                .stream_x = command_.stream_x,
                .stream_y = command_.stream_y,
                .remaining_x = command_.remaining_x,
                .remaining_y = command_.remaining_y,
            },
        .background_line_buffer = background_line_buffer_,
        .sprite_line_buffer = sprite_line_buffer_,
        .line_buffer = line_buffer_,
    };
}

void V9938::load_state(const State& state) {
    vram_ = state.vram;
    reg_ = state.reg;
    status_ = state.status;
    palette_ = state.palette;
    vram_addr_ = state.vram_addr;
    read_ahead_latch_ = state.read_ahead_latch;
    control_latch_ = state.control_latch;
    addr_latch_full_ = state.addr_latch_full;
    indirect_register_ = state.indirect_register;
    status_reg_select_ = state.status_reg_select;
    palette_index_ = state.palette_index;
    palette_phase_ = state.palette_phase;
    command_ = CommandState{
        .type = static_cast<CommandType>(state.command.type),
        .active = state.command.active,
        .transfer_ready = state.command.transfer_ready,
        .cycles_remaining = state.command.cycles_remaining,
        .stream_x = state.command.stream_x,
        .stream_y = state.command.stream_y,
        .remaining_x = state.command.remaining_x,
        .remaining_y = state.command.remaining_y,
    };
    background_line_buffer_ = state.background_line_buffer;
    sprite_line_buffer_ = state.sprite_line_buffer;
    line_buffer_ = state.line_buffer;
}

auto V9938::expand3to8(const std::uint8_t value) -> std::uint8_t {
    const auto clipped = static_cast<std::uint8_t>(value & 0x07U);
    return static_cast<std::uint8_t>((clipped << 5) | (clipped << 2) | (clipped >> 1));
}

void V9938::write_register_value(const std::uint8_t index, const std::uint8_t value) {
    if (index == 46) {
        begin_command(value);
        update_int_state();
        return;
    }

    reg_[index] = value;
    if (index == 14) {
        set_vram_addr(static_cast<std::uint16_t>(
            (vram_addr_ & 0x3FFFU) | ((static_cast<std::uint16_t>(value) & 0x03U) << 14U)
        ));
    }
    if (index == 15) {
        status_reg_select_ = static_cast<std::uint8_t>(value % status_.size());
    }
    update_int_state();
}

void V9938::set_vram_addr(const std::uint16_t address) {
    vram_addr_ = address;
    reg_[14] = static_cast<std::uint8_t>((reg_[14] & ~0x03U) | ((address >> 14U) & 0x03U));
}

void V9938::increment_vram_addr() { set_vram_addr(static_cast<std::uint16_t>(vram_addr_ + 1U)); }

auto V9938::cpu_vram_address_from_latch(const std::uint8_t high_control) const -> std::uint16_t {
    const auto low_14 = static_cast<std::uint16_t>(
        control_latch_ | (static_cast<std::uint16_t>(high_control & 0x3FU) << 8U)
    );
    return static_cast<std::uint16_t>(
        (low_14 & 0x3FFFU) | ((static_cast<std::uint16_t>(reg_[14]) & 0x03U) << 14U)
    );
}

auto V9938::graphic4_byte_address(const int line, const int x) const -> std::uint16_t {
    const auto scrolled_line =
        static_cast<std::uint16_t>((static_cast<unsigned>(line) + vertical_scroll()) & 0xFFU);
    return static_cast<std::uint16_t>(scrolled_line * bytes_per_scanline + (x / 2));
}

auto V9938::graphic6_byte_address(const int line, const int x) const -> std::uint16_t {
    const auto scrolled_line =
        static_cast<std::uint16_t>((static_cast<unsigned>(line) + vertical_scroll()) & 0xFFU);
    return static_cast<std::uint16_t>(scrolled_line * graphic6_bytes_per_scanline + (x / 2));
}

auto V9938::vertical_scroll() const -> std::uint8_t { return reg_[23]; }

auto V9938::reg16(const std::uint8_t low_index) const -> std::uint16_t {
    return static_cast<std::uint16_t>(
        reg_[low_index] | (static_cast<std::uint16_t>(reg_[low_index + 1] & 0x03U) << 8U)
    );
}

auto V9938::arg_dix() const -> bool { return (reg_[45] & arg_dix_bit) != 0U; }

auto V9938::arg_diy() const -> bool { return (reg_[45] & arg_diy_bit) != 0U; }

auto V9938::arg_eq() const -> bool { return (reg_[45] & arg_eq_bit) != 0U; }

auto V9938::arg_maj() const -> bool { return (reg_[45] & arg_maj_bit) != 0U; }

auto V9938::command_color_mask() const -> std::uint8_t { return 0x0FU; }

auto V9938::command_color_value() const -> std::uint8_t { return static_cast<std::uint8_t>(reg_[44] & 0x0FU); }

auto V9938::graphic4_command_byte_address(const std::uint16_t x, const std::uint16_t y) const -> std::uint16_t {
    return static_cast<std::uint16_t>(((y & 0x00FFU) * bytes_per_scanline) + ((x & 0x00FFU) >> 1U));
}

auto V9938::graphic4_point(const std::uint16_t x, const std::uint16_t y) const -> std::uint8_t {
    const auto byte = vram_[graphic4_command_byte_address(x, y)];
    return static_cast<std::uint8_t>((x & 0x0001U) == 0U ? (byte >> 4) & 0x0FU : byte & 0x0FU);
}

void V9938::graphic4_pset(
    const std::uint16_t x,
    const std::uint16_t y,
    const std::uint8_t source,
    const std::uint8_t op
) {
    const auto address = graphic4_command_byte_address(x, y);
    const auto even_pixel = (x & 0x0001U) == 0U;
    const auto dest = graphic4_point(x, y);
    const auto result = apply_logical_op(source, dest, op);
    auto byte = vram_[address];
    if (even_pixel) {
        byte = static_cast<std::uint8_t>((byte & 0x0FU) | ((result & 0x0FU) << 4U));
    } else {
        byte = static_cast<std::uint8_t>((byte & 0xF0U) | (result & 0x0FU));
    }
    vram_[address] = byte;
}

auto V9938::apply_logical_op(const std::uint8_t source, const std::uint8_t dest, const std::uint8_t op) const
    -> std::uint8_t {
    const auto sc = static_cast<std::uint8_t>(source & command_color_mask());
    const auto dc = static_cast<std::uint8_t>(dest & command_color_mask());
    switch (op & 0x0FU) {
    case 0x00:
        return sc;
    case 0x01:
        return static_cast<std::uint8_t>(sc & dc);
    case 0x02:
        return static_cast<std::uint8_t>(sc | dc);
    case 0x03:
        return static_cast<std::uint8_t>(sc ^ dc);
    case 0x04:
        return static_cast<std::uint8_t>((~sc) & command_color_mask());
    case 0x08:
        return sc == 0U ? dc : sc;
    case 0x09:
        return sc == 0U ? dc : static_cast<std::uint8_t>(sc & dc);
    case 0x0A:
        return sc == 0U ? dc : static_cast<std::uint8_t>(sc | dc);
    case 0x0B:
        return sc == 0U ? dc : static_cast<std::uint8_t>(sc ^ dc);
    case 0x0C:
        return sc == 0U ? dc : static_cast<std::uint8_t>((~sc) & command_color_mask());
    default:
        return sc;
    }
}

void V9938::begin_command(const std::uint8_t value) {
    reg_[46] = value;
    status_[2] = static_cast<std::uint8_t>(status_[2] & ~status_bd);
    command_ = {};

    switch ((value >> 4U) & 0x0FU) {
    case 0x0:
        finish_command();
        return;
    case 0x4:
        command_.type = CommandType::point;
        break;
    case 0x5:
        command_.type = CommandType::pset;
        break;
    case 0x6:
        command_.type = CommandType::srch;
        break;
    case 0x7:
        command_.type = CommandType::line;
        break;
    case 0x8:
        command_.type = CommandType::lmmv;
        break;
    case 0x9:
        command_.type = CommandType::lmmm;
        break;
    case 0xB:
        command_.type = CommandType::lmmc;
        break;
    case 0xC:
        command_.type = CommandType::hmmv;
        break;
    case 0xD:
        command_.type = CommandType::hmmm;
        break;
    case 0xE:
        command_.type = CommandType::ymmm;
        break;
    case 0xF:
        command_.type = CommandType::hmmc;
        break;
    default:
        finish_command();
        return;
    }

    command_.active = true;
    status_[2] = static_cast<std::uint8_t>(status_[2] | status_ce);
    if (command_.type == CommandType::lmmc || command_.type == CommandType::hmmc) {
        begin_cpu_stream_command(command_.type);
        return;
    }

    execute_immediate_command();
    command_.cycles_remaining = estimate_command_cycles(command_.type);
}

void V9938::finish_command() {
    command_ = {};
    status_[2] = static_cast<std::uint8_t>(status_[2] & static_cast<std::uint8_t>(~(status_ce | status_tr)));
}

auto V9938::estimate_command_cycles(const CommandType type) const -> std::uint64_t {
    if (type == CommandType::hmmm) {
        return 64U + (static_cast<std::uint64_t>(reg16(40)) * static_cast<std::uint64_t>(reg16(42)) * 3U);
    }
    return 1U;
}

void V9938::execute_immediate_command() {
    switch (command_.type) {
    case CommandType::point:
        execute_point();
        break;
    case CommandType::pset:
        execute_pset();
        break;
    case CommandType::srch:
        execute_srch();
        break;
    case CommandType::line:
        execute_line();
        break;
    case CommandType::lmmv:
        execute_lmmv();
        break;
    case CommandType::lmmm:
        execute_lmmm();
        break;
    case CommandType::hmmv:
        execute_hmmv();
        break;
    case CommandType::hmmm:
        execute_hmmm();
        break;
    case CommandType::ymmm:
        execute_ymmm();
        break;
    default:
        break;
    }
}

void V9938::execute_point() { status_[7] = graphic4_point(reg16(32), reg16(34)); }

void V9938::execute_pset() { graphic4_pset(reg16(36), reg16(38), command_color_value(), reg_[46] & 0x0FU); }

void V9938::execute_srch() {
    const auto start_x = reg16(32);
    const auto y = reg16(34);
    const auto needle = command_color_value();
    const auto step = arg_dix() ? -1 : 1;
    int x = static_cast<int>(start_x & 0x01FFU);
    int found_x = -1;
    while (x >= 0 && x < 256) {
        const auto match = graphic4_point(static_cast<std::uint16_t>(x), y) == needle;
        if ((arg_eq() && match) || (!arg_eq() && !match)) {
            found_x = x;
            break;
        }
        x += step;
    }

    if (found_x >= 0) {
        status_[2] = static_cast<std::uint8_t>(status_[2] | status_bd);
        status_[8] = static_cast<std::uint8_t>(found_x & 0xFF);
        status_[9] = static_cast<std::uint8_t>((status_[9] & ~0x01U) | ((found_x >> 8) & 0x01U));
    } else {
        status_[2] = static_cast<std::uint8_t>(status_[2] & ~status_bd);
        status_[8] = 0x00;
        status_[9] = static_cast<std::uint8_t>(status_[9] & ~0x01U);
    }
}

void V9938::execute_line() {
    const auto start_x = static_cast<int>(reg16(36));
    const auto start_y = static_cast<int>(reg16(38));
    const auto major = static_cast<int>(reg16(40));
    const auto minor = static_cast<int>(reg16(42));
    const auto step_x = arg_dix() ? -1 : 1;
    const auto step_y = arg_diy() ? -1 : 1;
    const auto dx = arg_maj() ? minor : major;
    const auto dy = arg_maj() ? major : minor;
    auto x = start_x;
    auto y = start_y;
    auto err = (arg_maj() ? dy : dx) / 2;
    const auto count = std::max(dx, dy) + 1;

    for (int index = 0; index < count; ++index) {
        if (x >= 0 && x < 256 && y >= 0 && y < 256) {
            graphic4_pset(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y), command_color_value(), reg_[46] & 0x0FU);
        }

        if (arg_maj()) {
            y += step_y;
            err -= dx;
            if (err < 0) {
                x += step_x;
                err += dy;
            }
        } else {
            x += step_x;
            err -= dy;
            if (err < 0) {
                y += step_y;
                err += dx;
            }
        }
    }
}

void V9938::execute_lmmv() {
    const auto dx = reg16(36);
    const auto dy = reg16(38);
    const auto nx = reg16(40);
    const auto ny = reg16(42);
    for (std::uint16_t row = 0; row < ny; ++row) {
        const auto y = static_cast<std::uint16_t>(dy + (arg_diy() ? -static_cast<int>(row) : static_cast<int>(row)));
        for (std::uint16_t col = 0; col < nx; ++col) {
            const auto x = static_cast<std::uint16_t>(dx + (arg_dix() ? -static_cast<int>(col) : static_cast<int>(col)));
            graphic4_pset(x, y, command_color_value(), reg_[46] & 0x0FU);
        }
    }
}

void V9938::execute_lmmm() {
    const auto sx = reg16(32);
    const auto sy = reg16(34);
    const auto dx = reg16(36);
    const auto dy = reg16(38);
    const auto nx = reg16(40);
    const auto ny = reg16(42);

    std::vector<std::uint8_t> source;
    source.reserve(static_cast<std::size_t>(nx) * static_cast<std::size_t>(ny));
    for (std::uint16_t row = 0; row < ny; ++row) {
        const auto src_y = static_cast<std::uint16_t>(sy + (arg_diy() ? -static_cast<int>(row) : static_cast<int>(row)));
        for (std::uint16_t col = 0; col < nx; ++col) {
            const auto src_x = static_cast<std::uint16_t>(sx + (arg_dix() ? -static_cast<int>(col) : static_cast<int>(col)));
            source.push_back(graphic4_point(src_x, src_y));
        }
    }

    std::size_t index = 0;
    for (std::uint16_t row = 0; row < ny; ++row) {
        const auto dst_y = static_cast<std::uint16_t>(dy + (arg_diy() ? -static_cast<int>(row) : static_cast<int>(row)));
        for (std::uint16_t col = 0; col < nx; ++col) {
            const auto dst_x = static_cast<std::uint16_t>(dx + (arg_dix() ? -static_cast<int>(col) : static_cast<int>(col)));
            graphic4_pset(dst_x, dst_y, source[index++], reg_[46] & 0x0FU);
        }
    }
}

void V9938::execute_hmmv() {
    const auto dx = reg16(36);
    const auto dy = reg16(38);
    const auto nx = reg16(40);
    const auto ny = reg16(42);
    for (std::uint16_t row = 0; row < ny; ++row) {
        const auto y = static_cast<std::uint16_t>(dy + (arg_diy() ? -static_cast<int>(row) : static_cast<int>(row)));
        for (std::uint16_t col = 0; col < nx; ++col) {
            const auto x = static_cast<std::uint16_t>((dx & 0x01FEU) + (arg_dix() ? -static_cast<int>(col * 2U) : static_cast<int>(col * 2U)));
            const auto address = graphic4_command_byte_address(x, y);
            vram_[address] = reg_[44];
        }
    }
}

void V9938::execute_hmmm() {
    const auto sx = reg16(32);
    const auto sy = reg16(34);
    const auto dx = reg16(36);
    const auto dy = reg16(38);
    const auto nx = reg16(40);
    const auto ny = reg16(42);

    std::vector<std::uint8_t> source;
    source.reserve(static_cast<std::size_t>(nx) * static_cast<std::size_t>(ny));
    for (std::uint16_t row = 0; row < ny; ++row) {
        const auto src_y = static_cast<std::uint16_t>(sy + (arg_diy() ? -static_cast<int>(row) : static_cast<int>(row)));
        for (std::uint16_t col = 0; col < nx; ++col) {
            const auto src_x = static_cast<std::uint16_t>((sx & 0x01FEU) + (arg_dix() ? -static_cast<int>(col * 2U) : static_cast<int>(col * 2U)));
            source.push_back(vram_[graphic4_command_byte_address(src_x, src_y)]);
        }
    }

    std::size_t index = 0;
    for (std::uint16_t row = 0; row < ny; ++row) {
        const auto dst_y = static_cast<std::uint16_t>(dy + (arg_diy() ? -static_cast<int>(row) : static_cast<int>(row)));
        for (std::uint16_t col = 0; col < nx; ++col) {
            const auto dst_x = static_cast<std::uint16_t>((dx & 0x01FEU) + (arg_dix() ? -static_cast<int>(col * 2U) : static_cast<int>(col * 2U)));
            vram_[graphic4_command_byte_address(dst_x, dst_y)] = source[index++];
        }
    }
}

void V9938::execute_ymmm() {
    const auto sx = reg16(32);
    const auto sy = reg16(34);
    const auto dx = reg16(36);
    const auto dy = reg16(38);
    const auto ny = reg16(42);
    const auto start_x = static_cast<std::uint16_t>(arg_dix() ? 0x00FEU : 0x0000U);

    for (std::uint16_t row = 0; row < ny; ++row) {
        const auto src_y = static_cast<std::uint16_t>(sy + (arg_diy() ? -static_cast<int>(row) : static_cast<int>(row)));
        const auto dst_y = static_cast<std::uint16_t>(dy + (arg_diy() ? -static_cast<int>(row) : static_cast<int>(row)));
        for (std::uint16_t x = 0; x < 256; x += 2) {
            const auto src_x = static_cast<std::uint16_t>((sx & 0x01FEU) + x);
            const auto dst_x = static_cast<std::uint16_t>((dx & 0x01FEU) + x);
            vram_[graphic4_command_byte_address(dst_x, dst_y)] =
                vram_[graphic4_command_byte_address(src_x, src_y)];
        }
        static_cast<void>(start_x);
    }
}

void V9938::begin_cpu_stream_command(const CommandType type) {
    command_.type = type;
    command_.transfer_ready = true;
    command_.stream_x = reg16(36);
    command_.stream_y = reg16(38);
    command_.remaining_x = reg16(40);
    command_.remaining_y = reg16(42);
    command_.cycles_remaining = 1U;
    status_[2] = static_cast<std::uint8_t>(status_[2] | status_tr);
}

void V9938::stream_command_value(const std::uint8_t value) {
    if (!command_.active || !command_.transfer_ready) {
        return;
    }

    if (command_.type == CommandType::lmmc) {
        stream_lmmc_value(value);
    } else if (command_.type == CommandType::hmmc) {
        stream_hmmc_value(value);
    }
}

void V9938::stream_lmmc_value(const std::uint8_t value) {
    graphic4_pset(command_.stream_x, command_.stream_y, static_cast<std::uint8_t>(value & 0x0FU), reg_[46] & 0x0FU);
    advance_stream_position(false);
}

void V9938::stream_hmmc_value(const std::uint8_t value) {
    vram_[graphic4_command_byte_address(command_.stream_x & 0x01FEU, command_.stream_y)] = value;
    advance_stream_position(true);
}

void V9938::advance_stream_position(const bool byte_mode) {
    if (command_.remaining_x == 0U || command_.remaining_y == 0U) {
        finish_command();
        return;
    }

    --command_.remaining_x;
    if (command_.remaining_x == 0U) {
        command_.remaining_x = reg16(40);
        if (command_.remaining_y > 0U) {
            --command_.remaining_y;
        }
        if (command_.remaining_y == 0U) {
            finish_command();
            return;
        }
        command_.stream_x = reg16(36);
        command_.stream_y = static_cast<std::uint16_t>(
            command_.stream_y + (arg_diy() ? -1 : 1)
        );
        return;
    }

    command_.stream_x = static_cast<std::uint16_t>(
        command_.stream_x + (arg_dix() ? -(byte_mode ? 2 : 1) : (byte_mode ? 2 : 1))
    );
}

void V9938::update_int_state() {}

auto V9938::current_display_mode() const -> DisplayMode {
    const auto mode_bits = static_cast<std::uint8_t>(
        ((reg_[0] & register0_mode_m5) != 0U ? 0x10U : 0x00U) |
        ((reg_[0] & register0_mode_m4) != 0U ? 0x08U : 0x00U) |
        ((reg_[0] & register0_mode_m3) != 0U ? 0x04U : 0x00U) |
        ((reg_[1] & register1_mode_m2) != 0U ? 0x02U : 0x00U) |
        ((reg_[1] & register1_mode_m1) != 0U ? 0x01U : 0x00U)
    );

    switch (mode_bits) {
    case 0x00:
        return DisplayMode::graphic1;
    case 0x04:
        return DisplayMode::graphic2;
    case 0x08:
        return DisplayMode::graphic3;
    case 0x0C:
        return DisplayMode::graphic4;
    case 0x14:
        return DisplayMode::graphic6;
    default:
        return DisplayMode::unsupported;
    }
}

auto V9938::display_enabled() const -> bool { return (reg_[1] & 0x40U) != 0U; }

auto V9938::backdrop_color() const -> std::uint8_t { return static_cast<std::uint8_t>(reg_[7] & 0x0FU); }

auto V9938::current_visible_width() const -> int {
    return current_display_mode() == DisplayMode::graphic6 ? max_visible_width : visible_width;
}

auto V9938::pattern_name_base() const -> std::uint16_t {
    return static_cast<std::uint16_t>(reg_[2] << 10U);
}

auto V9938::graphic1_color_base() const -> std::uint16_t {
    return static_cast<std::uint16_t>(reg_[3] << 6U);
}

auto V9938::graphic1_pattern_base() const -> std::uint16_t {
    return static_cast<std::uint16_t>((reg_[4] & 0x3FU) << 11U);
}

auto V9938::graphic2_color_base() const -> std::uint16_t {
    return static_cast<std::uint16_t>(reg_[3] << 6U);
}

auto V9938::graphic2_pattern_base() const -> std::uint16_t {
    return static_cast<std::uint16_t>((reg_[4] & 0x3FU) << 11U);
}

void V9938::render_graphic1_background_scanline(const int line) {
    if (line < 0 || line >= 192) {
        background_line_buffer_.fill(backdrop_color());
        return;
    }

    const auto tile_row = line / 8;
    const auto row_in_tile = line % 8;
    const auto name_base = pattern_name_base();
    const auto pattern_base = graphic1_pattern_base();
    const auto color_base = graphic1_color_base();

    for (int tile_col = 0; tile_col < 32; ++tile_col) {
        const auto name_address = static_cast<std::uint16_t>(name_base + tile_row * 32 + tile_col);
        const auto pattern_number = vram_[name_address];
        const auto pattern_address =
            static_cast<std::uint16_t>(pattern_base + static_cast<std::uint16_t>(pattern_number) * 8U + row_in_tile);
        const auto color_address =
            static_cast<std::uint16_t>(color_base + static_cast<std::uint16_t>(pattern_number / 8U));
        const auto pattern = vram_[pattern_address];
        const auto colors = vram_[color_address];
        const auto foreground = static_cast<std::uint8_t>((colors >> 4) & 0x0FU);
        const auto background = static_cast<std::uint8_t>(colors & 0x0FU);

        for (int pixel = 0; pixel < 8; ++pixel) {
            const auto x = tile_col * 8 + pixel;
            background_line_buffer_[x] =
                (pattern & (0x80U >> pixel)) != 0U ? foreground : background;
        }
    }
}

void V9938::render_graphic2_background_scanline(const int line) {
    if (line < 0 || line >= 192) {
        background_line_buffer_.fill(backdrop_color());
        return;
    }

    const auto tile_row = line / 8;
    const auto row_in_tile = line % 8;
    const auto bank = tile_row / 8;
    const auto bank_offset = static_cast<std::uint16_t>(bank * 0x0800U);
    const auto name_base = pattern_name_base();
    const auto pattern_base = graphic2_pattern_base();
    const auto color_base = graphic2_color_base();

    for (int tile_col = 0; tile_col < 32; ++tile_col) {
        const auto name_address = static_cast<std::uint16_t>(name_base + tile_row * 32 + tile_col);
        const auto pattern_number = vram_[name_address];
        const auto row_offset = static_cast<std::uint16_t>(pattern_number * 8U + row_in_tile);
        const auto pattern =
            vram_[static_cast<std::uint16_t>(pattern_base + bank_offset + row_offset)];
        const auto colors =
            vram_[static_cast<std::uint16_t>(color_base + bank_offset + row_offset)];
        const auto foreground = static_cast<std::uint8_t>((colors >> 4) & 0x0FU);
        const auto background = static_cast<std::uint8_t>(colors & 0x0FU);

        for (int pixel = 0; pixel < 8; ++pixel) {
            const auto x = tile_col * 8 + pixel;
            background_line_buffer_[x] =
                (pattern & (0x80U >> pixel)) != 0U ? foreground : background;
        }
    }
}

void V9938::render_graphic4_background_scanline(const int line) {
    for (int x = 0; x < visible_width; ++x) {
        const auto address = graphic4_byte_address(line, x);
        const auto byte = vram_[address];
        background_line_buffer_[x] =
            static_cast<std::uint8_t>((x % 2) == 0 ? (byte >> 4) : (byte & 0x0FU));
    }
    for (int x = visible_width; x < max_visible_width; ++x) {
        background_line_buffer_[x] = backdrop_color();
    }
}

void V9938::render_graphic6_background_scanline(const int line) {
    for (int x = 0; x < max_visible_width; ++x) {
        const auto address = graphic6_byte_address(line, x);
        const auto byte = vram_[address];
        background_line_buffer_[x] =
            static_cast<std::uint8_t>((x % 2) == 0 ? (byte >> 4) : (byte & 0x0FU));
    }
}

void V9938::render_graphic3_background_scanline(const int line) {
    if (line < 0 || line >= 192) {
        background_line_buffer_.fill(backdrop_color());
        return;
    }

    const auto tile_row = line / 8;
    const auto row_in_tile = line % 8;
    const auto bank = tile_row / 8;
    const auto bank_offset = static_cast<std::uint16_t>(bank * 0x0800U);

    for (int tile_col = 0; tile_col < 32; ++tile_col) {
        const auto name_address = static_cast<std::uint16_t>(
            graphic3_name_table_base + tile_row * 32 + tile_col
        );
        const auto pattern_number = vram_[name_address];
        const auto row_offset = static_cast<std::uint16_t>(pattern_number * 8U + row_in_tile);
        const auto pattern =
            vram_[static_cast<std::uint16_t>(graphic3_pattern_base + bank_offset + row_offset)];
        const auto colors =
            vram_[static_cast<std::uint16_t>(graphic3_color_base + bank_offset + row_offset)];
        const auto foreground = static_cast<std::uint8_t>((colors >> 4) & 0x0FU);
        const auto background = static_cast<std::uint8_t>(colors & 0x0FU);

        for (int pixel = 0; pixel < 8; ++pixel) {
            const auto x = tile_col * 8 + pixel;
            background_line_buffer_[x] =
                (pattern & (0x80U >> pixel)) != 0U ? foreground : background;
        }
    }
}

void V9938::render_mode1_sprites_for_scanline(const int line) {
    sprite_line_buffer_.fill(transparent_sprite_pixel);

    const auto mode = current_display_mode();
    if (mode != DisplayMode::graphic1 && mode != DisplayMode::graphic2) {
        return;
    }

    std::array<int, max_visible_width> first_owner{};
    first_owner.fill(-1);

    int visible_sprite_count = 0;
    bool collision_recorded = false;

    for (std::uint8_t sprite_index = 0; sprite_index < 32; ++sprite_index) {
        const auto sat_base =
            static_cast<std::uint16_t>(sprite_attribute_base() + sprite_index * 4U);
        const auto y = vram_[sat_base + 0];
        if (y == 0xD0U) {
            break;
        }

        const auto x = vram_[sat_base + 1];
        const auto pattern_number = vram_[sat_base + 2];
        const auto color_flags = vram_[sat_base + 3];
        const auto early_clock = (color_flags & 0x80U) != 0U;
        const auto color = static_cast<std::uint8_t>(color_flags & 0x0FU);
        const auto sprite_left = early_clock ? static_cast<int>(x) - 32 : static_cast<int>(x);
        const auto sprite_top = static_cast<int>(y);
        const auto magnified = sprite_magnified();
        const auto sprite_size = sprite_size_pixels();
        const auto display_size = magnified ? sprite_size * 2 : sprite_size;
        const auto sprite_line = line - sprite_top;
        const auto sprite_row = magnified ? sprite_line / 2 : sprite_line;

        if (sprite_line < 0 || sprite_line >= display_size) {
            continue;
        }

        ++visible_sprite_count;
        if (visible_sprite_count > 4) {
            status_[0] = static_cast<std::uint8_t>(status_[0] | 0x40U);
            continue;
        }

        const auto pattern = sprite_pattern_row_bytes(pattern_number, sprite_row);
        for (int half = 0; half < 2; ++half) {
            if (half == 1 && sprite_size < 16) {
                break;
            }

            for (int bit = 0; bit < 8; ++bit) {
                if ((pattern[half] & (0x80U >> bit)) == 0U) {
                    continue;
                }

                const auto pattern_x = half * 8 + bit;
                const auto pixel_x = sprite_left + pattern_x * (magnified ? 2 : 1);
                const auto pixel_width = magnified ? 2 : 1;
                for (int stretch = 0; stretch < pixel_width; ++stretch) {
                    const auto stretched_x = pixel_x + stretch;
                    if (stretched_x < 0 || stretched_x >= visible_width) {
                        continue;
                    }

                    if (first_owner[stretched_x] >= 0 && !collision_recorded) {
                        status_[0] = static_cast<std::uint8_t>(status_[0] | 0x20U);
                        status_[3] = static_cast<std::uint8_t>(stretched_x & 0xFF);
                        status_[4] = static_cast<std::uint8_t>((stretched_x >> 8) & 0x01);
                        status_[5] = static_cast<std::uint8_t>(line & 0xFF);
                        status_[6] = static_cast<std::uint8_t>((line >> 8) & 0x01);
                        collision_recorded = true;
                    }

                    if (first_owner[stretched_x] >= 0) {
                        continue;
                    }

                    first_owner[stretched_x] = sprite_index;
                    sprite_line_buffer_[stretched_x] = color;
                }
            }
        }
    }
}

void V9938::render_mode2_sprites_for_scanline(const int line) {
    sprite_line_buffer_.fill(transparent_sprite_pixel);

    const auto mode = current_display_mode();
    if (mode != DisplayMode::graphic3 && mode != DisplayMode::graphic4) {
        return;
    }

    std::array<int, max_visible_width> first_owner{};
    first_owner.fill(-1);

    int visible_sprite_count = 0;
    bool collision_recorded = false;

    for (std::uint8_t sprite_index = 0; sprite_index < 32; ++sprite_index) {
        const auto sat_base =
            static_cast<std::uint16_t>(sprite_attribute_base() + sprite_index * 8U);
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
        const auto magnified = sprite_magnified();
        const auto sprite_size = sprite_size_pixels();
        const auto display_size = magnified ? sprite_size * 2 : sprite_size;
        const auto sprite_line = line - sprite_top;
        const auto sprite_row = magnified ? sprite_line / 2 : sprite_line;

        if (sprite_line < 0 || sprite_line >= display_size) {
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

        const auto pattern = sprite_pattern_row_bytes(pattern_number, sprite_row);
        for (int half = 0; half < 2; ++half) {
            if (half == 1 && sprite_size < 16) {
                break;
            }

            for (int bit = 0; bit < 8; ++bit) {
                if ((pattern[half] & (0x80U >> bit)) == 0U) {
                    continue;
                }

                const auto pattern_x = half * 8 + bit;
                const auto pixel_x = sprite_left + pattern_x * (magnified ? 2 : 1);
                const auto pixel_width = magnified ? 2 : 1;
                for (int stretch = 0; stretch < pixel_width; ++stretch) {
                    const auto stretched_x = pixel_x + stretch;
                    if (stretched_x < 0 || stretched_x >= visible_width) {
                        continue;
                    }

                    if (first_owner[stretched_x] >= 0 && !collision_recorded) {
                        status_[0] = static_cast<std::uint8_t>(status_[0] | 0x20U);
                        status_[3] = static_cast<std::uint8_t>(stretched_x & 0xFF);
                        status_[4] = static_cast<std::uint8_t>((stretched_x >> 8) & 0x01);
                        status_[5] = static_cast<std::uint8_t>(line & 0xFF);
                        status_[6] = static_cast<std::uint8_t>((line >> 8) & 0x01);
                        collision_recorded = true;
                    }

                    if (first_owner[stretched_x] >= 0) {
                        continue;
                    }

                    first_owner[stretched_x] = sprite_index;
                    sprite_line_buffer_[stretched_x] = *color;
                }
            }
        }
    }
}

auto V9938::sprite_size_pixels() const -> int { return (reg_[1] & 0x02U) != 0U ? 16 : 8; }

auto V9938::sprite_magnified() const -> bool { return (reg_[1] & 0x01U) != 0U; }

auto V9938::sprite_color_for_line(const std::uint8_t sprite_index, const int row) const
    -> std::optional<std::uint8_t> {
    const auto color_address =
        static_cast<std::uint16_t>(sprite_color_base() + sprite_index * 16U + row);
    const auto color_entry = vram_[color_address];
    if ((color_entry & 0x40U) != 0U) {
        return std::nullopt;
    }
    return static_cast<std::uint8_t>(color_entry & 0x0FU);
}

auto V9938::sprite_pattern_row_bytes(const std::uint8_t pattern_number, const int row) const
    -> std::array<std::uint8_t, 2> {
    const auto pattern_base = sprite_pattern_base();
    const auto row_in_block = static_cast<std::uint16_t>(row & 0x07);
    const auto pattern_group = static_cast<std::uint8_t>(
        sprite_size_pixels() >= 16 ? (pattern_number & 0xFCU) : pattern_number
    );
    const auto block_base =
        static_cast<std::uint16_t>(pattern_base + static_cast<std::uint16_t>(pattern_group) * 8U);

    if (sprite_size_pixels() < 16) {
        return {vram_[static_cast<std::uint16_t>(block_base + row_in_block)], 0x00};
    }

    const auto lower_half = row >= 8;
    const auto left_offset = static_cast<std::uint16_t>(lower_half ? 16U : 0U);
    const auto right_offset = static_cast<std::uint16_t>(left_offset + 8U);
    return {
        vram_[static_cast<std::uint16_t>(block_base + left_offset + row_in_block)],
        vram_[static_cast<std::uint16_t>(block_base + right_offset + row_in_block)],
    };
}

auto V9938::sprite_pattern_base() const -> std::uint16_t {
    return static_cast<std::uint16_t>((reg_[6] & 0x3FU) << 11U);
}

auto V9938::sprite_color_base() const -> std::uint16_t {
    return static_cast<std::uint16_t>(sprite_attribute_base() - 0x0200U);
}

auto V9938::sprite_attribute_base() const -> std::uint16_t {
    return static_cast<std::uint16_t>(
        ((static_cast<std::uint16_t>(reg_[11] & 0x03U)) << 15U) |
        (static_cast<std::uint16_t>(reg_[5]) << 7U)
    );
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

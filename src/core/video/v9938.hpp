#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace vanguard8::core::video {

class V9938 {
  public:
    static constexpr int visible_width = 256;
    static constexpr int visible_height = 212;
    static constexpr int vram_size = 65'536;
    static constexpr int bytes_per_scanline = 128;
    static constexpr std::uint8_t transparent_sprite_pixel = 0xFF;
    static constexpr std::uint8_t register0_mode_m3 = 0x02;
    static constexpr std::uint8_t register0_mode_m4 = 0x04;
    static constexpr std::uint8_t register0_mode_m5 = 0x08;
    static constexpr std::uint8_t register1_mode_m2 = 0x08;
    static constexpr std::uint8_t register1_mode_m1 = 0x10;
    static constexpr std::uint8_t graphic4_mode_r0 =
        static_cast<std::uint8_t>(register0_mode_m4 | register0_mode_m3);
    static constexpr std::uint8_t graphic3_mode_r0 = register0_mode_m4;
    static constexpr std::uint8_t graphic_mode_r1 = 0x00;
    static constexpr std::uint16_t graphic4_sprite_pattern_base = 0x7000;
    static constexpr std::uint16_t graphic4_sprite_color_base = 0x7A00;
    static constexpr std::uint16_t graphic4_sprite_attribute_base = 0x7C00;
    static constexpr std::uint16_t graphic3_name_table_base = 0x0000;
    static constexpr std::uint16_t graphic3_pattern_base = 0x0300;
    static constexpr std::uint16_t graphic3_color_base = 0x1800;
    static constexpr std::uint16_t graphic3_sprite_pattern_base = 0x3000;
    static constexpr std::uint16_t graphic3_sprite_color_base = 0x4000;
    static constexpr std::uint16_t graphic3_sprite_attribute_base = 0x4200;

    using LineBuffer = std::array<std::uint8_t, visible_width>;
    using Framebuffer = std::vector<std::uint8_t>;

    struct CommandStateSnapshot {
        std::uint8_t type = 0;
        bool active = false;
        bool transfer_ready = false;
        std::uint64_t cycles_remaining = 0;
        std::uint16_t stream_x = 0;
        std::uint16_t stream_y = 0;
        std::uint16_t remaining_x = 0;
        std::uint16_t remaining_y = 0;
    };

    struct State {
        std::array<std::uint8_t, vram_size> vram{};
        std::array<std::uint8_t, 64> reg{};
        std::array<std::uint8_t, 10> status{};
        std::array<std::array<std::uint8_t, 2>, 16> palette{};
        std::uint16_t vram_addr = 0;
        std::uint8_t read_ahead_latch = 0;
        std::uint8_t control_latch = 0;
        bool addr_latch_full = false;
        std::uint8_t indirect_register = 0x20;
        std::uint8_t status_reg_select = 0;
        std::uint8_t palette_index = 0;
        std::uint8_t palette_phase = 0;
        CommandStateSnapshot command{};
        LineBuffer background_line_buffer{};
        LineBuffer sprite_line_buffer{};
        LineBuffer line_buffer{};
    };

    V9938();

    void reset();

    [[nodiscard]] auto read_data() -> std::uint8_t;
    void write_data(std::uint8_t value);
    [[nodiscard]] auto read_status() -> std::uint8_t;
    void write_control(std::uint8_t value);
    void write_palette(std::uint8_t value);
    void write_register(std::uint8_t value);
    void poke_vram(std::uint16_t address, std::uint8_t value);
    void advance_command(std::uint64_t master_cycles);

    void tick_scanline(int line);
    [[nodiscard]] auto render_graphic4_frame() -> Framebuffer;
    void set_vblank_flag(bool asserted);
    [[nodiscard]] auto int_pending() const -> bool;
    [[nodiscard]] auto vblank_irq_pending() const -> bool;
    [[nodiscard]] auto hblank_irq_pending() const -> bool;

    [[nodiscard]] auto register_value(std::uint8_t index) const -> std::uint8_t;
    [[nodiscard]] auto status_value(std::uint8_t index) const -> std::uint8_t;
    [[nodiscard]] auto palette_entry_raw(std::uint8_t index) const
        -> std::array<std::uint8_t, 2>;
    [[nodiscard]] auto palette_entry_rgb(std::uint8_t index) const
        -> std::array<std::uint8_t, 3>;
    [[nodiscard]] auto line_buffer() const -> const LineBuffer&;
    [[nodiscard]] auto background_line_buffer() const -> const LineBuffer&;
    [[nodiscard]] auto sprite_line_buffer() const -> const LineBuffer&;
    [[nodiscard]] auto vram() const -> const std::array<std::uint8_t, vram_size>&;
    [[nodiscard]] auto color_zero_transparent() const -> bool;
    [[nodiscard]] auto state_snapshot() const -> State;
    void load_state(const State& state);

    [[nodiscard]] static auto expand3to8(std::uint8_t value) -> std::uint8_t;

  private:
    enum class DisplayMode {
        unsupported,
        graphic3,
        graphic4,
    };

    enum class CommandType {
        none,
        point,
        pset,
        srch,
        line,
        lmmv,
        lmmm,
        lmmc,
        hmmv,
        hmmm,
        ymmm,
        hmmc,
    };

    struct CommandState {
        CommandType type = CommandType::none;
        bool active = false;
        bool transfer_ready = false;
        std::uint64_t cycles_remaining = 0;
        std::uint16_t stream_x = 0;
        std::uint16_t stream_y = 0;
        std::uint16_t remaining_x = 0;
        std::uint16_t remaining_y = 0;
    };

    std::array<std::uint8_t, vram_size> vram_{};
    std::array<std::uint8_t, 64> reg_{};
    std::array<std::uint8_t, 10> status_{};
    std::array<std::array<std::uint8_t, 2>, 16> palette_{};
    std::uint16_t vram_addr_ = 0;
    std::uint8_t read_ahead_latch_ = 0;
    std::uint8_t control_latch_ = 0;
    bool addr_latch_full_ = false;
    std::uint8_t indirect_register_ = 0x20;
    std::uint8_t status_reg_select_ = 0;
    std::uint8_t palette_index_ = 0;
    std::uint8_t palette_phase_ = 0;
    CommandState command_{};
    LineBuffer background_line_buffer_{};
    LineBuffer sprite_line_buffer_{};
    LineBuffer line_buffer_{};

    void write_register_value(std::uint8_t index, std::uint8_t value);
    [[nodiscard]] auto graphic4_byte_address(int line, int x) const -> std::uint16_t;
    [[nodiscard]] auto vertical_scroll() const -> std::uint8_t;
    [[nodiscard]] auto reg16(std::uint8_t low_index) const -> std::uint16_t;
    [[nodiscard]] auto arg_dix() const -> bool;
    [[nodiscard]] auto arg_diy() const -> bool;
    [[nodiscard]] auto arg_eq() const -> bool;
    [[nodiscard]] auto arg_maj() const -> bool;
    [[nodiscard]] auto command_color_mask() const -> std::uint8_t;
    [[nodiscard]] auto command_color_value() const -> std::uint8_t;
    [[nodiscard]] auto graphic4_command_byte_address(std::uint16_t x, std::uint16_t y) const -> std::uint16_t;
    [[nodiscard]] auto graphic4_point(std::uint16_t x, std::uint16_t y) const -> std::uint8_t;
    void graphic4_pset(std::uint16_t x, std::uint16_t y, std::uint8_t source, std::uint8_t op);
    [[nodiscard]] auto apply_logical_op(std::uint8_t source, std::uint8_t dest, std::uint8_t op) const
        -> std::uint8_t;
    void begin_command(std::uint8_t value);
    void finish_command();
    [[nodiscard]] auto estimate_command_cycles(CommandType type) const -> std::uint64_t;
    void execute_immediate_command();
    void execute_point();
    void execute_pset();
    void execute_srch();
    void execute_line();
    void execute_lmmv();
    void execute_lmmm();
    void execute_hmmv();
    void execute_hmmm();
    void execute_ymmm();
    void begin_cpu_stream_command(CommandType type);
    void stream_command_value(std::uint8_t value);
    void stream_lmmc_value(std::uint8_t value);
    void stream_hmmc_value(std::uint8_t value);
    void advance_stream_position(bool byte_mode);
    void update_int_state();
    [[nodiscard]] auto current_display_mode() const -> DisplayMode;
    [[nodiscard]] auto display_enabled() const -> bool;
    [[nodiscard]] auto backdrop_color() const -> std::uint8_t;
    void render_graphic4_background_scanline(int line);
    void render_graphic3_background_scanline(int line);
    void render_mode2_sprites_for_scanline(int line);
    [[nodiscard]] auto sprite_size_pixels() const -> int;
    [[nodiscard]] auto sprite_magnified() const -> bool;
    [[nodiscard]] auto sprite_pattern_base() const -> std::uint16_t;
    [[nodiscard]] auto sprite_color_base() const -> std::uint16_t;
    [[nodiscard]] auto sprite_attribute_base() const -> std::uint16_t;
    [[nodiscard]] auto sprite_color_for_line(std::uint8_t sprite_index, int row) const
        -> std::optional<std::uint8_t>;
    [[nodiscard]] auto sprite_pattern_row_bytes(std::uint8_t pattern_number, int row) const
        -> std::array<std::uint8_t, 2>;
    [[nodiscard]] auto current_palette_rgb(std::uint8_t index) const
        -> std::array<std::uint8_t, 3>;
};

}  // namespace vanguard8::core::video

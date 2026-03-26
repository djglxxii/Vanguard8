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
    static constexpr std::uint16_t graphic4_sprite_pattern_base = 0x6A00;
    static constexpr std::uint16_t graphic4_sprite_color_base = 0x7A00;
    static constexpr std::uint16_t graphic4_sprite_attribute_base = 0x7C00;

    using LineBuffer = std::array<std::uint8_t, visible_width>;
    using Framebuffer = std::vector<std::uint8_t>;

    V9938();

    void reset();

    [[nodiscard]] auto read_data() -> std::uint8_t;
    void write_data(std::uint8_t value);
    [[nodiscard]] auto read_status() -> std::uint8_t;
    void write_control(std::uint8_t value);
    void write_palette(std::uint8_t value);
    void write_register(std::uint8_t value);
    void poke_vram(std::uint16_t address, std::uint8_t value);

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

    [[nodiscard]] static auto expand3to8(std::uint8_t value) -> std::uint8_t;

  private:
    std::array<std::uint8_t, vram_size> vram_{};
    std::array<std::uint8_t, 64> reg_{};
    std::array<std::uint8_t, 10> status_{};
    std::array<std::array<std::uint8_t, 2>, 16> palette_{};
    std::uint16_t vram_addr_ = 0;
    std::uint8_t control_latch_ = 0;
    bool addr_latch_full_ = false;
    std::uint8_t indirect_register_ = 0x20;
    std::uint8_t status_reg_select_ = 0;
    std::uint8_t palette_index_ = 0;
    std::uint8_t palette_phase_ = 0;
    LineBuffer background_line_buffer_{};
    LineBuffer sprite_line_buffer_{};
    LineBuffer line_buffer_{};

    void write_register_value(std::uint8_t index, std::uint8_t value);
    [[nodiscard]] auto graphic4_byte_address(int line, int x) const -> std::uint16_t;
    [[nodiscard]] auto vertical_scroll() const -> std::uint8_t;
    void update_int_state();
    void render_graphic4_background_scanline(int line);
    void render_mode2_sprites_for_scanline(int line);
    [[nodiscard]] auto sprite_color_for_line(std::uint8_t sprite_index, int row) const
        -> std::optional<std::uint8_t>;
    [[nodiscard]] auto sprite_pattern_byte(std::uint8_t pattern_number, int row) const
        -> std::uint8_t;
    [[nodiscard]] auto current_palette_rgb(std::uint8_t index) const
        -> std::array<std::uint8_t, 3>;
};

}  // namespace vanguard8::core::video

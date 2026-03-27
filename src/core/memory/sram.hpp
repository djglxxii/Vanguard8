#pragma once

 #include <array>
 #include <cstddef>
 #include <cstdint>

namespace vanguard8::core::memory {

class Sram {
  public:
    static constexpr std::uint32_t physical_base = 0xF0000;
    static constexpr std::size_t size = 0x8000;

    [[nodiscard]] static auto contains_physical_address(std::uint32_t physical_address) -> bool;
    [[nodiscard]] auto read(std::uint32_t physical_address) const -> std::uint8_t;
    [[nodiscard]] auto bytes() const -> const std::array<std::uint8_t, size>&;
    void write(std::uint32_t physical_address, std::uint8_t value);
    void clear(std::uint8_t fill = 0x00);
    void load_bytes(const std::array<std::uint8_t, size>& bytes);

  private:
    std::array<std::uint8_t, size> bytes_{};
};

}  // namespace vanguard8::core::memory

#pragma once

 #include <cstddef>
 #include <cstdint>
 #include <vector>

namespace vanguard8::core::memory {

class CartridgeSlot {
  public:
    static constexpr std::uint32_t fixed_region_size = 0x4000;
    static constexpr std::uint32_t bank_size = 0x4000;
    static constexpr std::uint32_t fixed_region_base = 0x00000;
    static constexpr std::uint32_t banked_region_base = 0x04000;
    static constexpr std::uint32_t banked_region_limit = 0xEFFFF;
    static constexpr std::uint32_t max_rom_size = 0xF0000;
    static constexpr std::size_t max_switchable_banks = 59;

    CartridgeSlot() = default;
    explicit CartridgeSlot(std::vector<std::uint8_t> rom_image);

    void load(std::vector<std::uint8_t> rom_image);

    [[nodiscard]] static auto contains_physical_address(std::uint32_t physical_address) -> bool;
    [[nodiscard]] auto read_physical(std::uint32_t physical_address) const -> std::uint8_t;
    [[nodiscard]] auto read_fixed(std::uint16_t offset) const -> std::uint8_t;
    [[nodiscard]] auto read_switchable_bank(std::size_t bank_index, std::uint16_t offset) const
        -> std::uint8_t;
    [[nodiscard]] auto rom_size() const -> std::size_t;
    [[nodiscard]] auto switchable_bank_count() const -> std::size_t;
    [[nodiscard]] auto rom_image() const -> const std::vector<std::uint8_t>&;

  private:
    std::vector<std::uint8_t> rom_image_;
};

}  // namespace vanguard8::core::memory

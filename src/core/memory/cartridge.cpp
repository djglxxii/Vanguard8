#include "core/memory/cartridge.hpp"

#include <stdexcept>

namespace vanguard8::core::memory {

CartridgeSlot::CartridgeSlot(std::vector<std::uint8_t> rom_image) { load(std::move(rom_image)); }

void CartridgeSlot::load(std::vector<std::uint8_t> rom_image) {
    if (!rom_image.empty()) {
        if (rom_image.size() < fixed_region_size) {
            throw std::invalid_argument("Cartridge ROM must contain at least 16 KB.");
        }

        if ((rom_image.size() % bank_size) != 0) {
            throw std::invalid_argument("Cartridge ROM must be aligned to 16 KB pages.");
        }

        if (rom_image.size() > max_rom_size) {
            throw std::invalid_argument("Cartridge ROM exceeds the 960 KB limit.");
        }
    }

    rom_image_ = std::move(rom_image);
}

auto CartridgeSlot::contains_physical_address(const std::uint32_t physical_address) -> bool {
    return physical_address <= banked_region_limit;
}

auto CartridgeSlot::read_physical(const std::uint32_t physical_address) const -> std::uint8_t {
    if (!contains_physical_address(physical_address)) {
        return 0xFF;
    }

    if (physical_address >= rom_image_.size()) {
        return 0xFF;
    }

    return rom_image_[physical_address];
}

auto CartridgeSlot::read_fixed(const std::uint16_t offset) const -> std::uint8_t {
    if (offset >= fixed_region_size) {
        return 0xFF;
    }

    return read_physical(offset);
}

auto CartridgeSlot::read_switchable_bank(const std::size_t bank_index, const std::uint16_t offset) const
    -> std::uint8_t {
    if (offset >= bank_size || bank_index >= max_switchable_banks) {
        return 0xFF;
    }

    const auto physical_address =
        banked_region_base + static_cast<std::uint32_t>(bank_index * bank_size) + offset;
    return read_physical(physical_address);
}

auto CartridgeSlot::rom_size() const -> std::size_t { return rom_image_.size(); }

auto CartridgeSlot::switchable_bank_count() const -> std::size_t {
    if (rom_image_.size() <= fixed_region_size) {
        return 0;
    }

    return (rom_image_.size() - fixed_region_size) / bank_size;
}

auto CartridgeSlot::rom_image() const -> const std::vector<std::uint8_t>& { return rom_image_; }

}  // namespace vanguard8::core::memory

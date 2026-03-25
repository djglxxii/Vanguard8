#include "core/memory/sram.hpp"

namespace vanguard8::core::memory {

auto Sram::contains_physical_address(const std::uint32_t physical_address) -> bool {
    return physical_address >= physical_base && physical_address < (physical_base + size);
}

auto Sram::read(const std::uint32_t physical_address) const -> std::uint8_t {
    if (!contains_physical_address(physical_address)) {
        return 0xFF;
    }

    return bytes_[physical_address - physical_base];
}

void Sram::write(const std::uint32_t physical_address, const std::uint8_t value) {
    if (!contains_physical_address(physical_address)) {
        return;
    }

    bytes_[physical_address - physical_base] = value;
}

void Sram::clear(const std::uint8_t fill) { bytes_.fill(fill); }

}  // namespace vanguard8::core::memory

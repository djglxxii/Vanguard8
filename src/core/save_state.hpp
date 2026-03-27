#pragma once

#include "core/emulator.hpp"

#include <cstdint>
#include <vector>

namespace vanguard8::core {

class SaveState {
  public:
    static constexpr std::uint32_t format_version = 1;

    [[nodiscard]] static auto serialize(const Emulator& emulator) -> std::vector<std::uint8_t>;
    static void load(Emulator& emulator, const std::vector<std::uint8_t>& bytes);
};

}  // namespace vanguard8::core

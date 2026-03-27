#pragma once

#include "core/hash.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace vanguard8::core::replay {

enum class AnchorType : std::uint8_t {
    power_on = 0,
    embedded_save_state = 1,
};

struct FrameRecord {
    std::uint32_t frame_number = 0;
    std::uint8_t controller1 = 0xFF;
    std::uint8_t controller2 = 0xFF;
};

struct Recording {
    Sha256Digest rom_sha256{};
    AnchorType anchor_type = AnchorType::power_on;
    std::vector<std::uint8_t> anchor_save_state;
    std::vector<FrameRecord> frames;
};

}  // namespace vanguard8::core::replay

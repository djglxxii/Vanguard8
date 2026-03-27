#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace vanguard8::core {

using Sha256Digest = std::array<std::uint8_t, 32>;

[[nodiscard]] auto sha256(const std::vector<std::uint8_t>& bytes) -> Sha256Digest;

}  // namespace vanguard8::core

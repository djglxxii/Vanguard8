#include "third_party/msm5205/msm5205_core.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace vanguard8::third_party::msm5205 {

namespace {

constexpr std::array<int, 8> index_shift = {-1, -1, -1, -1, 2, 4, 6, 8};

constexpr std::array<std::array<int, 4>, 16> nibble_to_bits = {{
    {{1, 0, 0, 0}},
    {{1, 0, 0, 1}},
    {{1, 0, 1, 0}},
    {{1, 0, 1, 1}},
    {{1, 1, 0, 0}},
    {{1, 1, 0, 1}},
    {{1, 1, 1, 0}},
    {{1, 1, 1, 1}},
    {{-1, 0, 0, 0}},
    {{-1, 0, 0, 1}},
    {{-1, 0, 1, 0}},
    {{-1, 0, 1, 1}},
    {{-1, 1, 0, 0}},
    {{-1, 1, 0, 1}},
    {{-1, 1, 1, 0}},
    {{-1, 1, 1, 1}},
}};

}  // namespace

Core::Core() { compute_tables(); }

void Core::reset() {
    signal_ = 0;
    step_ = 0;
}

void Core::clock(const std::uint8_t nibble) {
    const auto value = static_cast<int>(nibble & 0x0FU);
    auto next_signal = signal_ + diff_lookup_[static_cast<std::size_t>(step_ * 16 + value)];
    next_signal = std::clamp(next_signal, -2048, 2047);

    step_ += index_shift[static_cast<std::size_t>(value & 0x07)];
    step_ = std::clamp(step_, 0, 48);
    signal_ = next_signal;
}

auto Core::sample() const -> std::int16_t { return static_cast<std::int16_t>(signal_); }

void Core::compute_tables() {
    for (int step = 0; step <= 48; ++step) {
        const auto step_value = static_cast<int>(std::floor(16.0 * std::pow(11.0 / 10.0, step)));
        for (int nibble = 0; nibble < 16; ++nibble) {
            const auto& bits = nibble_to_bits[static_cast<std::size_t>(nibble)];
            diff_lookup_[static_cast<std::size_t>(step * 16 + nibble)] =
                bits[0] *
                (step_value * bits[1] + (step_value / 2) * bits[2] + (step_value / 4) * bits[3] +
                 (step_value / 8));
        }
    }
}

}  // namespace vanguard8::third_party::msm5205

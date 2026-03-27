#pragma once

#include "core/audio/sample.hpp"

#include <array>
#include <cstdint>

extern "C" {
#include "third_party/ayumi/ayumi.h"
}

namespace vanguard8::core::audio {

struct Ay8910State {
    ayumi chip{};
    std::array<std::uint8_t, 16> registers{};
    std::uint8_t selected_register = 0;
    std::uint32_t master_cycle_divider = 0;
    float latest_mono_sample = 0.0F;
    float common_window_sum = 0.0F;
    std::uint32_t common_window_samples = 0;
};

class Ay8910 {
  public:
    static constexpr std::uint32_t master_cycles_per_sample = 128;

    Ay8910() { reset(); }

    void reset();
    void write_address(std::uint8_t reg);
    void write_data(std::uint8_t data);
    [[nodiscard]] auto read_data() const -> std::uint8_t;
    void advance_master_cycle();
    [[nodiscard]] auto consume_common_rate_sample() -> float;
    [[nodiscard]] auto current_output() const -> StereoSample;
    [[nodiscard]] auto selected_register() const -> std::uint8_t;
    [[nodiscard]] auto state_snapshot() const -> Ay8910State;
    void load_state(const Ay8910State& state);

  private:
    ayumi chip_{};
    std::array<std::uint8_t, 16> registers_{};
    std::uint8_t selected_register_ = 0;
    std::uint32_t master_cycle_divider_ = 0;
    float latest_mono_sample_ = 0.0F;
    float common_window_sum_ = 0.0F;
    std::uint32_t common_window_samples_ = 0;

    [[nodiscard]] static auto mask_register(std::uint8_t reg, std::uint8_t value) -> std::uint8_t;
    void apply_channel(int channel);
    void apply_register(std::uint8_t reg);
};

}  // namespace vanguard8::core::audio

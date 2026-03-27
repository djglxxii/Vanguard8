#pragma once

#include "third_party/msm5205/msm5205_core.hpp"

#include <cstdint>

namespace vanguard8::core::audio {

enum class Msm5205Rate {
    stopped,
    hz_4000,
    hz_6000,
    hz_8000,
};

struct Msm5205State {
    vanguard8::third_party::msm5205::Core::State core;
    std::uint8_t control = 0x83;
    std::uint8_t latched_nibble = 0;
    std::uint64_t vclk_count = 0;
};

class Msm5205Adapter {
  public:
    Msm5205Adapter() { reset(); }

    void reset();
    void write_control(std::uint8_t data);
    void write_data(std::uint8_t nibble);
    [[nodiscard]] auto trigger_vclk() -> bool;
    [[nodiscard]] auto current_output() const -> float;
    [[nodiscard]] auto control() const -> std::uint8_t;
    [[nodiscard]] auto latched_nibble() const -> std::uint8_t;
    [[nodiscard]] auto vclk_rate() const -> Msm5205Rate;
    [[nodiscard]] auto vclk_enabled() const -> bool;
    [[nodiscard]] auto vclk_count() const -> std::uint64_t;
    [[nodiscard]] auto state_snapshot() const -> Msm5205State;
    void load_state(const Msm5205State& state);

  private:
    vanguard8::third_party::msm5205::Core core_{};
    std::uint8_t control_ = 0x83;
    std::uint8_t latched_nibble_ = 0;
    std::uint64_t vclk_count_ = 0;
};

}  // namespace vanguard8::core::audio

#include "core/audio/msm5205_adapter.hpp"

#include <algorithm>

namespace vanguard8::core::audio {

void Msm5205Adapter::reset() {
    core_.reset();
    control_ = 0x83;
    latched_nibble_ = 0;
    vclk_count_ = 0;
}

void Msm5205Adapter::write_control(const std::uint8_t data) {
    control_ = data;
    if (!vclk_enabled()) {
        core_.reset();
    }
}

void Msm5205Adapter::write_data(const std::uint8_t nibble) { latched_nibble_ = nibble & 0x0FU; }

auto Msm5205Adapter::trigger_vclk() -> bool {
    if (!vclk_enabled()) {
        return false;
    }

    core_.clock(latched_nibble_);
    ++vclk_count_;
    return true;
}

auto Msm5205Adapter::current_output() const -> float {
    return std::clamp(static_cast<float>(core_.sample()) / 2048.0F, -1.0F, 1.0F);
}

auto Msm5205Adapter::control() const -> std::uint8_t { return control_; }

auto Msm5205Adapter::latched_nibble() const -> std::uint8_t { return latched_nibble_; }

auto Msm5205Adapter::vclk_rate() const -> Msm5205Rate {
    if ((control_ & 0x80U) != 0U) {
        return Msm5205Rate::stopped;
    }

    switch (control_ & 0x03U) {
    case 0x00:
        return Msm5205Rate::hz_4000;
    case 0x01:
        return Msm5205Rate::hz_6000;
    case 0x02:
        return Msm5205Rate::hz_8000;
    default:
        return Msm5205Rate::stopped;
    }
}

auto Msm5205Adapter::vclk_enabled() const -> bool { return vclk_rate() != Msm5205Rate::stopped; }

auto Msm5205Adapter::vclk_count() const -> std::uint64_t { return vclk_count_; }

auto Msm5205Adapter::state_snapshot() const -> Msm5205State {
    return Msm5205State{
        .core = core_.state_snapshot(),
        .control = control_,
        .latched_nibble = latched_nibble_,
        .vclk_count = vclk_count_,
    };
}

void Msm5205Adapter::load_state(const Msm5205State& state) {
    core_.load_state(state.core);
    control_ = state.control;
    latched_nibble_ = state.latched_nibble;
    vclk_count_ = state.vclk_count;
}

}  // namespace vanguard8::core::audio

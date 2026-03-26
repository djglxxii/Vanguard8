#include "core/audio/ay8910.hpp"

#include "core/scheduler.hpp"

#include <algorithm>
#include <cmath>

namespace vanguard8::core::audio {

namespace {

constexpr auto tone_mask = 0x0FFFU;
constexpr auto coarse_mask = 0x0FU;
constexpr auto noise_mask = 0x1FU;
constexpr auto mixer_mask = 0x3FU;
constexpr auto volume_mask = 0x1FU;
constexpr auto envelope_mask = 0x0FU;

[[nodiscard]] auto channel_period(const std::array<std::uint8_t, 16>& registers, const int base_register)
    -> int {
    const auto fine = static_cast<unsigned>(registers[static_cast<std::size_t>(base_register)]);
    const auto coarse =
        static_cast<unsigned>(registers[static_cast<std::size_t>(base_register + 1)] & coarse_mask);
    return static_cast<int>((coarse << 8U) | fine);
}

[[nodiscard]] auto clamp_unit(const float value) -> float {
    return std::clamp(value, -1.0F, 1.0F);
}

}  // namespace

void Ay8910::reset() {
    chip_ = ayumi{};
    registers_.fill(0);
    selected_register_ = 0;
    master_cycle_divider_ = 0;
    latest_mono_sample_ = 0.0F;
    common_window_sum_ = 0.0F;
    common_window_samples_ = 0;

    const auto clock_rate = static_cast<double>(timing::master_hz) / 8.0;
    const auto sample_rate = static_cast<int>(std::llround(static_cast<double>(timing::master_hz) / 128.0));
    ayumi_configure(&chip_, 0, clock_rate, sample_rate);
    for (int channel = 0; channel < 3; ++channel) {
        ayumi_set_pan(&chip_, channel, 0.5, 0);
    }

    for (std::uint8_t reg = 0; reg < registers_.size(); ++reg) {
        apply_register(reg);
    }
}

void Ay8910::write_address(const std::uint8_t reg) { selected_register_ = reg & 0x0FU; }

void Ay8910::write_data(const std::uint8_t data) {
    registers_[selected_register_] = mask_register(selected_register_, data);
    apply_register(selected_register_);
}

auto Ay8910::read_data() const -> std::uint8_t { return registers_[selected_register_]; }

void Ay8910::advance_master_cycle() {
    ++master_cycle_divider_;
    if (master_cycle_divider_ < master_cycles_per_sample) {
        return;
    }

    master_cycle_divider_ = 0;
    ayumi_process(&chip_);
    ayumi_remove_dc(&chip_);

    latest_mono_sample_ = clamp_unit(static_cast<float>((chip_.left + chip_.right) * 0.5));
    common_window_sum_ += latest_mono_sample_;
    ++common_window_samples_;
}

auto Ay8910::consume_common_rate_sample() -> float {
    if (common_window_samples_ == 0U) {
        return latest_mono_sample_;
    }

    const auto sample = common_window_sum_ / static_cast<float>(common_window_samples_);
    common_window_sum_ = 0.0F;
    common_window_samples_ = 0;
    return sample;
}

auto Ay8910::current_output() const -> StereoSample { return make_centered_sample(latest_mono_sample_); }

auto Ay8910::selected_register() const -> std::uint8_t { return selected_register_; }

auto Ay8910::mask_register(const std::uint8_t reg, const std::uint8_t value) -> std::uint8_t {
    switch (reg) {
    case 1:
    case 3:
    case 5:
        return static_cast<std::uint8_t>(value & coarse_mask);
    case 6:
        return static_cast<std::uint8_t>(value & noise_mask);
    case 7:
        return static_cast<std::uint8_t>(value & mixer_mask);
    case 8:
    case 9:
    case 10:
        return static_cast<std::uint8_t>(value & volume_mask);
    case 13:
        return static_cast<std::uint8_t>(value & envelope_mask);
    default:
        return value;
    }
}

void Ay8910::apply_channel(const int channel) {
    const auto period = channel_period(registers_, channel * 2);
    ayumi_set_tone(&chip_, channel, period);

    const auto mixer = registers_[7];
    const auto tone_disabled = (mixer & (1U << channel)) != 0U ? 1 : 0;
    const auto noise_disabled = (mixer & (1U << (channel + 3))) != 0U ? 1 : 0;
    const auto volume = registers_[static_cast<std::size_t>(8 + channel)];
    const auto envelope_enabled = (volume & 0x10U) != 0U ? 1 : 0;

    ayumi_set_mixer(&chip_, channel, tone_disabled, noise_disabled, envelope_enabled);
    ayumi_set_volume(&chip_, channel, volume & 0x0FU);
}

void Ay8910::apply_register(const std::uint8_t reg) {
    switch (reg) {
    case 0:
    case 1:
        apply_channel(0);
        break;
    case 2:
    case 3:
        apply_channel(1);
        break;
    case 4:
    case 5:
        apply_channel(2);
        break;
    case 6:
        ayumi_set_noise(&chip_, registers_[6] & noise_mask);
        break;
    case 7:
        apply_channel(0);
        apply_channel(1);
        apply_channel(2);
        break;
    case 8:
    case 9:
    case 10:
        apply_channel(static_cast<int>(reg - 8U));
        break;
    case 11:
    case 12: {
        const auto period =
            static_cast<int>((static_cast<unsigned>(registers_[12]) << 8U) | registers_[11]);
        ayumi_set_envelope(&chip_, period);
        break;
    }
    case 13:
        ayumi_set_envelope_shape(&chip_, registers_[13] & envelope_mask);
        break;
    default:
        break;
    }
}

}  // namespace vanguard8::core::audio

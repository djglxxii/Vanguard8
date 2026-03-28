#include "core/audio/audio_mixer.hpp"

#include <algorithm>
#include <cmath>

namespace vanguard8::core::audio {

namespace {

constexpr std::uint64_t fnv_offset_basis = 1469598103934665603ULL;
constexpr std::uint64_t fnv_prime = 1099511628211ULL;

[[nodiscard]] auto quantize(const float value) -> std::int32_t {
    return static_cast<std::int32_t>(std::lround(value * 32767.0F));
}

}  // namespace

void AudioMixer::reset() {
    current_common_sample_ = {};
    common_cycle_divider_ = 0;
    output_phase_ = 0;
    output_digest_ = fnv_offset_basis;
    total_output_sample_count_ = 0;
    frame_output_sample_count_ = 0;
    output_bytes_.clear();
}

auto AudioMixer::advance_common_clock() -> bool {
    ++common_cycle_divider_;
    if (common_cycle_divider_ < common_sample_divider) {
        return false;
    }

    common_cycle_divider_ = 0;
    return true;
}

void AudioMixer::set_common_sample(const StereoSample sample) { current_common_sample_ = sample; }

void AudioMixer::advance_output_clock() {
    output_phase_ += output_sample_rate;
    while (output_phase_ >= 14'318'180ULL) {
        output_phase_ -= 14'318'180ULL;
        append_output_sample(current_common_sample_);
    }
}

void AudioMixer::end_frame() { frame_output_sample_count_ = 0; }

auto AudioMixer::output_digest() const -> std::uint64_t { return output_digest_; }

auto AudioMixer::frame_output_sample_count() const -> std::size_t { return frame_output_sample_count_; }

auto AudioMixer::total_output_sample_count() const -> std::uint64_t { return total_output_sample_count_; }

auto AudioMixer::current_common_sample() const -> StereoSample { return current_common_sample_; }

auto AudioMixer::output_bytes() const -> const std::vector<std::uint8_t>& { return output_bytes_; }

auto AudioMixer::consume_output_bytes() -> std::vector<std::uint8_t> {
    auto bytes = std::move(output_bytes_);
    output_bytes_.clear();
    return bytes;
}

auto AudioMixer::state_snapshot() const -> AudioMixerState {
    return AudioMixerState{
        .current_common_sample = current_common_sample_,
        .common_cycle_divider = common_cycle_divider_,
        .output_phase = output_phase_,
        .output_digest = output_digest_,
        .total_output_sample_count = total_output_sample_count_,
        .frame_output_sample_count = frame_output_sample_count_,
    };
}

void AudioMixer::load_state(const AudioMixerState& state) {
    current_common_sample_ = state.current_common_sample;
    common_cycle_divider_ = state.common_cycle_divider;
    output_phase_ = state.output_phase;
    output_digest_ = state.output_digest;
    total_output_sample_count_ = state.total_output_sample_count;
    frame_output_sample_count_ = state.frame_output_sample_count;
}

void AudioMixer::append_output_sample(const StereoSample& sample) {
    const std::array<std::int32_t, 2> channels = {
        quantize(sample.left),
        quantize(sample.right),
    };
    for (const auto channel : channels) {
        output_digest_ ^= static_cast<std::uint64_t>(static_cast<std::uint32_t>(channel));
        output_digest_ *= fnv_prime;
        const auto sample16 = static_cast<std::int16_t>(std::clamp(channel, -32768, 32767));
        output_bytes_.push_back(static_cast<std::uint8_t>(sample16 & 0x00FF));
        output_bytes_.push_back(static_cast<std::uint8_t>((sample16 >> 8U) & 0x00FF));
    }
    ++total_output_sample_count_;
    ++frame_output_sample_count_;
}

}  // namespace vanguard8::core::audio

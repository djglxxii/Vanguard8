#pragma once

#include "core/audio/sample.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace vanguard8::core::audio {

struct AudioMixerState {
    StereoSample current_common_sample{};
    std::uint32_t common_cycle_divider = 0;
    std::uint64_t output_phase = 0;
    std::uint64_t output_digest = 0;
    std::uint64_t total_output_sample_count = 0;
    std::size_t frame_output_sample_count = 0;
};

class AudioMixer {
  public:
    static constexpr std::uint32_t common_sample_divider = 256;
    static constexpr std::uint32_t output_sample_rate = 48'000;

    AudioMixer() { reset(); }

    void reset();
    [[nodiscard]] auto advance_common_clock() -> bool;
    void set_common_sample(StereoSample sample);
    void advance_output_clock();
    void end_frame();
    [[nodiscard]] auto output_digest() const -> std::uint64_t;
    [[nodiscard]] auto frame_output_sample_count() const -> std::size_t;
    [[nodiscard]] auto total_output_sample_count() const -> std::uint64_t;
    [[nodiscard]] auto current_common_sample() const -> StereoSample;
    [[nodiscard]] auto output_bytes() const -> const std::vector<std::uint8_t>&;
    [[nodiscard]] auto consume_output_bytes() -> std::vector<std::uint8_t>;
    [[nodiscard]] auto state_snapshot() const -> AudioMixerState;
    void load_state(const AudioMixerState& state);

  private:
    StereoSample current_common_sample_{};
    std::uint32_t common_cycle_divider_ = 0;
    std::uint64_t output_phase_ = 0;
    std::uint64_t output_digest_ = 0;
    std::uint64_t total_output_sample_count_ = 0;
    std::size_t frame_output_sample_count_ = 0;
    std::vector<std::uint8_t> output_bytes_{};

    void append_output_sample(const StereoSample& sample);
};

}  // namespace vanguard8::core::audio

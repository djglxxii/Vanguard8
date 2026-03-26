#pragma once

namespace vanguard8::core::audio {

struct StereoSample {
    float left = 0.0F;
    float right = 0.0F;
};

[[nodiscard]] constexpr auto make_centered_sample(const float mono) -> StereoSample {
    return StereoSample{
        .left = mono,
        .right = mono,
    };
}

}  // namespace vanguard8::core::audio

#pragma once

#include "core/video/v9938.hpp"

namespace vanguard8::frontend {

void build_video_fixture_frame(core::video::V9938& vdp);
void build_dual_vdp_fixture_frame(core::video::V9938& vdp_a, core::video::V9938& vdp_b);

}  // namespace vanguard8::frontend

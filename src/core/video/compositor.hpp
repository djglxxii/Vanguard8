#pragma once

#include "core/video/v9938.hpp"

namespace vanguard8::core::video {

class Compositor {
  public:
    [[nodiscard]] static auto compose_single_vdp(const V9938& vdp) -> V9938::Framebuffer {
        return const_cast<V9938&>(vdp).render_graphic4_frame();
    }
};

}  // namespace vanguard8::core::video

#include "frontend/app.hpp"

#include "core/config.hpp"
#include "core/emulator.hpp"
#include "core/logging.hpp"
#include "core/video/compositor.hpp"
#include "core/video/v9938.hpp"
#include "frontend/display.hpp"
#include "frontend/video_fixture.hpp"

#include <iostream>
#include <string>

namespace vanguard8::frontend {

auto run_frontend_app(int argc, char** argv) -> int {
    const auto config = core::load_or_create_config(core::default_config_path());
    (void)config;

    bool render_fixture = false;
    for (int index = 1; index < argc; ++index) {
        if (std::string(argv[index]) == "--video-fixture") {
            render_fixture = true;
        }
    }

    const core::Emulator emulator;
    core::log(core::LogLevel::info, "Launching frontend video path.");
    std::cout << emulator.build_summary() << '\n';

    if (!render_fixture) {
        std::cout << "Frontend status: single-VDP upload path available" << '\n';
        return 0;
    }

    core::video::V9938 vdp;
    build_video_fixture_frame(vdp);
    Display display;
    display.upload_frame(core::video::Compositor::compose_single_vdp(vdp));

    std::cout << "Frontend status: single-VDP upload path" << '\n';
    std::cout << "Frontend frame digest: " << display.frame_digest() << '\n';
    return 0;
}

}  // namespace vanguard8::frontend

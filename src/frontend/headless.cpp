#include "frontend/headless.hpp"

#include "core/config.hpp"
#include "core/emulator.hpp"
#include "core/logging.hpp"
#include "core/video/compositor.hpp"
#include "core/video/v9938.hpp"
#include "frontend/display.hpp"
#include "frontend/video_fixture.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

namespace vanguard8::frontend {

auto run_headless_app(int argc, char** argv) -> int {
    const auto config = core::load_or_create_config(core::default_config_path());
    (void)config;

    std::uint64_t frames_to_run = 0;
    bool start_paused = false;
    bool step_one_frame = false;
    auto vclk_rate = core::VclkRate::stopped;
    bool dump_fixture = false;
    std::filesystem::path dump_path;

    try {
        for (int index = 1; index < argc; ++index) {
            const std::string arg = argv[index];
            if (arg == "--frames" && (index + 1) < argc) {
                frames_to_run = std::stoull(argv[++index]);
                continue;
            }
            if (arg == "--paused") {
                start_paused = true;
                continue;
            }
            if (arg == "--step-frame") {
                step_one_frame = true;
                continue;
            }
            if (arg == "--vclk" && (index + 1) < argc) {
                vclk_rate = core::Emulator::parse_vclk_rate(argv[++index]);
                continue;
            }
            if (arg == "--dump-frame" && (index + 1) < argc) {
                dump_fixture = true;
                dump_path = argv[++index];
                continue;
            }
            if (arg == "--help") {
                std::cout << "Usage: vanguard8_headless [--frames N] [--paused] [--step-frame] [--vclk off|4000|6000|8000] [--dump-frame path.ppm]\n";
                return 0;
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "Headless argument error: " << error.what() << '\n';
        return 2;
    }

    core::Emulator emulator;
    emulator.set_vclk_rate(vclk_rate);
    if (start_paused) {
        emulator.pause();
    }

    core::log(core::LogLevel::info, "Launching deterministic headless runtime.");
    std::cout << emulator.build_summary() << '\n';

    if (dump_fixture) {
        core::video::V9938 vdp;
        build_video_fixture_frame(vdp);
        Display display;
        display.upload_frame(core::video::Compositor::compose_single_vdp(vdp));
        display.dump_ppm_file(dump_path);
        std::cout << "Headless status: frame dump complete" << '\n';
        std::cout << "Frame dump path: " << dump_path.string() << '\n';
        std::cout << "Frame dump digest: " << display.frame_digest() << '\n';
        return 0;
    }

    if (step_one_frame) {
        emulator.step_frame();
    } else if (frames_to_run > 0) {
        emulator.run_frames(frames_to_run);
    }

    std::cout << "Headless status: deterministic runtime" << '\n';
    std::cout << "VCLK: " << core::Emulator::vclk_rate_name(emulator.vclk_rate()) << '\n';
    std::cout << "Frames completed: " << emulator.completed_frames() << '\n';
    std::cout << "Master cycles: " << emulator.master_cycle() << '\n';
    std::cout << "CPU T-states: " << emulator.cpu_tstates() << '\n';
    std::cout << "Event log size: " << emulator.event_log().size() << '\n';
    std::cout << "Event log digest: " << emulator.event_log_digest() << '\n';
    return 0;
}

}  // namespace vanguard8::frontend

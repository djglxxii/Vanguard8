#include "frontend/headless.hpp"

#include "core/config.hpp"
#include "core/emulator.hpp"
#include "core/logging.hpp"
#include "core/video/compositor.hpp"
#include "frontend/display.hpp"
#include "frontend/input.hpp"
#include "frontend/rom_loader.hpp"
#include "frontend/video_fixture.hpp"

#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace vanguard8::frontend {

namespace {

struct RuntimeOptions {
    bool help_requested = false;
    std::uint64_t frames_to_run = 0;
    bool start_paused = false;
    bool step_one_frame = false;
    core::VclkRate vclk_rate = core::VclkRate::stopped;
    bool dump_fixture = false;
    std::filesystem::path dump_path;
    std::optional<std::filesystem::path> rom_path;
    std::optional<std::size_t> recent_index;
    std::vector<std::string> pressed_keys;
    std::vector<std::string> gamepad1_buttons;
    std::vector<std::string> gamepad2_buttons;
};

auto parse_options(int argc, char** argv) -> RuntimeOptions {
    RuntimeOptions options;
    for (int index = 1; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "--frames" && (index + 1) < argc) {
            options.frames_to_run = std::stoull(argv[++index]);
            continue;
        }
        if (arg == "--paused") {
            options.start_paused = true;
            continue;
        }
        if (arg == "--step-frame") {
            options.step_one_frame = true;
            continue;
        }
        if (arg == "--vclk" && (index + 1) < argc) {
            options.vclk_rate = core::Emulator::parse_vclk_rate(argv[++index]);
            continue;
        }
        if (arg == "--dump-frame" && (index + 1) < argc) {
            options.dump_fixture = true;
            options.dump_path = argv[++index];
            continue;
        }
        if (arg == "--rom" && (index + 1) < argc) {
            options.rom_path = argv[++index];
            continue;
        }
        if (arg == "--recent" && (index + 1) < argc) {
            options.recent_index = static_cast<std::size_t>(std::stoull(argv[++index]));
            continue;
        }
        if (arg == "--press-key" && (index + 1) < argc) {
            options.pressed_keys.emplace_back(argv[++index]);
            continue;
        }
        if (arg == "--gamepad1-button" && (index + 1) < argc) {
            options.gamepad1_buttons.emplace_back(argv[++index]);
            continue;
        }
        if (arg == "--gamepad2-button" && (index + 1) < argc) {
            options.gamepad2_buttons.emplace_back(argv[++index]);
            continue;
        }
        if (arg == "--help") {
            options.help_requested = true;
            return options;
        }
    }
    return options;
}

}  // namespace

auto run_headless_app(int argc, char** argv) -> int {
    const auto config_path = core::default_config_path();
    auto config = core::load_or_create_config(config_path);

    RuntimeOptions options;
    try {
        options = parse_options(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "Headless argument error: " << error.what() << '\n';
        return 2;
    }

    if (options.help_requested) {
        std::cout
            << "Usage: vanguard8_headless [--rom path] [--recent index] [--frames N] [--paused] "
               "[--step-frame] [--vclk off|4000|6000|8000] [--dump-frame path.ppm] "
               "[--press-key NAME] [--gamepad1-button NAME] [--gamepad2-button NAME]\n";
        return 0;
    }

    core::Emulator emulator;
    emulator.set_vclk_rate(options.vclk_rate);
    if (options.start_paused) {
        emulator.pause();
    }

    std::optional<LoadedRom> loaded_rom;
    try {
        if (options.recent_index.has_value()) {
            if (*options.recent_index == 0 || *options.recent_index > config.recent_roms.size()) {
                throw std::out_of_range("Recent ROM index is out of range.");
            }
            loaded_rom = load_rom_file(config.recent_roms[*options.recent_index - 1]);
        } else if (options.rom_path.has_value()) {
            loaded_rom = load_rom_file(*options.rom_path);
        }
    } catch (const std::exception& error) {
        std::cerr << "ROM load error: " << error.what() << '\n';
        return 2;
    }

    if (loaded_rom.has_value()) {
        emulator.load_rom_image(loaded_rom->bytes);
        record_recent_rom(config, loaded_rom->path);
        save_config(config_path, config);
    }

    InputManager input(emulator.mutable_bus().mutable_controller_ports());
    input.configure(config.input);
    input.reset();
    for (const auto& key : options.pressed_keys) {
        (void)input.press_key(key);
    }
    for (const auto& button : options.gamepad1_buttons) {
        (void)input.press_gamepad_button(0, button);
    }
    for (const auto& button : options.gamepad2_buttons) {
        (void)input.press_gamepad_button(1, button);
    }

    core::log(core::LogLevel::info, "Launching deterministic headless runtime.");
    std::cout << emulator.build_summary() << '\n';

    if (options.dump_fixture) {
        build_dual_vdp_fixture_frame(
            emulator.mutable_bus().mutable_vdp_a(),
            emulator.mutable_bus().mutable_vdp_b()
        );
        Display display;
        display.upload_frame(core::video::Compositor::compose_dual_vdp(emulator.vdp_a(), emulator.vdp_b()));
        display.dump_ppm_file(options.dump_path);
        std::cout << "Headless status: frame dump complete" << '\n';
        if (loaded_rom.has_value()) {
            std::cout << "ROM path: " << loaded_rom->path.string() << '\n';
        }
        const auto p1 = emulator.bus().controller_ports().read(core::io::Player::one);
        const auto p2 = emulator.bus().controller_ports().read(core::io::Player::two);
        std::cout << "Controller 1 port: 0x" << std::hex << std::uppercase
                  << static_cast<int>(p1) << '\n';
        std::cout << "Controller 2 port: 0x" << std::hex << std::uppercase
                  << static_cast<int>(p2) << '\n';
        std::cout << std::dec;
        std::cout << "Frame dump path: " << options.dump_path.string() << '\n';
        std::cout << "Frame dump digest: " << display.frame_digest() << '\n';
        return 0;
    }

    if (options.step_one_frame) {
        emulator.step_frame();
    } else if (options.frames_to_run > 0) {
        emulator.run_frames(options.frames_to_run);
    }

    std::cout << "Headless status: deterministic runtime" << '\n';
    if (loaded_rom.has_value()) {
        std::cout << "ROM size: " << emulator.loaded_rom_size() << '\n';
    }
    const auto p1 = emulator.bus().controller_ports().read(core::io::Player::one);
    const auto p2 = emulator.bus().controller_ports().read(core::io::Player::two);
    std::cout << "Controller 1 port: 0x" << std::hex << std::uppercase
              << static_cast<int>(p1) << '\n';
    std::cout << "Controller 2 port: 0x" << std::hex << std::uppercase
              << static_cast<int>(p2) << '\n';
    std::cout << std::dec;
    std::cout << "VCLK: " << core::Emulator::vclk_rate_name(emulator.vclk_rate()) << '\n';
    std::cout << "Frames completed: " << emulator.completed_frames() << '\n';
    std::cout << "Master cycles: " << emulator.master_cycle() << '\n';
    std::cout << "CPU T-states: " << emulator.cpu_tstates() << '\n';
    std::cout << "Event log size: " << emulator.event_log().size() << '\n';
    std::cout << "Event log digest: " << emulator.event_log_digest() << '\n';
    return 0;
}

}  // namespace vanguard8::frontend

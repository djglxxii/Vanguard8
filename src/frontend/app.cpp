#include "frontend/app.hpp"

#include "core/config.hpp"
#include "core/emulator.hpp"
#include "core/logging.hpp"
#include "core/video/compositor.hpp"
#include "debugger/trace_panel.hpp"
#include "frontend/audio_output.hpp"
#include "frontend/display.hpp"
#include "frontend/display_presenter.hpp"
#include "frontend/input.hpp"
#include "frontend/rom_loader.hpp"
#include "frontend/runtime.hpp"
#include "frontend/sdl_window.hpp"
#include "frontend/video_fixture.hpp"

#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <optional>
#include <string>
#include <vector>

namespace vanguard8::frontend {

namespace {

struct RuntimeOptions {
    bool help_requested = false;
    bool debugger_visible = false;
    bool list_recent = false;
    std::optional<std::filesystem::path> rom_path;
    std::optional<std::size_t> recent_index;
    std::optional<int> scale;
    std::optional<std::string> aspect;
    std::optional<bool> fullscreen;
    std::optional<bool> frame_pacing;
    std::vector<std::string> pressed_keys;
    std::vector<std::string> gamepad1_buttons;
    std::vector<std::string> gamepad2_buttons;
};

auto parse_bool_option(const std::string& value) -> bool {
    if (value == "on" || value == "true" || value == "1") {
        return true;
    }
    if (value == "off" || value == "false" || value == "0") {
        return false;
    }
    throw std::invalid_argument("Expected on/off boolean option.");
}

auto parse_options(int argc, char** argv) -> RuntimeOptions {
    RuntimeOptions options;
    for (int index = 1; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "--debugger") {
            options.debugger_visible = true;
            continue;
        }
        if (arg == "--rom" && (index + 1) < argc) {
            options.rom_path = argv[++index];
            continue;
        }
        if (arg == "--drop-rom" && (index + 1) < argc) {
            options.rom_path = argv[++index];
            continue;
        }
        if (arg == "--recent" && (index + 1) < argc) {
            options.recent_index = static_cast<std::size_t>(std::stoull(argv[++index]));
            continue;
        }
        if (arg == "--list-recent") {
            options.list_recent = true;
            continue;
        }
        if (arg == "--scale" && (index + 1) < argc) {
            options.scale = std::stoi(argv[++index]);
            continue;
        }
        if (arg == "--aspect" && (index + 1) < argc) {
            options.aspect = argv[++index];
            continue;
        }
        if (arg == "--fullscreen") {
            options.fullscreen = true;
            continue;
        }
        if (arg == "--windowed") {
            options.fullscreen = false;
            continue;
        }
        if (arg == "--frame-pacing" && (index + 1) < argc) {
            options.frame_pacing = parse_bool_option(argv[++index]);
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

void print_usage() {
    std::cout
        << "Usage: vanguard8_frontend [--rom path] [--drop-rom path] [--recent index] [--list-recent] "
           "[--debugger] [--scale N] [--aspect square|ntsc|stretch] [--fullscreen|--windowed] "
           "[--frame-pacing on|off] [--press-key NAME] [--gamepad1-button NAME] [--gamepad2-button NAME]\n";
}

void apply_config_overrides(core::AppConfig& config, const RuntimeOptions& options) {
    if (options.scale.has_value()) {
        config.display_scale = *options.scale;
    }
    if (options.aspect.has_value()) {
        config.display_aspect = *options.aspect;
    }
    if (options.fullscreen.has_value()) {
        config.fullscreen = *options.fullscreen;
    }
    if (options.frame_pacing.has_value()) {
        config.frame_pacing = *options.frame_pacing;
    }
}

void print_recent_roms(const core::AppConfig& config) {
    std::cout << "Recent ROMs: " << config.recent_roms.size() << '\n';
    for (std::size_t index = 0; index < config.recent_roms.size(); ++index) {
        std::cout << (index + 1) << ". " << config.recent_roms[index] << '\n';
    }
}

void print_runtime_banner(
    const core::Emulator& emulator,
    const core::AppConfig& config,
    const std::optional<LoadedRom>& loaded_rom,
    const bool debugger_visible
) {
    std::cout << emulator.build_summary() << '\n';
    if (loaded_rom.has_value()) {
        std::cout << "Frontend status: desktop runtime with ROM" << '\n';
        std::cout << "ROM path: " << loaded_rom->path.string() << '\n';
        std::cout << "ROM size: " << emulator.loaded_rom_size() << '\n';
    } else {
        std::cout << "Frontend status: desktop runtime without ROM" << '\n';
    }

    std::cout << "Display scale: " << config.display_scale << '\n';
    std::cout << "Display aspect: " << config.display_aspect << '\n';
    std::cout << "Fullscreen: " << std::boolalpha << config.fullscreen << '\n';
    std::cout << "Frame pacing: " << std::boolalpha << config.frame_pacing << '\n';
    std::cout << "Debugger requested: " << std::boolalpha << debugger_visible << '\n';
    std::cout << "Runtime controls: Escape=quit, F11=fullscreen, F10=print status" << '\n';
}

void apply_cli_input(InputManager& input, const RuntimeOptions& options) {
    for (const auto& key : options.pressed_keys) {
        (void)input.press_key(key);
    }
    for (const auto& button : options.gamepad1_buttons) {
        (void)input.press_gamepad_button(0, button);
    }
    for (const auto& button : options.gamepad2_buttons) {
        (void)input.press_gamepad_button(1, button);
    }
}

[[nodiscard]] auto controller_port_hex(const std::uint8_t value) -> std::string {
    std::ostringstream stream;
    stream << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
           << static_cast<int>(value);
    return stream.str();
}

struct RuntimeStatus {
    std::string title_base = "Vanguard 8";
    std::uint64_t frame_count = 0;
    bool fullscreen = false;
};

[[nodiscard]] auto build_window_title(
    const RuntimeStatus& status,
    const core::Emulator& emulator,
    const SdlAudioOutputDevice& audio_output
) -> std::string {
    std::ostringstream stream;
    stream << status.title_base << " | frame " << status.frame_count << " | audio "
           << audio_output.queued_bytes() << " B | P1 "
           << controller_port_hex(emulator.bus().controller_ports().read(core::io::Player::one))
           << " | P2 "
           << controller_port_hex(emulator.bus().controller_ports().read(core::io::Player::two));
    return stream.str();
}

void print_runtime_status(
    const RuntimeStatus& status,
    const core::Emulator& emulator,
    const SdlAudioOutputDevice& audio_output,
    const std::optional<LoadedRom>& loaded_rom
) {
    const auto rom_label =
        loaded_rom.has_value() ? loaded_rom->path.string() : std::string("fixture mode");
    std::cout << debugger::format_trace_runtime_summary(emulator, rom_label);
    std::cout << "Presented frames: " << status.frame_count << '\n';
    std::cout << "Fullscreen: " << std::boolalpha << status.fullscreen << '\n';
    std::cout << "Audio queued bytes: " << audio_output.queued_bytes() << '\n';
    std::cout << "Controller 1 port: "
              << controller_port_hex(emulator.bus().controller_ports().read(core::io::Player::one)) << '\n';
    std::cout << "Controller 2 port: "
              << controller_port_hex(emulator.bus().controller_ports().read(core::io::Player::two)) << '\n';
}

[[nodiscard]] auto apply_runtime_event(
    WindowHost& window_host,
    InputManager& input,
    const RuntimeEvent& event,
    RuntimeStatus& status,
    const core::Emulator& emulator,
    const SdlAudioOutputDevice& audio_output,
    const std::optional<LoadedRom>& loaded_rom
) -> bool {
    switch (event.type) {
    case RuntimeEventType::key_down:
        if (event.control_name == "Escape") {
            return false;
        }
        if (event.control_name == "F11") {
            status.fullscreen = !status.fullscreen;
            window_host.set_fullscreen(status.fullscreen);
            window_host.set_title(build_window_title(status, emulator, audio_output));
            return true;
        }
        if (event.control_name == "F10") {
            print_runtime_status(status, emulator, audio_output, loaded_rom);
            return true;
        }
        (void)input.press_key(event.control_name);
        break;
    case RuntimeEventType::key_up:
        (void)input.release_key(event.control_name);
        break;
    case RuntimeEventType::gamepad_button_down:
        (void)input.press_gamepad_button(event.gamepad_index, event.control_name);
        break;
    case RuntimeEventType::gamepad_button_up:
        (void)input.release_gamepad_button(event.gamepad_index, event.control_name);
        break;
    case RuntimeEventType::quit:
        break;
    }
    return true;
}

}  // namespace

auto run_frontend_app(int argc, char** argv) -> int {
    const auto config_path = core::default_config_path();
    auto config = core::load_or_create_config(config_path);

    RuntimeOptions options;
    try {
        options = parse_options(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "Frontend argument error: " << error.what() << '\n';
        show_frontend_error_dialog("Vanguard 8 Frontend Error", error.what());
        return 2;
    }

    if (options.help_requested) {
        print_usage();
        return 0;
    }

    apply_config_overrides(config, options);
    core::log(core::LogLevel::info, "Launching frontend runtime.");

    if (options.list_recent) {
        print_recent_roms(config);
        return 0;
    }

    core::Emulator emulator;
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
        show_frontend_error_dialog("Vanguard 8 ROM Load Error", error.what());
        return 2;
    }

    if (loaded_rom.has_value()) {
        emulator.load_rom_image(loaded_rom->bytes);
        record_recent_rom(config, loaded_rom->path);
    }

    InputManager input(emulator.mutable_bus().mutable_controller_ports());
    input.configure(config.input);
    input.reset();
    apply_cli_input(input, options);

    save_config(config_path, config);
    print_runtime_banner(emulator, config, loaded_rom, options.debugger_visible);

    SdlWindowHost window_host;
    Display display;
    OpenGlDisplayPresenter presenter;
    SdlAudioOutputDevice audio_output;
    AudioQueuePump audio_queue_pump(16'384);
    RuntimeStatus runtime_status{
        .title_base =
            loaded_rom.has_value() ? std::string("Vanguard 8 - ") + loaded_rom->path.filename().string()
                                   : std::string("Vanguard 8 - Fixture"),
        .fullscreen = config.fullscreen,
    };
    bool fixture_initialized = false;
    const WindowConfig window_config{
        .title = runtime_status.title_base,
        .logical_width = 256,
        .logical_height = 212,
        .scale = config.display_scale,
        .fullscreen = config.fullscreen,
    };

    RuntimeHooks hooks;
    hooks.on_event = [&](const RuntimeEvent& event) {
        return apply_runtime_event(window_host, input, event, runtime_status, emulator, audio_output, loaded_rom);
    };
    hooks.on_started = [&](std::string& error) {
        if (!presenter.initialize(error)) {
            return false;
        }
        if (!audio_output.open(
            AudioDeviceConfig{
                .sample_rate = config.audio_sample_rate,
                .max_queue_bytes = 16'384,
            },
            error
        )) {
            return false;
        }
        window_host.set_title(build_window_title(runtime_status, emulator, audio_output));
        return true;
    };
    hooks.on_frame = [&](std::string& error) {
        if (loaded_rom.has_value()) {
            emulator.run_frames(1);
        } else if (!fixture_initialized) {
            build_dual_vdp_fixture_frame(
                emulator.mutable_bus().mutable_vdp_a(),
                emulator.mutable_bus().mutable_vdp_b()
            );
            fixture_initialized = true;
        }

        display.upload_frame(core::video::Compositor::compose_dual_vdp(emulator.vdp_a(), emulator.vdp_b()));

        int drawable_width = 0;
        int drawable_height = 0;
        window_host.drawable_size(drawable_width, drawable_height);
        if (!presenter.present(display, drawable_width, drawable_height, error)) {
            return false;
        }

        if (!audio_queue_pump.pump(
            audio_output,
            emulator.mutable_bus().mutable_audio_mixer().consume_output_bytes(),
            error
        )) {
            return false;
        }

        ++runtime_status.frame_count;
        if (runtime_status.frame_count == 1 || (runtime_status.frame_count % 60) == 0) {
            window_host.set_title(build_window_title(runtime_status, emulator, audio_output));
        }
        return true;
    };
    hooks.on_shutdown = [&]() {
        audio_output.close();
        presenter.shutdown();
    };

    std::string error;
    const auto result = run_window_runtime(window_host, window_config, hooks, error);
    if (result != 0) {
        std::cerr << "Frontend runtime error: " << error << '\n';
        if (!error.empty()) {
            show_frontend_error_dialog("Vanguard 8 Frontend Error", error);
        }
    }
    return result;
}

}  // namespace vanguard8::frontend

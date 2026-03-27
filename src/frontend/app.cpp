#include "frontend/app.hpp"

#include "core/config.hpp"
#include "core/emulator.hpp"
#include "core/logging.hpp"
#include "core/memory/cartridge.hpp"
#include "core/video/compositor.hpp"
#include "debugger/debugger.hpp"
#include "frontend/display.hpp"
#include "frontend/input.hpp"
#include "frontend/rom_loader.hpp"
#include "frontend/video_fixture.hpp"

#include <exception>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace vanguard8::frontend {

namespace {

struct RuntimeOptions {
    bool help_requested = false;
    bool render_fixture = false;
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
        if (arg == "--video-fixture") {
            options.render_fixture = true;
            continue;
        }
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

}  // namespace

auto run_frontend_app(int argc, char** argv) -> int {
    const auto config_path = core::default_config_path();
    auto config = core::load_or_create_config(config_path);

    RuntimeOptions options;
    try {
        options = parse_options(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "Frontend argument error: " << error.what() << '\n';
        return 2;
    }

    if (options.help_requested) {
        std::cout
            << "Usage: vanguard8_frontend [--rom path] [--drop-rom path] [--recent index] [--list-recent] "
               "[--video-fixture] [--debugger] [--scale N] [--aspect square|ntsc|stretch] [--fullscreen|--windowed] "
               "[--frame-pacing on|off] [--press-key NAME] [--gamepad1-button NAME] [--gamepad2-button NAME]\n";
        return 0;
    }

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

    const core::Emulator emulator_template;
    core::log(core::LogLevel::info, "Launching frontend video path.");
    std::cout << emulator_template.build_summary() << '\n';

    if (options.list_recent) {
        std::cout << "Recent ROMs: " << config.recent_roms.size() << '\n';
        for (std::size_t index = 0; index < config.recent_roms.size(); ++index) {
            std::cout << (index + 1) << ". " << config.recent_roms[index] << '\n';
        }
        return 0;
    }

    core::Emulator emulator;
    debugger::DebuggerShell debugger_shell;
    debugger_shell.set_visible(options.debugger_visible);
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

    save_config(config_path, config);

    if (loaded_rom.has_value() || options.render_fixture) {
        build_dual_vdp_fixture_frame(
            emulator.mutable_bus().mutable_vdp_a(),
            emulator.mutable_bus().mutable_vdp_b()
        );

        Display display;
        display.upload_frame(core::video::Compositor::compose_dual_vdp(emulator.vdp_a(), emulator.vdp_b()));
        debugger_shell.attach(emulator, display);
        const auto& debugger_snapshot = debugger_shell.render();

        if (loaded_rom.has_value()) {
            std::cout << "Frontend status: ROM loaded" << '\n';
            std::cout << "ROM path: " << loaded_rom->path.string() << '\n';
            std::cout << "ROM size: " << emulator.loaded_rom_size() << '\n';
        } else {
            std::cout << "Frontend status: video fixture" << '\n';
        }

        std::cout << "Display scale: " << config.display_scale << '\n';
        std::cout << "Display aspect: " << config.display_aspect << '\n';
        std::cout << "Fullscreen: " << std::boolalpha << config.fullscreen << '\n';
        std::cout << "Frame pacing: " << std::boolalpha << config.frame_pacing << '\n';
        const auto p1 = emulator.bus().controller_ports().read(core::io::Player::one);
        const auto p2 = emulator.bus().controller_ports().read(core::io::Player::two);
        std::cout << "Controller 1 port: 0x" << std::hex << std::uppercase
                  << static_cast<int>(p1) << '\n';
        std::cout << "Controller 2 port: 0x" << std::hex << std::uppercase
                  << static_cast<int>(p2) << '\n';
        std::cout << std::dec << "Recent ROM count: " << config.recent_roms.size() << '\n';
        std::cout << "Frontend frame digest: " << display.frame_digest() << '\n';
        if (debugger_snapshot.debugger_visible) {
            const auto cpu_snapshot = debugger_shell.cpu_panel().snapshot(emulator);
            const auto memory_snapshot = debugger_shell.memory_panel().snapshot(
                emulator,
                debugger::MemorySelection{
                    .region = debugger::MemoryRegion::logical,
                    .start = cpu_snapshot.cpu.registers.pc,
                    .length = 8,
                }
            );
            const auto vdp_snapshot = debugger_shell.vdp_panel().snapshot(emulator, debugger::VdpTarget::a);
            const auto interrupt_snapshot = debugger_shell.interrupt_panel().snapshot(emulator);
            const auto bank_snapshot = debugger_shell.bank_panel().snapshot(emulator);
            std::cout << "Debugger status: "
                      << (debugger_snapshot.rendered ? "rendered" : "hidden")
                      << '\n';
            std::cout << "Debugger dockspace: " << debugger_shell.layout().dockspace_id << '\n';
            std::cout << "Debugger visible panels: " << debugger_snapshot.visible_panel_count << '\n';
            std::cout << "Debugger CPU PC: 0x" << std::hex << std::uppercase
                      << cpu_snapshot.cpu.registers.pc << '\n';
            std::cout << "Debugger disassembly: " << cpu_snapshot.disassembly.front().mnemonic << '\n';
            std::cout << std::dec << "Debugger logical bytes: " << memory_snapshot.bytes.size() << '\n';
            std::cout << "Debugger VDP-A CE: "
                      << (((vdp_snapshot.status[2] & 0x01U) != 0U) ? "on" : "off") << '\n';
            std::cout << "Debugger interrupts: " << interrupt_snapshot.size() << '\n';
            std::cout << "Debugger bank switches: " << bank_snapshot.size() << '\n';
        }
        return 0;
    }

    debugger_shell.attach(emulator);
    const auto& debugger_snapshot = debugger_shell.render();
    std::cout << "Frontend status: ROM workflow and controller mapping available" << '\n';
    std::cout << "Display scale: " << config.display_scale << '\n';
    std::cout << "Display aspect: " << config.display_aspect << '\n';
    std::cout << "Fullscreen: " << std::boolalpha << config.fullscreen << '\n';
    std::cout << "Frame pacing: " << std::boolalpha << config.frame_pacing << '\n';
    if (debugger_snapshot.debugger_visible) {
        const auto cpu_snapshot = debugger_shell.cpu_panel().snapshot(emulator);
        const auto bank_snapshot = debugger_shell.bank_panel().snapshot(emulator);
        std::cout << "Debugger status: "
                  << (debugger_snapshot.rendered ? "rendered" : "hidden")
                  << '\n';
        std::cout << "Debugger dockspace: " << debugger_shell.layout().dockspace_id << '\n';
        std::cout << "Debugger visible panels: " << debugger_snapshot.visible_panel_count << '\n';
        std::cout << "Debugger CPU PC: 0x" << std::hex << std::uppercase
                  << cpu_snapshot.cpu.registers.pc << '\n';
        std::cout << "Debugger disassembly: " << cpu_snapshot.disassembly.front().mnemonic << '\n';
        std::cout << std::dec << "Debugger bank switches: " << bank_snapshot.size() << '\n';
    }
    return 0;
}

}  // namespace vanguard8::frontend

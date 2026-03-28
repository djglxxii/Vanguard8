#include "frontend/headless.hpp"

#include "core/config.hpp"
#include "core/emulator.hpp"
#include "core/hash.hpp"
#include "core/logging.hpp"
#include "core/replay/replayer.hpp"
#include "core/save_state.hpp"
#include "core/symbols.hpp"
#include "core/video/compositor.hpp"
#include "debugger/trace_panel.hpp"
#include "frontend/display.hpp"
#include "frontend/input.hpp"
#include "frontend/rom_loader.hpp"
#include "frontend/video_fixture.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
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
    std::optional<std::filesystem::path> replay_path;
    std::optional<std::uint64_t> hash_frame;
    std::optional<std::pair<std::uint64_t, std::string>> expect_frame_hash;
    bool hash_audio = false;
    std::optional<std::string> expect_audio_hash;
    std::optional<std::filesystem::path> trace_path;
    std::size_t trace_instructions = 256;
    std::optional<std::filesystem::path> symbol_path;
    std::vector<std::string> pressed_keys;
    std::vector<std::string> gamepad1_buttons;
    std::vector<std::string> gamepad2_buttons;
};

enum class ExitCode : int {
    success = 0,
    hash_mismatch = 1,
    replay_rom_mismatch = 2,
    rom_load_failure = 3,
    emulator_error = 4,
};

auto read_binary_file(const std::filesystem::path& path) -> std::vector<std::uint8_t> {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Unable to open binary file.");
    }
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
}

auto digest_hex(const core::Sha256Digest& digest) -> std::string {
    std::ostringstream stream;
    stream << std::hex << std::nouppercase << std::setfill('0');
    for (const auto byte : digest) {
        stream << std::setw(2) << static_cast<int>(byte);
    }
    return stream.str();
}

auto normalize_hex(std::string value) -> std::string {
    for (auto& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

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
        if (arg == "--replay" && (index + 1) < argc) {
            options.replay_path = argv[++index];
            continue;
        }
        if (arg == "--recent" && (index + 1) < argc) {
            options.recent_index = static_cast<std::size_t>(std::stoull(argv[++index]));
            continue;
        }
        if (arg == "--hash-frame" && (index + 1) < argc) {
            options.hash_frame = std::stoull(argv[++index]);
            continue;
        }
        if (arg == "--expect-frame-hash" && (index + 2) < argc) {
            const auto frame = std::stoull(argv[++index]);
            options.expect_frame_hash = std::make_pair(frame, std::string(argv[++index]));
            continue;
        }
        if (arg == "--hash-audio") {
            options.hash_audio = true;
            continue;
        }
        if (arg == "--trace" && (index + 1) < argc) {
            options.trace_path = argv[++index];
            continue;
        }
        if (arg == "--trace-instructions" && (index + 1) < argc) {
            options.trace_instructions = static_cast<std::size_t>(std::stoull(argv[++index]));
            continue;
        }
        if (arg == "--symbols" && (index + 1) < argc) {
            options.symbol_path = argv[++index];
            continue;
        }
        if (arg == "--expect-audio-hash" && (index + 1) < argc) {
            options.expect_audio_hash = std::string(argv[++index]);
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
        throw std::invalid_argument("Unsupported headless option: " + arg);
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
        return static_cast<int>(ExitCode::emulator_error);
    }

    if (options.help_requested) {
        std::cout
            << "Usage: vanguard8_headless [--rom path] [--recent index] [--frames N] [--paused] "
               "[--step-frame] [--replay file.v8r] [--vclk off|4000|6000|8000] [--dump-frame path.ppm] "
               "[--hash-frame N] [--expect-frame-hash N HASH] [--hash-audio] [--expect-audio-hash HASH] "
               "[--trace path.log] [--trace-instructions N] [--symbols path.sym] "
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
        return static_cast<int>(ExitCode::rom_load_failure);
    }

    if (loaded_rom.has_value()) {
        emulator.load_rom_image(loaded_rom->bytes);
        record_recent_rom(config, loaded_rom->path);
        save_config(config_path, config);
    }

    std::optional<core::replay::Replayer> replayer;
    if (options.replay_path.has_value()) {
        try {
            replayer.emplace();
            replayer->load(read_binary_file(*options.replay_path));
        } catch (const std::exception& error) {
            std::cerr << "Replay load error: " << error.what() << '\n';
            return static_cast<int>(ExitCode::emulator_error);
        }

        if (!loaded_rom.has_value() || !replayer->rom_matches(loaded_rom->bytes)) {
            std::cerr << "Replay ROM mismatch." << '\n';
            return static_cast<int>(ExitCode::replay_rom_mismatch);
        }

        if (replayer->recording().anchor_type == core::replay::AnchorType::embedded_save_state) {
            try {
                core::SaveState::load(emulator, replayer->recording().anchor_save_state);
            } catch (const std::exception& error) {
                std::cerr << "Replay anchor load error: " << error.what() << '\n';
                return static_cast<int>(ExitCode::emulator_error);
            }
        }
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

    if (options.trace_path.has_value()) {
        debugger::TracePanel trace_panel;
        core::SymbolTable symbols;
        try {
            if (options.symbol_path.has_value()) {
                symbols.load_from_file(*options.symbol_path);
            } else if (loaded_rom.has_value()) {
                auto auto_symbol_path = loaded_rom->path;
                auto_symbol_path.replace_extension(".sym");
                if (std::filesystem::exists(auto_symbol_path)) {
                    symbols.load_from_file(auto_symbol_path);
                    options.symbol_path = auto_symbol_path;
                }
            }

            const auto result =
                trace_panel.write_to_file(
                    emulator,
                    *options.trace_path,
                    options.trace_instructions,
                    symbols.empty() ? nullptr : &symbols
                );
            const auto rom_label =
                loaded_rom.has_value() ? loaded_rom->path.string() : std::string("idle ROM");
            std::cout << debugger::format_trace_runtime_summary(emulator, rom_label);
            std::cout << "Trace file: " << options.trace_path->string() << '\n';
            if (options.symbol_path.has_value()) {
                std::cout << "Symbol file: " << options.symbol_path->string() << '\n';
                std::cout << "Symbol count: " << symbols.size() << '\n';
            }
            std::cout << "Trace lines written: " << result.line_count << '\n';
            std::cout << "CPU halted: " << std::boolalpha << result.halted << '\n';
        } catch (const std::exception& error) {
            std::cerr << "Trace write error: " << error.what() << '\n';
            return static_cast<int>(ExitCode::emulator_error);
        }
        return static_cast<int>(ExitCode::success);
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

    const auto frame_hash_target = options.expect_frame_hash.has_value()
                                       ? std::optional<std::uint64_t>(options.expect_frame_hash->first)
                                       : options.hash_frame;
    std::vector<std::uint8_t> hashed_frame;
    const auto replay_frames = replayer.has_value() ? static_cast<std::uint64_t>(replayer->frame_count()) : 0U;
    std::uint64_t session_frames_to_run = options.step_one_frame ? 1U : options.frames_to_run;
    if (replayer.has_value()) {
        session_frames_to_run = session_frames_to_run == 0U ? replay_frames : std::min(session_frames_to_run, replay_frames);
    }
    if (frame_hash_target.has_value()) {
        session_frames_to_run = std::max(session_frames_to_run, *frame_hash_target);
    }

    for (std::uint64_t frame = 0; frame < session_frames_to_run; ++frame) {
        if (replayer.has_value() && frame < replay_frames) {
            replayer->apply_frame(emulator.mutable_bus().mutable_controller_ports(), static_cast<std::uint32_t>(frame));
        }
        emulator.run_frames(1);
        if (frame_hash_target.has_value() && (frame + 1U) == *frame_hash_target) {
            hashed_frame = core::video::Compositor::compose_dual_vdp(emulator.vdp_a(), emulator.vdp_b());
        }
    }

    if (replayer.has_value() && session_frames_to_run != replay_frames && options.frames_to_run == 0U) {
        std::cerr << "Replay did not run to completion." << '\n';
        return static_cast<int>(ExitCode::emulator_error);
    }

    if (frame_hash_target.has_value() && hashed_frame.empty()) {
        std::cerr << "Requested frame hash target was not reached." << '\n';
        return static_cast<int>(ExitCode::emulator_error);
    }

    const auto frame_hash_hex =
        frame_hash_target.has_value() ? digest_hex(core::sha256(hashed_frame)) : std::string{};
    const auto audio_hash_hex =
        (options.hash_audio || options.expect_audio_hash.has_value())
            ? digest_hex(core::sha256(emulator.bus().audio_mixer().output_bytes()))
            : std::string{};

    if (options.expect_frame_hash.has_value() &&
        normalize_hex(options.expect_frame_hash->second) != normalize_hex(frame_hash_hex)) {
        std::cerr << "Frame hash mismatch." << '\n';
        return static_cast<int>(ExitCode::hash_mismatch);
    }

    if (options.expect_audio_hash.has_value() &&
        normalize_hex(*options.expect_audio_hash) != normalize_hex(audio_hash_hex)) {
        std::cerr << "Audio hash mismatch." << '\n';
        return static_cast<int>(ExitCode::hash_mismatch);
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
    if (frame_hash_target.has_value()) {
        std::cout << "Frame SHA-256 (" << *frame_hash_target << "): " << frame_hash_hex << '\n';
    }
    if (options.hash_audio || options.expect_audio_hash.has_value()) {
        std::cout << "Audio SHA-256: " << audio_hash_hex << '\n';
    }
    return 0;
}

}  // namespace vanguard8::frontend

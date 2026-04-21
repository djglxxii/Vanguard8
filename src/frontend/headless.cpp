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
#include "frontend/headless_inspect.hpp"
#include "frontend/input.hpp"
#include "frontend/rom_loader.hpp"
#include "frontend/video_fixture.hpp"

#include <algorithm>
#include <cctype>
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

struct RunUntilPcOption {
    std::uint16_t pc = 0;
    std::optional<std::uint64_t> max_frames;
};

struct RuntimeOptions {
    static constexpr std::size_t max_peek_length = 256;

    bool help_requested = false;
    std::uint64_t frames_to_run = 0;
    bool start_paused = false;
    bool step_one_frame = false;
    core::VclkRate vclk_rate = core::VclkRate::stopped;
    std::optional<std::filesystem::path> dump_frame_path;
    std::optional<std::filesystem::path> dump_fixture_path;
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
    std::vector<HeadlessMemoryRange> peek_mem_ranges;
    std::vector<HeadlessMemoryRange> peek_logical_ranges;
    std::optional<std::filesystem::path> dump_vram_a_path;
    std::optional<std::filesystem::path> dump_vram_b_path;
    bool dump_vdp_regs = false;
    bool dump_cpu = false;
    std::optional<std::uint64_t> inspect_frame;
    std::optional<std::filesystem::path> inspect_path;
    std::optional<RunUntilPcOption> run_until_pc;
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

auto parse_hex_u64(const std::string& value, const char* label) -> std::uint64_t {
    std::size_t parsed = 0;
    const auto parsed_value = std::stoull(value, &parsed, 16);
    if (parsed != value.size()) {
        throw std::invalid_argument(std::string("Invalid hex value for ") + label + ": " + value);
    }
    return parsed_value;
}

auto parse_decimal_u64(const std::string& value, const char* label) -> std::uint64_t {
    std::size_t parsed = 0;
    const auto parsed_value = std::stoull(value, &parsed, 10);
    if (parsed != value.size()) {
        throw std::invalid_argument(std::string("Invalid decimal value for ") + label + ": " + value);
    }
    return parsed_value;
}

auto parse_length_u64(const std::string& value, const char* label) -> std::uint64_t {
    std::size_t parsed = 0;
    const int base = value.rfind("0x", 0) == 0 || value.rfind("0X", 0) == 0 ? 16 : 10;
    const auto parsed_value = std::stoull(value, &parsed, base);
    if (parsed != value.size()) {
        throw std::invalid_argument(std::string("Invalid length for ") + label + ": " + value);
    }
    return parsed_value;
}

auto parse_memory_range(const std::string& value, const bool logical) -> HeadlessMemoryRange {
    const auto separator = value.find(':');
    const auto address_text = separator == std::string::npos ? value : value.substr(0, separator);
    const auto length_text = separator == std::string::npos ? std::string{} : value.substr(separator + 1);
    if (address_text.empty()) {
        throw std::invalid_argument("Memory peek address is empty.");
    }
    const auto address = parse_hex_u64(address_text, logical ? "--peek-logical" : "--peek-mem");
    const auto max_address = logical ? 0xFFFFULL : 0xFFFFFULL;
    if (address > max_address) {
        throw std::invalid_argument("Memory peek address is out of range.");
    }
    std::uint64_t length = 1;
    if (!length_text.empty()) {
        length = parse_length_u64(length_text, logical ? "--peek-logical" : "--peek-mem");
    }
    if (length == 0U) {
        throw std::invalid_argument("Memory peek length must be at least 1.");
    }
    length = std::min<std::uint64_t>(length, RuntimeOptions::max_peek_length);
    return HeadlessMemoryRange{
        .address = static_cast<std::uint32_t>(address),
        .length = static_cast<std::size_t>(length),
    };
}

auto parse_run_until_pc(const std::string& value) -> RunUntilPcOption {
    const auto separator = value.find(':');
    const auto pc_text = separator == std::string::npos ? value : value.substr(0, separator);
    const auto max_frames_text = separator == std::string::npos ? std::string{} : value.substr(separator + 1);
    const auto pc = parse_hex_u64(pc_text, "--run-until-pc");
    if (pc > 0xFFFFULL) {
        throw std::invalid_argument("--run-until-pc target is outside the 16-bit PC range.");
    }
    return RunUntilPcOption{
        .pc = static_cast<std::uint16_t>(pc),
        .max_frames = max_frames_text.empty()
                          ? std::optional<std::uint64_t>{}
                          : std::optional<std::uint64_t>{parse_decimal_u64(max_frames_text, "--run-until-pc")},
    };
}

auto has_observability_output(const RuntimeOptions& options) -> bool {
    return !options.peek_mem_ranges.empty() || !options.peek_logical_ranges.empty() ||
           options.dump_vram_a_path.has_value() || options.dump_vram_b_path.has_value() ||
           options.dump_vdp_regs || options.dump_cpu || options.inspect_path.has_value();
}

struct FrameDumpSummary {
    std::uint64_t digest = 0;
    int width = 0;
    int height = 0;
};

auto dump_rgb_frame(
    const std::vector<std::uint8_t>& rgb_frame,
    const std::filesystem::path& output_path
) -> FrameDumpSummary {
    Display display;
    display.upload_frame(rgb_frame);
    display.dump_ppm_file(output_path);
    return FrameDumpSummary{
        .digest = display.frame_digest(),
        .width = display.frame_width(),
        .height = display.uploaded_frame_height(),
    };
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
            options.dump_frame_path = argv[++index];
            continue;
        }
        if (arg == "--dump-fixture" && (index + 1) < argc) {
            options.dump_fixture_path = argv[++index];
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
        if (arg == "--peek-mem" && (index + 1) < argc) {
            options.peek_mem_ranges.push_back(parse_memory_range(argv[++index], false));
            continue;
        }
        if (arg == "--peek-logical" && (index + 1) < argc) {
            options.peek_logical_ranges.push_back(parse_memory_range(argv[++index], true));
            continue;
        }
        if (arg == "--dump-vram-a" && (index + 1) < argc) {
            options.dump_vram_a_path = argv[++index];
            continue;
        }
        if (arg == "--dump-vram-b" && (index + 1) < argc) {
            options.dump_vram_b_path = argv[++index];
            continue;
        }
        if (arg == "--dump-vdp-regs") {
            options.dump_vdp_regs = true;
            continue;
        }
        if (arg == "--dump-cpu") {
            options.dump_cpu = true;
            continue;
        }
        if (arg == "--inspect-frame" && (index + 1) < argc) {
            options.inspect_frame = parse_decimal_u64(argv[++index], "--inspect-frame");
            continue;
        }
        if (arg == "--inspect" && (index + 1) < argc) {
            options.inspect_path = argv[++index];
            continue;
        }
        if (arg == "--run-until-pc" && (index + 1) < argc) {
            options.run_until_pc = parse_run_until_pc(argv[++index]);
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
    if (options.dump_frame_path.has_value() && options.dump_fixture_path.has_value()) {
        throw std::invalid_argument("Use either --dump-frame or --dump-fixture, not both.");
    }
    if (options.dump_fixture_path.has_value() && has_observability_output(options)) {
        throw std::invalid_argument("Headless inspection flags require the runtime path, not --dump-fixture.");
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
               "[--dump-fixture path.ppm] "
               "[--hash-frame N] [--expect-frame-hash N HASH] [--hash-audio] [--expect-audio-hash HASH] "
               "[--trace path.log] [--trace-instructions N] [--symbols path.sym] "
               "[--inspect path.txt] [--inspect-frame N] [--peek-mem ADDR[:LEN]] "
               "[--peek-logical ADDR[:LEN]] [--dump-cpu] [--dump-vdp-regs] "
               "[--dump-vram-a path.bin] [--dump-vram-b path.bin] [--run-until-pc HEX[:MAX_FRAMES]] "
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

    if (options.dump_fixture_path.has_value()) {
        build_dual_vdp_fixture_frame(
            emulator.mutable_bus().mutable_vdp_a(),
            emulator.mutable_bus().mutable_vdp_b()
        );
        const auto dump = dump_rgb_frame(
            core::video::Compositor::compose_dual_vdp(emulator.vdp_a(), emulator.vdp_b()),
            *options.dump_fixture_path
        );
        std::cout << "Headless status: fixture frame dump complete" << '\n';
        const auto p1 = emulator.bus().controller_ports().read(core::io::Player::one);
        const auto p2 = emulator.bus().controller_ports().read(core::io::Player::two);
        std::cout << "Controller 1 port: 0x" << std::hex << std::uppercase
                  << static_cast<int>(p1) << '\n';
        std::cout << "Controller 2 port: 0x" << std::hex << std::uppercase
                  << static_cast<int>(p2) << '\n';
        std::cout << std::dec;
        std::cout << "Frame dump source: fixture" << '\n';
        std::cout << "Frame dump path: " << options.dump_fixture_path->string() << '\n';
        std::cout << "Frame dump size: " << dump.width << "x" << dump.height << '\n';
        std::cout << "Frame dump digest: " << dump.digest << '\n';
        return static_cast<int>(ExitCode::success);
    }

    const auto frame_hash_target = options.expect_frame_hash.has_value()
                                       ? std::optional<std::uint64_t>(options.expect_frame_hash->first)
                                       : options.hash_frame;
    std::vector<std::uint8_t> hashed_frame;
    const auto replay_frames = replayer.has_value() ? static_cast<std::uint64_t>(replayer->frame_count()) : 0U;
    std::uint64_t session_frames_to_run = options.step_one_frame ? 1U : options.frames_to_run;
    if (options.run_until_pc.has_value()) {
        session_frames_to_run = options.run_until_pc->max_frames.value_or(options.frames_to_run);
    }
    if (replayer.has_value()) {
        session_frames_to_run = session_frames_to_run == 0U ? replay_frames : std::min(session_frames_to_run, replay_frames);
    }
    if (frame_hash_target.has_value()) {
        session_frames_to_run = std::max(session_frames_to_run, *frame_hash_target);
    }
    if (options.inspect_frame.has_value()) {
        session_frames_to_run = std::max(session_frames_to_run, *options.inspect_frame);
    }

    HeadlessRunUntilPcSummary run_until_pc_summary;
    if (options.run_until_pc.has_value()) {
        run_until_pc_summary.requested = true;
        run_until_pc_summary.target_pc = options.run_until_pc->pc;
        run_until_pc_summary.max_frames = options.run_until_pc->max_frames.value_or(options.frames_to_run);
        emulator.mutable_cpu().clear_breakpoint_hits();
        (void)emulator.mutable_cpu().add_breakpoint(core::cpu::Breakpoint{
            .type = core::cpu::BreakpointType::pc,
            .address = options.run_until_pc->pc,
        });
    }

    std::optional<std::string> inspection_report;
    std::vector<HeadlessVramDumpSummary> vram_dump_summaries;
    auto perform_inspection = [&](const std::uint64_t frame) {
        vram_dump_summaries.clear();
        if (options.dump_vram_a_path.has_value()) {
            vram_dump_summaries.push_back(
                dump_headless_vram(emulator.vdp_a(), 'a', *options.dump_vram_a_path)
            );
        }
        if (options.dump_vram_b_path.has_value()) {
            vram_dump_summaries.push_back(
                dump_headless_vram(emulator.vdp_b(), 'b', *options.dump_vram_b_path)
            );
        }
        const auto include_default_register_blocks =
            options.inspect_path.has_value() && !options.dump_cpu && !options.dump_vdp_regs;
        HeadlessInspectionConfig config{
            .frame = frame,
            .dump_cpu = options.dump_cpu || include_default_register_blocks,
            .dump_vdp_regs = options.dump_vdp_regs || include_default_register_blocks,
            .physical_peeks = options.peek_mem_ranges,
            .logical_peeks = options.peek_logical_ranges,
            .vram_dumps = vram_dump_summaries,
        };
        inspection_report = format_headless_inspection_report(emulator, config, run_until_pc_summary);
    };

    for (std::uint64_t frame = 0; frame < session_frames_to_run; ++frame) {
        if (replayer.has_value() && frame < replay_frames) {
            replayer->apply_frame(emulator.mutable_bus().mutable_controller_ports(), static_cast<std::uint32_t>(frame));
        }
        emulator.run_frames(1);
        if (frame_hash_target.has_value() && (frame + 1U) == *frame_hash_target) {
            hashed_frame = core::video::Compositor::compose_dual_vdp(emulator.vdp_a(), emulator.vdp_b());
        }
        const auto pc_at_frame_end = emulator.cpu().state_snapshot().registers.pc;
        if (options.run_until_pc.has_value() && !run_until_pc_summary.hit &&
            (!emulator.cpu().breakpoint_hits().empty() || pc_at_frame_end == options.run_until_pc->pc)) {
            run_until_pc_summary.hit = true;
            run_until_pc_summary.frame = frame + 1U;
            run_until_pc_summary.master_cycle = emulator.master_cycle();
            if (!options.inspect_frame.has_value()) {
                if (has_observability_output(options)) {
                    perform_inspection(frame + 1U);
                }
                break;
            }
        }
        if (options.inspect_frame.has_value() && has_observability_output(options) &&
            (frame + 1U) == *options.inspect_frame) {
            if (options.run_until_pc.has_value() && !run_until_pc_summary.hit) {
                run_until_pc_summary.frame = frame + 1U;
                run_until_pc_summary.master_cycle = emulator.master_cycle();
            }
            perform_inspection(frame + 1U);
        }
    }

    if (options.run_until_pc.has_value() && !run_until_pc_summary.hit) {
        run_until_pc_summary.frame = emulator.completed_frames();
        run_until_pc_summary.master_cycle = emulator.master_cycle();
    }

    if (has_observability_output(options) && !inspection_report.has_value()) {
        perform_inspection(emulator.completed_frames());
    }

    if (options.inspect_path.has_value() && inspection_report.has_value()) {
        std::ofstream output(*options.inspect_path);
        if (!output) {
            std::cerr << "Inspection write error: unable to open report path." << '\n';
            return static_cast<int>(ExitCode::emulator_error);
        }
        output << *inspection_report;
        if (!output) {
            std::cerr << "Inspection write error: unable to write report." << '\n';
            return static_cast<int>(ExitCode::emulator_error);
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
    std::optional<FrameDumpSummary> runtime_dump;

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

    if (options.dump_frame_path.has_value()) {
        runtime_dump = dump_rgb_frame(
            core::video::Compositor::compose_dual_vdp(emulator.vdp_a(), emulator.vdp_b()),
            *options.dump_frame_path
        );
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
    if (runtime_dump.has_value()) {
        std::cout << "Frame dump source: runtime" << '\n';
        std::cout << "Frame dump path: " << options.dump_frame_path->string() << '\n';
        std::cout << "Frame dump size: " << runtime_dump->width << "x" << runtime_dump->height << '\n';
        std::cout << "Frame dump digest: " << runtime_dump->digest << '\n';
    }
    if (options.run_until_pc.has_value()) {
        std::cout << "Run-until-PC target: 0x" << std::hex << std::nouppercase << std::setfill('0')
                  << std::setw(4) << run_until_pc_summary.target_pc << std::dec << '\n';
        std::cout << "Run-until-PC result: " << (run_until_pc_summary.hit ? "hit" : "not-hit") << '\n';
        std::cout << "Run-until-PC frame: " << run_until_pc_summary.frame << '\n';
        std::cout << "Run-until-PC master cycle: " << run_until_pc_summary.master_cycle << '\n';
    }
    for (const auto& dump : vram_dump_summaries) {
        std::cout << "VDP-" << static_cast<char>(std::toupper(static_cast<unsigned char>(dump.vdp)))
                  << " VRAM dump path: "
                  << (dump.vdp == 'a' && options.dump_vram_a_path.has_value()
                          ? options.dump_vram_a_path->string()
                          : options.dump_vram_b_path.has_value() ? options.dump_vram_b_path->string()
                                                                 : dump.path.string())
                  << '\n';
        std::cout << "VDP-" << static_cast<char>(std::toupper(static_cast<unsigned char>(dump.vdp)))
                  << " VRAM SHA-256: " << dump.sha256 << '\n';
    }
    if (!options.inspect_path.has_value() && inspection_report.has_value()) {
        std::cout << *inspection_report;
    }
    if (options.inspect_path.has_value()) {
        std::cout << "Inspection report path: " << options.inspect_path->string() << '\n';
    }
    return static_cast<int>(ExitCode::success);
}

}  // namespace vanguard8::frontend

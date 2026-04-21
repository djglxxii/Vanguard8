#include <catch2/catch_test_macros.hpp>

#include "core/hash.hpp"
#include "core/emulator.hpp"
#include "core/video/compositor.hpp"
#include "core/video/v9938.hpp"
#include "frontend/headless.hpp"
#include "frontend/video_fixture.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace {

using vanguard8::core::Emulator;
using vanguard8::core::video::Compositor;
using vanguard8::core::video::V9938;

struct ArgvBuilder {
    std::vector<std::string> storage;
    std::vector<char*> argv;

    explicit ArgvBuilder(std::initializer_list<std::string> args) : storage(args) {
        argv.reserve(storage.size());
        for (auto& arg : storage) {
            argv.push_back(arg.data());
        }
    }
};

class ScopedEnvVar {
  public:
    ScopedEnvVar(std::string name, std::string value) : name_(std::move(name)) {
        if (const char* existing = std::getenv(name_.c_str())) {
            old_value_ = std::string(existing);
        }
        setenv(name_.c_str(), value.c_str(), 1);
    }

    ~ScopedEnvVar() {
        if (old_value_.has_value()) {
            setenv(name_.c_str(), old_value_->c_str(), 1);
        } else {
            unsetenv(name_.c_str());
        }
    }

  private:
    std::string name_;
    std::optional<std::string> old_value_;
};

class ScopedStreamRedirect {
  public:
    ScopedStreamRedirect(std::ostream& stream, std::streambuf* replacement)
        : stream_(stream), old_(stream.rdbuf(replacement)) {}

    ~ScopedStreamRedirect() { stream_.rdbuf(old_); }

  private:
    std::ostream& stream_;
    std::streambuf* old_ = nullptr;
};

struct ParsedPpm {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> pixels;
};

auto make_temp_dir() -> std::filesystem::path {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path =
        std::filesystem::temp_directory_path() / ("vanguard8-headless-" + std::to_string(stamp));
    std::filesystem::create_directories(path);
    return path;
}

void write_binary_file(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes) {
    std::ofstream output(path, std::ios::binary);
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

auto read_text_file(const std::filesystem::path& path) -> std::string {
    std::ifstream input(path);
    return std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
}

auto read_binary_file(const std::filesystem::path& path) -> std::vector<std::uint8_t> {
    std::ifstream input(path, std::ios::binary);
    return std::vector<std::uint8_t>(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>()
    );
}

auto parse_ppm(const std::filesystem::path& path) -> ParsedPpm {
    const auto bytes = read_binary_file(path);
    REQUIRE(bytes.size() > 3);

    const auto first_nl = std::find(bytes.begin(), bytes.end(), static_cast<std::uint8_t>('\n'));
    REQUIRE(first_nl != bytes.end());
    const auto second_nl = std::find(first_nl + 1, bytes.end(), static_cast<std::uint8_t>('\n'));
    REQUIRE(second_nl != bytes.end());
    const auto third_nl = std::find(second_nl + 1, bytes.end(), static_cast<std::uint8_t>('\n'));
    REQUIRE(third_nl != bytes.end());

    const std::string magic(bytes.begin(), first_nl);
    const std::string dims(first_nl + 1, second_nl);
    const std::string maxv(second_nl + 1, third_nl);
    REQUIRE(magic == "P6");
    REQUIRE(maxv == "255");

    ParsedPpm ppm;
    const auto separator = dims.find(' ');
    REQUIRE(separator != std::string::npos);
    ppm.width = std::stoi(dims.substr(0, separator));
    ppm.height = std::stoi(dims.substr(separator + 1));
    ppm.pixels.assign(third_nl + 1, bytes.end());
    return ppm;
}

auto make_frame_loop_rom() -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);

    rom[0x0000] = 0xF3;
    rom[0x0001] = 0x3E; rom[0x0002] = 0x01;
    rom[0x0003] = 0xD3; rom[0x0004] = 0x82;
    rom[0x0005] = 0x3E; rom[0x0006] = 0x07;
    rom[0x0007] = 0xD3; rom[0x0008] = 0x82;
    rom[0x0009] = 0x3E; rom[0x000A] = 0x00;
    rom[0x000B] = 0xD3; rom[0x000C] = 0x82;
    rom[0x000D] = 0x3E; rom[0x000E] = 0x01;
    rom[0x000F] = 0xD3; rom[0x0010] = 0x81;
    rom[0x0011] = 0x3E; rom[0x0012] = 0x87;
    rom[0x0013] = 0xD3; rom[0x0014] = 0x81;
    rom[0x0015] = 0xC3; rom[0x0016] = 0x15; rom[0x0017] = 0x00;

    return rom;
}

auto make_graphic6_runtime_rom() -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);
    std::size_t pc = 0;

    auto emit = [&](const std::initializer_list<std::uint8_t> bytes) {
        for (const auto byte : bytes) {
            rom[pc++] = byte;
        }
    };
    auto emit_out = [&](const std::uint8_t port, const std::uint8_t value) {
        emit({0x3E, value, 0xD3, port});
    };
    auto emit_vdp_register_write = [&](const std::uint8_t control_port,
                                       const std::uint8_t reg,
                                       const std::uint8_t value) {
        emit_out(control_port, value);
        emit_out(control_port, static_cast<std::uint8_t>(0x80U | reg));
    };
    auto emit_vram_write = [&](const std::uint8_t control_port,
                               const std::uint8_t data_port,
                               const std::uint16_t address,
                               const std::uint8_t value) {
        emit_out(control_port, static_cast<std::uint8_t>(address & 0x00FFU));
        emit_out(control_port, static_cast<std::uint8_t>(0x40U | ((address >> 8U) & 0x3FU)));
        emit_out(data_port, value);
    };

    emit({0xF3});
    emit_vdp_register_write(0x81, 0, V9938::graphic6_mode_r0);
    emit_vdp_register_write(0x81, 1, static_cast<std::uint8_t>(V9938::graphic_mode_r1 | 0x40U));
    emit_vdp_register_write(0x81, 8, 0x20);
    emit_out(0x82, 0x01);
    emit_out(0x82, 0x70);
    emit_out(0x82, 0x00);
    emit_out(0x82, 0x02);
    emit_out(0x82, 0x07);
    emit_out(0x82, 0x00);
    emit_vram_write(0x81, 0x80, 0x0000, 0x12);
    emit({0xC3, static_cast<std::uint8_t>(pc & 0x00FFU), static_cast<std::uint8_t>(pc >> 8U)});

    return rom;
}

struct ObservabilityRom {
    std::vector<std::uint8_t> bytes;
    std::uint16_t loop_pc = 0;
};

auto make_observability_rom() -> ObservabilityRom {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);
    std::size_t pc = 0;

    auto emit = [&](const std::initializer_list<std::uint8_t> bytes) {
        for (const auto byte : bytes) {
            rom[pc++] = byte;
        }
    };
    auto emit_out0_a = [&](const std::uint8_t port, const std::uint8_t value) {
        emit({0x3E, value, 0xED, 0x39, port});
    };
    auto emit_ld_mem_a = [&](const std::uint16_t address, const std::uint8_t value) {
        emit({0x3E, value, 0x32, static_cast<std::uint8_t>(address & 0xFFU), static_cast<std::uint8_t>(address >> 8U)});
    };
    auto emit_out = [&](const std::uint8_t port, const std::uint8_t value) {
        emit({0x3E, value, 0xD3, port});
    };
    auto emit_vram_write = [&](const std::uint8_t control_port,
                               const std::uint8_t data_port,
                               const std::uint16_t address,
                               const std::uint8_t value) {
        emit_out(control_port, static_cast<std::uint8_t>(address & 0xFFU));
        emit_out(control_port, static_cast<std::uint8_t>(0x40U | ((address >> 8U) & 0x3FU)));
        emit_out(data_port, value);
    };

    emit({0xF3});  // DI
    emit_out0_a(0x3A, 0x48);
    emit_out0_a(0x38, 0xF0);
    emit_out0_a(0x39, 0x04);
    emit_ld_mem_a(0x8250, 0x12);
    emit_ld_mem_a(0x8251, 0x34);
    emit_vram_write(0x81, 0x80, 0x0000, 0xAB);
    emit_vram_write(0x85, 0x84, 0x0000, 0xCD);

    const auto loop_pc = static_cast<std::uint16_t>(pc);
    emit({0xC3, static_cast<std::uint8_t>(loop_pc & 0xFFU), static_cast<std::uint8_t>(loop_pc >> 8U)});
    return ObservabilityRom{.bytes = rom, .loop_pc = loop_pc};
}

auto run_headless_capture(std::initializer_list<std::string> args) -> std::pair<int, std::string> {
    ArgvBuilder argv(args);
    std::ostringstream stdout_capture;
    std::ostringstream stderr_capture;
    ScopedStreamRedirect cout_redirect(std::cout, stdout_capture.rdbuf());
    ScopedStreamRedirect cerr_redirect(std::cerr, stderr_capture.rdbuf());
    const auto code =
        vanguard8::frontend::run_headless_app(static_cast<int>(argv.argv.size()), argv.argv.data());
    return {code, stdout_capture.str() + stderr_capture.str()};
}

auto digest_hex(const vanguard8::core::Sha256Digest& digest) -> std::string {
    static constexpr char hex[] = "0123456789abcdef";
    std::string output;
    output.reserve(digest.size() * 2U);
    for (const auto byte : digest) {
        output.push_back(hex[(byte >> 4U) & 0x0FU]);
        output.push_back(hex[byte & 0x0FU]);
    }
    return output;
}

auto output_line(const std::string& output, const std::string& prefix) -> std::string {
    std::istringstream stream(output);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.rfind(prefix, 0) == 0) {
            return line;
        }
    }
    return {};
}

auto expected_runtime_frame(const std::vector<std::uint8_t>& rom, const std::uint64_t frames)
    -> std::vector<std::uint8_t> {
    Emulator emulator;
    emulator.load_rom_image(rom);
    emulator.run_frames(frames);
    return Compositor::compose_dual_vdp(emulator.vdp_a(), emulator.vdp_b());
}

auto expected_fixture_frame() -> std::vector<std::uint8_t> {
    V9938 vdp_a;
    V9938 vdp_b;
    vanguard8::frontend::build_dual_vdp_fixture_frame(vdp_a, vdp_b);
    return Compositor::compose_dual_vdp(vdp_a, vdp_b);
}

}  // namespace

TEST_CASE("headless --dump-frame writes the ROM-driven 256x212 runtime frame", "[frontend]") {
    const auto temp_dir = make_temp_dir();
    const auto rom_path = temp_dir / "runtime-256.rom";
    const auto dump_path = temp_dir / "runtime-256.ppm";
    const auto config_home = temp_dir / "xdg-config";
    const auto rom = make_frame_loop_rom();
    write_binary_file(rom_path, rom);

    ScopedEnvVar scoped_xdg("XDG_CONFIG_HOME", config_home.string());
    ArgvBuilder argv({"vanguard8_headless", "--rom", rom_path.string(), "--frames", "1", "--dump-frame", dump_path.string()});

    REQUIRE(vanguard8::frontend::run_headless_app(
                static_cast<int>(argv.argv.size()),
                argv.argv.data()
            ) == 0);

    const auto ppm = parse_ppm(dump_path);
    REQUIRE(ppm.width == V9938::visible_width);
    REQUIRE(ppm.height == V9938::visible_height);
    REQUIRE(ppm.pixels == expected_runtime_frame(rom, 1));

    std::filesystem::remove_all(temp_dir);
}

TEST_CASE("headless --dump-frame preserves 512x212 mixed-width runtime output", "[frontend]") {
    const auto temp_dir = make_temp_dir();
    const auto rom_path = temp_dir / "runtime-512.rom";
    const auto dump_path = temp_dir / "runtime-512.ppm";
    const auto config_home = temp_dir / "xdg-config";
    const auto rom = make_graphic6_runtime_rom();
    write_binary_file(rom_path, rom);

    ScopedEnvVar scoped_xdg("XDG_CONFIG_HOME", config_home.string());
    ArgvBuilder argv({"vanguard8_headless", "--rom", rom_path.string(), "--frames", "1", "--dump-frame", dump_path.string()});

    REQUIRE(vanguard8::frontend::run_headless_app(
                static_cast<int>(argv.argv.size()),
                argv.argv.data()
            ) == 0);

    const auto ppm = parse_ppm(dump_path);
    REQUIRE(ppm.width == V9938::max_visible_width);
    REQUIRE(ppm.height == V9938::visible_height);
    REQUIRE(ppm.pixels == expected_runtime_frame(rom, 1));

    std::filesystem::remove_all(temp_dir);
}

TEST_CASE("headless --dump-fixture remains an explicit fixture-only capture path", "[frontend]") {
    const auto temp_dir = make_temp_dir();
    const auto dump_path = temp_dir / "fixture.ppm";
    const auto config_home = temp_dir / "xdg-config";

    ScopedEnvVar scoped_xdg("XDG_CONFIG_HOME", config_home.string());
    ArgvBuilder argv({"vanguard8_headless", "--dump-fixture", dump_path.string()});

    REQUIRE(vanguard8::frontend::run_headless_app(
                static_cast<int>(argv.argv.size()),
                argv.argv.data()
            ) == 0);

    const auto ppm = parse_ppm(dump_path);
    REQUIRE(ppm.width == V9938::visible_width);
    REQUIRE(ppm.height == V9938::visible_height);
    REQUIRE(ppm.pixels == expected_fixture_frame());

    std::filesystem::remove_all(temp_dir);
}

TEST_CASE("headless --inspect emits the committed observability report format", "[frontend]") {
    const auto temp_dir = make_temp_dir();
    const auto config_home = temp_dir / "xdg-config";
    const auto report_path = temp_dir / "inspect.txt";
    const auto source_dir = std::filesystem::path(VANGUARD8_SOURCE_DIR);
    const auto rom_path = source_dir / "tests/replays/replay_fixture.rom";
    const auto golden_path = source_dir / "tests/golden/headless_observability_report.txt";

    ScopedEnvVar scoped_xdg("XDG_CONFIG_HOME", config_home.string());
    const auto [code, output] = run_headless_capture({
        "vanguard8_headless",
        "--rom",
        rom_path.string(),
        "--frames",
        "1",
        "--inspect-frame",
        "1",
        "--inspect",
        report_path.string(),
        "--dump-cpu",
        "--dump-vdp-regs",
        "--peek-mem",
        "0x00000:4",
        "--peek-logical",
        "0x0000:4",
    });

    REQUIRE(code == 0);
    REQUIRE(output.find("Inspection report path: ") != std::string::npos);
    REQUIRE(read_text_file(report_path) == read_text_file(golden_path));

    std::filesystem::remove_all(temp_dir);
}

TEST_CASE("headless inspection reports physical and logical memory peeks from the same frame", "[frontend]") {
    const auto temp_dir = make_temp_dir();
    const auto config_home = temp_dir / "xdg-config";
    const auto rom_path = temp_dir / "observability.rom";
    const auto rom = make_observability_rom();
    write_binary_file(rom_path, rom.bytes);

    ScopedEnvVar scoped_xdg("XDG_CONFIG_HOME", config_home.string());
    const auto [code, output] = run_headless_capture({
        "vanguard8_headless",
        "--rom",
        rom_path.string(),
        "--frames",
        "1",
        "--peek-mem",
        "0xF0250:2",
        "--peek-logical",
        "0x8250:2",
    });

    Emulator emulator;
    emulator.load_rom_image(rom.bytes);
    emulator.run_frames(1);

    REQUIRE(code == 0);
    REQUIRE(emulator.mutable_bus().read_memory(0xF0250) == 0x12);
    REQUIRE(emulator.mutable_bus().read_memory(0xF0251) == 0x34);
    REQUIRE(output.find("physical 0xf0250 length 2") != std::string::npos);
    REQUIRE(output.find("logical 0x8250 physical 0xf0250 region ca1 length 2") != std::string::npos);
    REQUIRE(output.find("0xf0250: 12 34") != std::string::npos);

    std::filesystem::remove_all(temp_dir);
}

TEST_CASE("headless VRAM dumps are deterministic and match VDP state snapshots", "[frontend]") {
    const auto temp_dir = make_temp_dir();
    const auto config_home = temp_dir / "xdg-config";
    const auto rom_path = temp_dir / "observability.rom";
    const auto dump_a_1 = temp_dir / "a1.bin";
    const auto dump_b_1 = temp_dir / "b1.bin";
    const auto dump_a_2 = temp_dir / "a2.bin";
    const auto dump_b_2 = temp_dir / "b2.bin";
    const auto rom = make_observability_rom();
    write_binary_file(rom_path, rom.bytes);

    ScopedEnvVar scoped_xdg("XDG_CONFIG_HOME", config_home.string());
    const auto [first_code, first_output] = run_headless_capture({
        "vanguard8_headless",
        "--rom",
        rom_path.string(),
        "--frames",
        "1",
        "--dump-vram-a",
        dump_a_1.string(),
        "--dump-vram-b",
        dump_b_1.string(),
    });
    const auto [second_code, second_output] = run_headless_capture({
        "vanguard8_headless",
        "--rom",
        rom_path.string(),
        "--frames",
        "1",
        "--dump-vram-a",
        dump_a_2.string(),
        "--dump-vram-b",
        dump_b_2.string(),
    });

    Emulator emulator;
    emulator.load_rom_image(rom.bytes);
    emulator.run_frames(1);
    const auto expected_a = std::vector<std::uint8_t>(emulator.vdp_a().vram().begin(), emulator.vdp_a().vram().end());
    const auto expected_b = std::vector<std::uint8_t>(emulator.vdp_b().vram().begin(), emulator.vdp_b().vram().end());

    REQUIRE(first_code == 0);
    REQUIRE(second_code == 0);
    REQUIRE(read_binary_file(dump_a_1) == read_binary_file(dump_a_2));
    REQUIRE(read_binary_file(dump_b_1) == read_binary_file(dump_b_2));
    REQUIRE(read_binary_file(dump_a_1) == expected_a);
    REQUIRE(read_binary_file(dump_b_1) == expected_b);
    REQUIRE(first_output.find("VDP-A VRAM SHA-256: " + digest_hex(vanguard8::core::sha256(expected_a))) != std::string::npos);
    REQUIRE(first_output.find("VDP-B VRAM SHA-256: " + digest_hex(vanguard8::core::sha256(expected_b))) != std::string::npos);
    REQUIRE(output_line(first_output, "VDP-A VRAM SHA-256: ") == output_line(second_output, "VDP-A VRAM SHA-256: "));
    REQUIRE(output_line(first_output, "VDP-B VRAM SHA-256: ") == output_line(second_output, "VDP-B VRAM SHA-256: "));

    std::filesystem::remove_all(temp_dir);
}

TEST_CASE("headless observability flags do not perturb frame audio or event digests", "[frontend]") {
    const auto temp_dir = make_temp_dir();
    const auto config_home = temp_dir / "xdg-config";
    const auto rom_path = temp_dir / "observability.rom";
    const auto report_path = temp_dir / "inspect.txt";
    const auto dump_a = temp_dir / "a.bin";
    const auto dump_b = temp_dir / "b.bin";
    const auto rom = make_observability_rom();
    write_binary_file(rom_path, rom.bytes);

    ScopedEnvVar scoped_xdg("XDG_CONFIG_HOME", config_home.string());
    const auto [baseline_code, baseline_output] = run_headless_capture({
        "vanguard8_headless",
        "--rom",
        rom_path.string(),
        "--frames",
        "1",
        "--hash-frame",
        "1",
        "--hash-audio",
    });
    const auto [observed_code, observed_output] = run_headless_capture({
        "vanguard8_headless",
        "--rom",
        rom_path.string(),
        "--frames",
        "1",
        "--hash-frame",
        "1",
        "--hash-audio",
        "--inspect",
        report_path.string(),
        "--peek-mem",
        "0xF0250:2",
        "--peek-logical",
        "0x8250:2",
        "--dump-cpu",
        "--dump-vdp-regs",
        "--dump-vram-a",
        dump_a.string(),
        "--dump-vram-b",
        dump_b.string(),
        "--run-until-pc",
        "0x1234:1",
    });

    REQUIRE(baseline_code == 0);
    REQUIRE(observed_code == 0);
    REQUIRE(output_line(observed_output, "Frame SHA-256 (1): ") == output_line(baseline_output, "Frame SHA-256 (1): "));
    REQUIRE(output_line(observed_output, "Audio SHA-256: ") == output_line(baseline_output, "Audio SHA-256: "));
    REQUIRE(output_line(observed_output, "Event log digest: ") == output_line(baseline_output, "Event log digest: "));

    std::filesystem::remove_all(temp_dir);
}

TEST_CASE("headless --run-until-pc reports hit and not-hit frame outcomes", "[frontend]") {
    const auto temp_dir = make_temp_dir();
    const auto config_home = temp_dir / "xdg-config";
    const auto rom_path = std::filesystem::path(VANGUARD8_SOURCE_DIR) / "tests/replays/replay_fixture.rom";

    ScopedEnvVar scoped_xdg("XDG_CONFIG_HOME", config_home.string());
    const auto [hit_code, hit_output] = run_headless_capture({
        "vanguard8_headless",
        "--rom",
        rom_path.string(),
        "--run-until-pc",
        "0x0000:1",
    });
    const auto [miss_code, miss_output] = run_headless_capture({
        "vanguard8_headless",
        "--rom",
        rom_path.string(),
        "--frames",
        "1",
        "--run-until-pc",
        "0x1234",
    });

    REQUIRE(hit_code == 0);
    REQUIRE(hit_output.find("Run-until-PC result: hit") != std::string::npos);
    REQUIRE(hit_output.find("Run-until-PC frame: 1") != std::string::npos);
    REQUIRE(miss_code == 0);
    REQUIRE(miss_output.find("Run-until-PC result: not-hit") != std::string::npos);
    REQUIRE(miss_output.find("Run-until-PC frame: 1") != std::string::npos);

    std::filesystem::remove_all(temp_dir);
}

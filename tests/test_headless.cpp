#include <catch2/catch_test_macros.hpp>

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
#include <optional>
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

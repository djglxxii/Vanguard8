#include <catch2/catch_test_macros.hpp>

#include "core/config.hpp"
#include "core/emulator.hpp"

#include <filesystem>

TEST_CASE("no-op core exposes build summary", "[scaffold]") {
    const vanguard8::core::Emulator emulator;
    const auto summary = emulator.build_summary();

    REQUIRE(summary.find("Vanguard 8 Emulator") != std::string::npos);
    REQUIRE(summary.find("Vanguard 8 Emulator") != std::string::npos);
}

TEST_CASE("config scaffold creates a default TOML file", "[scaffold]") {
    const auto temp_root =
        std::filesystem::temp_directory_path() / "vanguard8-scaffold-config-test";
    std::filesystem::remove_all(temp_root);

    const auto config_path = temp_root / "config.toml";
    const auto config = vanguard8::core::load_or_create_config(config_path);

    REQUIRE(std::filesystem::exists(config_path));
    REQUIRE(config.display_scale == 4);
    REQUIRE(config.audio_sample_rate == 48'000);
}

TEST_CASE("config save and reload preserves milestone 6 frontend settings", "[scaffold]") {
    const auto temp_root =
        std::filesystem::temp_directory_path() / "vanguard8-config-roundtrip-test";
    std::filesystem::remove_all(temp_root);

    const auto config_path = temp_root / "config.toml";
    vanguard8::core::AppConfig config;
    config.display_scale = 5;
    config.display_aspect = "ntsc";
    config.fullscreen = true;
    config.frame_pacing = false;
    config.recent_roms = {"/tmp/one.rom", "/tmp/two.rom"};

    vanguard8::core::save_config(config_path, config);
    const auto loaded = vanguard8::core::load_or_create_config(config_path);

    REQUIRE(loaded.display_scale == 5);
    REQUIRE(loaded.display_aspect == "ntsc");
    REQUIRE(loaded.fullscreen);
    REQUIRE_FALSE(loaded.frame_pacing);
    REQUIRE(loaded.recent_roms == config.recent_roms);
}

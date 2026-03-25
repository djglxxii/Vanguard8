#include <catch2/catch_test_macros.hpp>

#include "core/config.hpp"
#include "core/emulator.hpp"

#include <filesystem>

TEST_CASE("no-op core exposes build summary", "[scaffold]") {
    const vanguard8::core::Emulator emulator;
    const auto summary = emulator.build_summary();

    REQUIRE(summary.find("Vanguard 8 Emulator") != std::string::npos);
    REQUIRE(summary.find("no-op shell") != std::string::npos);
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

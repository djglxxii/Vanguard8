#pragma once

#include <filesystem>
#include <string>

namespace vanguard8::core {

struct AppConfig {
    int display_scale = 4;
    int audio_sample_rate = 48'000;
    bool fullscreen = false;

    [[nodiscard]] auto to_toml() const -> std::string;
};

[[nodiscard]] auto default_config_path() -> std::filesystem::path;
[[nodiscard]] auto load_or_create_config(const std::filesystem::path& path) -> AppConfig;

}  // namespace vanguard8::core

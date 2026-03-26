#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace vanguard8::core {

struct InputBindings {
    std::string p1_up = "Up";
    std::string p1_down = "Down";
    std::string p1_left = "Left";
    std::string p1_right = "Right";
    std::string p1_a = "Z";
    std::string p1_b = "X";
    std::string p1_select = "RShift";
    std::string p1_start = "Return";

    std::string p2_up = "W";
    std::string p2_down = "S";
    std::string p2_left = "A";
    std::string p2_right = "D";
    std::string p2_a = "Q";
    std::string p2_b = "E";
    std::string p2_select = "Tab";
    std::string p2_start = "Backspace";

    std::string gamepad_up = "dpup";
    std::string gamepad_down = "dpdown";
    std::string gamepad_left = "dpleft";
    std::string gamepad_right = "dpright";
    std::string gamepad_a = "a";
    std::string gamepad_b = "b";
    std::string gamepad_select = "back";
    std::string gamepad_start = "start";
};

struct AppConfig {
    int display_scale = 4;
    std::string display_aspect = "square";
    int audio_sample_rate = 48'000;
    bool fullscreen = false;
    bool frame_pacing = true;
    std::vector<std::string> recent_roms;
    InputBindings input{};

    [[nodiscard]] auto to_toml() const -> std::string;
};

[[nodiscard]] auto default_config_path() -> std::filesystem::path;
[[nodiscard]] auto load_or_create_config(const std::filesystem::path& path) -> AppConfig;
void save_config(const std::filesystem::path& path, const AppConfig& config);

}  // namespace vanguard8::core

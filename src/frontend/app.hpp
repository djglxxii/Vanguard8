#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace vanguard8::frontend {

struct FrontendRuntimeOptions {
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

auto parse_frontend_options(int argc, char** argv) -> FrontendRuntimeOptions;

auto run_frontend_app(int argc, char** argv) -> int;

}  // namespace vanguard8::frontend

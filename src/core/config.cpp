#include "core/config.hpp"

#include "core/logging.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace vanguard8::core {

auto AppConfig::to_toml() const -> std::string {
    std::ostringstream toml;
    toml << "[display]\n";
    toml << "scale = " << display_scale << '\n';
    toml << "fullscreen = " << (fullscreen ? "true" : "false") << "\n\n";
    toml << "[audio]\n";
    toml << "sample_rate = " << audio_sample_rate << '\n';
    return toml.str();
}

auto default_config_path() -> std::filesystem::path {
    if (const char* xdg_config_home = std::getenv("XDG_CONFIG_HOME")) {
        return std::filesystem::path(xdg_config_home) / "vanguard8" / "config.toml";
    }

    if (const char* home = std::getenv("HOME")) {
        return std::filesystem::path(home) / ".config" / "vanguard8" / "config.toml";
    }

    return std::filesystem::current_path() / "vanguard8-config.toml";
}

auto load_or_create_config(const std::filesystem::path& path) -> AppConfig {
    const AppConfig config{};

    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream output(path);
        output << config.to_toml();
        log(LogLevel::info, "Created default config at " + path.string());
        return config;
    }

    log(LogLevel::info, "Loaded config scaffold from " + path.string());
    return config;
}

}  // namespace vanguard8::core

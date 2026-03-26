#include "core/config.hpp"

#include "core/logging.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string_view>

namespace vanguard8::core {

namespace {

auto trim(std::string value) -> std::string {
    const auto not_space = [](const unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

auto unquote(const std::string& value) -> std::string {
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

auto parse_bool(const std::string& value, const bool fallback) -> bool {
    if (value == "true") {
        return true;
    }
    if (value == "false") {
        return false;
    }
    return fallback;
}

auto parse_int(const std::string& value, const int fallback) -> int {
    try {
        return std::stoi(value);
    } catch (...) {
        return fallback;
    }
}

auto parse_string_array(const std::string& value) -> std::vector<std::string> {
    std::vector<std::string> items;
    const auto stripped = trim(value);
    if (stripped.size() < 2 || stripped.front() != '[' || stripped.back() != ']') {
        return items;
    }

    auto body = stripped.substr(1, stripped.size() - 2);
    std::string current;
    bool in_quotes = false;
    for (const char ch : body) {
        if (ch == '"') {
            in_quotes = !in_quotes;
            current.push_back(ch);
            continue;
        }
        if (ch == ',' && !in_quotes) {
            const auto item = trim(current);
            if (!item.empty()) {
                items.push_back(unquote(item));
            }
            current.clear();
            continue;
        }
        current.push_back(ch);
    }

    const auto tail = trim(current);
    if (!tail.empty()) {
        items.push_back(unquote(tail));
    }
    return items;
}

void assign_input_binding(InputBindings& input, const std::string& key, const std::string& value) {
    if (key == "p1_up") input.p1_up = value;
    else if (key == "p1_down") input.p1_down = value;
    else if (key == "p1_left") input.p1_left = value;
    else if (key == "p1_right") input.p1_right = value;
    else if (key == "p1_a") input.p1_a = value;
    else if (key == "p1_b") input.p1_b = value;
    else if (key == "p1_select") input.p1_select = value;
    else if (key == "p1_start") input.p1_start = value;
    else if (key == "p2_up") input.p2_up = value;
    else if (key == "p2_down") input.p2_down = value;
    else if (key == "p2_left") input.p2_left = value;
    else if (key == "p2_right") input.p2_right = value;
    else if (key == "p2_a") input.p2_a = value;
    else if (key == "p2_b") input.p2_b = value;
    else if (key == "p2_select") input.p2_select = value;
    else if (key == "p2_start") input.p2_start = value;
    else if (key == "gamepad_up") input.gamepad_up = value;
    else if (key == "gamepad_down") input.gamepad_down = value;
    else if (key == "gamepad_left") input.gamepad_left = value;
    else if (key == "gamepad_right") input.gamepad_right = value;
    else if (key == "gamepad_a") input.gamepad_a = value;
    else if (key == "gamepad_b") input.gamepad_b = value;
    else if (key == "gamepad_select") input.gamepad_select = value;
    else if (key == "gamepad_start") input.gamepad_start = value;
}

}  // namespace

auto AppConfig::to_toml() const -> std::string {
    std::ostringstream toml;
    toml << "[display]\n";
    toml << "scale = " << display_scale << '\n';
    toml << "aspect = \"" << display_aspect << "\"\n";
    toml << "fullscreen = " << (fullscreen ? "true" : "false") << '\n';
    toml << "frame_pacing = " << (frame_pacing ? "true" : "false") << "\n\n";

    toml << "[audio]\n";
    toml << "sample_rate = " << audio_sample_rate << "\n\n";

    toml << "[input]\n";
    toml << "p1_up = \"" << input.p1_up << "\"\n";
    toml << "p1_down = \"" << input.p1_down << "\"\n";
    toml << "p1_left = \"" << input.p1_left << "\"\n";
    toml << "p1_right = \"" << input.p1_right << "\"\n";
    toml << "p1_a = \"" << input.p1_a << "\"\n";
    toml << "p1_b = \"" << input.p1_b << "\"\n";
    toml << "p1_select = \"" << input.p1_select << "\"\n";
    toml << "p1_start = \"" << input.p1_start << "\"\n";
    toml << "p2_up = \"" << input.p2_up << "\"\n";
    toml << "p2_down = \"" << input.p2_down << "\"\n";
    toml << "p2_left = \"" << input.p2_left << "\"\n";
    toml << "p2_right = \"" << input.p2_right << "\"\n";
    toml << "p2_a = \"" << input.p2_a << "\"\n";
    toml << "p2_b = \"" << input.p2_b << "\"\n";
    toml << "p2_select = \"" << input.p2_select << "\"\n";
    toml << "p2_start = \"" << input.p2_start << "\"\n";
    toml << "gamepad_up = \"" << input.gamepad_up << "\"\n";
    toml << "gamepad_down = \"" << input.gamepad_down << "\"\n";
    toml << "gamepad_left = \"" << input.gamepad_left << "\"\n";
    toml << "gamepad_right = \"" << input.gamepad_right << "\"\n";
    toml << "gamepad_a = \"" << input.gamepad_a << "\"\n";
    toml << "gamepad_b = \"" << input.gamepad_b << "\"\n";
    toml << "gamepad_select = \"" << input.gamepad_select << "\"\n";
    toml << "gamepad_start = \"" << input.gamepad_start << "\"\n\n";

    toml << "[session]\n";
    toml << "recent_roms = [";
    for (std::size_t index = 0; index < recent_roms.size(); ++index) {
        if (index > 0) {
            toml << ", ";
        }
        toml << '"' << recent_roms[index] << '"';
    }
    toml << "]\n";
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

void save_config(const std::filesystem::path& path, const AppConfig& config) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path);
    output << config.to_toml();
}

auto load_or_create_config(const std::filesystem::path& path) -> AppConfig {
    AppConfig config{};

    if (!std::filesystem::exists(path)) {
        save_config(path, config);
        log(LogLevel::info, "Created default config at " + path.string());
        return config;
    }

    std::ifstream input(path);
    std::string line;
    std::string section;
    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty() || line.starts_with('#')) {
            continue;
        }
        if (line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            continue;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        const auto key = trim(line.substr(0, separator));
        const auto raw_value = trim(line.substr(separator + 1));

        if (section == "display") {
            if (key == "scale") config.display_scale = parse_int(raw_value, config.display_scale);
            else if (key == "aspect") config.display_aspect = unquote(raw_value);
            else if (key == "fullscreen") config.fullscreen = parse_bool(raw_value, config.fullscreen);
            else if (key == "frame_pacing") config.frame_pacing = parse_bool(raw_value, config.frame_pacing);
            continue;
        }

        if (section == "audio") {
            if (key == "sample_rate") {
                config.audio_sample_rate = parse_int(raw_value, config.audio_sample_rate);
            }
            continue;
        }

        if (section == "input") {
            assign_input_binding(config.input, key, unquote(raw_value));
            continue;
        }

        if (section == "session" && key == "recent_roms") {
            config.recent_roms = parse_string_array(raw_value);
        }
    }

    log(LogLevel::info, "Loaded config scaffold from " + path.string());
    return config;
}

}  // namespace vanguard8::core

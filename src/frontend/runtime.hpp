#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace vanguard8::frontend {

enum class RuntimeEventType {
    quit,
    key_down,
    key_up,
    gamepad_button_down,
    gamepad_button_up,
};

struct RuntimeEvent {
    RuntimeEventType type = RuntimeEventType::quit;
    std::string control_name{};
    int gamepad_index = 0;
};

struct WindowConfig {
    std::string title = "Vanguard 8";
    int logical_width = 256;
    int logical_height = 212;
    int scale = 4;
    bool fullscreen = false;
    bool hidden = false;
};

class WindowHost {
  public:
    virtual ~WindowHost() = default;

    [[nodiscard]] virtual auto create(const WindowConfig& config, std::string& error) -> bool = 0;
    virtual void drawable_size(int& width, int& height) const = 0;
    virtual void pump_events(std::vector<RuntimeEvent>& events) = 0;
    virtual void set_title(std::string_view title) = 0;
    virtual void set_fullscreen(bool fullscreen) = 0;
    virtual void present() = 0;
    virtual void shutdown() = 0;
};

struct RuntimeHooks {
    std::function<bool(std::string&)> on_started{};
    std::function<bool(const RuntimeEvent&)> on_event{};
    std::function<bool(std::string&)> on_frame{};
    std::function<void()> on_shutdown{};
};

[[nodiscard]] auto run_window_runtime(
    WindowHost& window_host,
    const WindowConfig& config,
    const RuntimeHooks& hooks,
    std::string& error
) -> int;

}  // namespace vanguard8::frontend

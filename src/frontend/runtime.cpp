#include "frontend/runtime.hpp"

namespace vanguard8::frontend {

namespace {

class WindowHostGuard {
  public:
    explicit WindowHostGuard(WindowHost& host) : host_(host) {}
    ~WindowHostGuard() { host_.shutdown(); }

    WindowHostGuard(const WindowHostGuard&) = delete;
    auto operator=(const WindowHostGuard&) -> WindowHostGuard& = delete;

  private:
    WindowHost& host_;
};

}  // namespace

auto run_window_runtime(
    WindowHost& window_host,
    const WindowConfig& config,
    const RuntimeHooks& hooks,
    std::string& error
) -> int {
    if (!window_host.create(config, error)) {
        return 2;
    }

    WindowHostGuard guard(window_host);
    std::vector<RuntimeEvent> events;
    bool running = true;

    while (running) {
        if (hooks.on_frame) {
            hooks.on_frame();
        }
        window_host.present();

        events.clear();
        window_host.pump_events(events);
        for (const auto& event : events) {
            if (hooks.on_event) {
                hooks.on_event(event);
            }
            if (event.type == RuntimeEventType::quit) {
                running = false;
            }
        }
    }

    return 0;
}

}  // namespace vanguard8::frontend

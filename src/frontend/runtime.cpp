#include "frontend/runtime.hpp"

#include <thread>

namespace vanguard8::frontend {

namespace {

using RuntimeClock = RuntimeTimingHooks::Clock;

[[nodiscard]] auto default_runtime_timing_hooks() -> RuntimeTimingHooks {
    return RuntimeTimingHooks{
        .now = [] { return RuntimeClock::now(); },
        .sleep_for = [](const RuntimeClock::duration duration) {
            if (duration > RuntimeClock::duration::zero()) {
                std::this_thread::sleep_for(duration);
            }
        },
    };
}

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
    const RuntimeTimingHooks& timing_hooks,
    std::string& error
) -> int {
    if (!window_host.create(config, error)) {
        return 2;
    }

    WindowHostGuard guard(window_host);
    if (hooks.on_started && !hooks.on_started(error)) {
        if (hooks.on_shutdown) {
            hooks.on_shutdown();
        }
        return 2;
    }

    std::vector<RuntimeEvent> events;
    bool running = true;
    auto next_frame_deadline = timing_hooks.now();

    while (running) {
        if (hooks.on_frame && !hooks.on_frame(error)) {
            if (hooks.on_shutdown) {
                hooks.on_shutdown();
            }
            return 2;
        }
        window_host.present();

        events.clear();
        window_host.pump_events(events);
        for (const auto& event : events) {
            if (hooks.on_event && !hooks.on_event(event)) {
                running = false;
            }
            if (event.type == RuntimeEventType::quit) {
                running = false;
            }
        }

        if (running && config.frame_pacing) {
            next_frame_deadline += timing_hooks.frame_period;
            const auto now = timing_hooks.now();
            if (now < next_frame_deadline) {
                timing_hooks.sleep_for(next_frame_deadline - now);
            } else {
                next_frame_deadline = now;
            }
        }
    }

    if (hooks.on_shutdown) {
        hooks.on_shutdown();
    }
    return 0;
}

auto run_window_runtime(
    WindowHost& window_host,
    const WindowConfig& config,
    const RuntimeHooks& hooks,
    std::string& error
) -> int {
    return run_window_runtime(window_host, config, hooks, default_runtime_timing_hooks(), error);
}

}  // namespace vanguard8::frontend

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "frontend/app.hpp"
#include "frontend/audio_output.hpp"
#include "frontend/runtime.hpp"

#include <filesystem>
#include <string>
#include <chrono>
#include <utility>
#include <vector>

namespace {

using vanguard8::frontend::RuntimeEvent;
using vanguard8::frontend::RuntimeEventType;
using vanguard8::frontend::RuntimeHooks;
using vanguard8::frontend::WindowConfig;
using vanguard8::frontend::WindowHost;

class FakeWindowHost final : public WindowHost {
  public:
    bool fail_create = false;
    bool create_called = false;
    bool shutdown_called = false;
    int present_count = 0;
    WindowConfig created_config{};
    std::vector<std::vector<RuntimeEvent>> event_batches{};
    std::size_t event_batch_index = 0;

    [[nodiscard]] auto create(const WindowConfig& config, std::string& error) -> bool override {
        create_called = true;
        created_config = config;
        if (fail_create) {
            error = "fake create failure";
            return false;
        }
        return true;
    }

    void drawable_size(int& width, int& height) const override {
        width = created_config.logical_width * created_config.scale;
        height = created_config.logical_height * created_config.scale;
    }

    void pump_events(std::vector<RuntimeEvent>& events) override {
        if (event_batch_index >= event_batches.size()) {
            return;
        }
        for (const auto& event : event_batches[event_batch_index]) {
            events.push_back(event);
        }
        ++event_batch_index;
    }

    void set_title(std::string_view) override {}
    void set_fullscreen(bool) override {}
    void present() override { ++present_count; }

    void shutdown() override { shutdown_called = true; }
};

class FakeAudioOutputDevice final : public vanguard8::frontend::AudioOutputDevice {
  public:
    bool opened = false;
    bool closed = false;
    int open_calls = 0;
    int queue_calls = 0;
    std::size_t queued = 0;

    [[nodiscard]] auto open(const vanguard8::frontend::AudioDeviceConfig&, std::string&) -> bool override {
        ++open_calls;
        opened = true;
        closed = false;
        return true;
    }

    [[nodiscard]] auto queue_audio(const std::vector<std::uint8_t>& pcm_bytes, std::string&) -> bool override {
        ++queue_calls;
        queued += pcm_bytes.size();
        return true;
    }

    [[nodiscard]] auto queued_bytes() const -> std::size_t override { return 0; }

    void close() override {
        opened = false;
        closed = true;
        queued = 0;
    }
};

struct ArgvBuilder {
    std::vector<std::string> storage;
    std::vector<char*> argv;

    explicit ArgvBuilder(std::initializer_list<std::string> args) : storage(args) {
        argv.reserve(storage.size());
        for (auto& arg : storage) {
            argv.push_back(arg.data());
        }
    }
};

}  // namespace

TEST_CASE("window runtime creates pumps and shuts down through the host seam", "[frontend]") {
    FakeWindowHost host;
    host.event_batches = {
        {},
        {RuntimeEvent{.type = RuntimeEventType::quit}},
    };

    int frame_count = 0;
    std::string error;
    const auto result = vanguard8::frontend::run_window_runtime(
        host,
        WindowConfig{
            .title = "Runtime test",
            .logical_width = 256,
            .logical_height = 212,
            .scale = 3,
            .fullscreen = true,
            .frame_pacing = false,
        },
        RuntimeHooks{
            .on_frame = [&](std::string&) {
                ++frame_count;
                return true;
            },
        },
        error
    );

    REQUIRE(result == 0);
    REQUIRE(error.empty());
    REQUIRE(host.create_called);
    REQUIRE(host.shutdown_called);
    REQUIRE(host.present_count == 2);
    REQUIRE(frame_count == 2);
    REQUIRE(host.created_config.title == "Runtime test");
    REQUIRE(host.created_config.scale == 3);
    REQUIRE(host.created_config.fullscreen);
}

TEST_CASE("window runtime returns an error when the host cannot initialize", "[frontend]") {
    FakeWindowHost host;
    host.fail_create = true;

    std::string error;
    const auto result = vanguard8::frontend::run_window_runtime(
        host,
        WindowConfig{},
        RuntimeHooks{},
        error
    );

    REQUIRE(result == 2);
    REQUIRE(error == "fake create failure");
    REQUIRE(host.create_called);
    REQUIRE_FALSE(host.shutdown_called);
}

TEST_CASE("window runtime forwards input events to the callback hook", "[frontend]") {
    FakeWindowHost host;
    host.event_batches = {{
        RuntimeEvent{.type = RuntimeEventType::key_down, .control_name = "Return"},
        RuntimeEvent{.type = RuntimeEventType::gamepad_button_down, .control_name = "a", .gamepad_index = 1},
        RuntimeEvent{.type = RuntimeEventType::quit},
    }};

    std::vector<std::pair<RuntimeEventType, std::string>> seen_events;
    std::string error;
    const auto result = vanguard8::frontend::run_window_runtime(
        host,
        WindowConfig{.frame_pacing = false},
        RuntimeHooks{
            .on_event =
                [&](const RuntimeEvent& event) {
                    seen_events.emplace_back(event.type, event.control_name);
                    return true;
                },
            .on_frame = [](std::string&) { return true; },
        },
        error
    );

    REQUIRE(result == 0);
    REQUIRE(error.empty());
    REQUIRE(seen_events.size() == 3);
    REQUIRE(seen_events[0].first == RuntimeEventType::key_down);
    REQUIRE(seen_events[0].second == "Return");
    REQUIRE(seen_events[1].first == RuntimeEventType::gamepad_button_down);
    REQUIRE(seen_events[1].second == "a");
    REQUIRE(seen_events[2].first == RuntimeEventType::quit);
}

TEST_CASE("window runtime reports startup hook failures after the host creates", "[frontend]") {
    FakeWindowHost host;

    std::string error;
    const auto result = vanguard8::frontend::run_window_runtime(
        host,
        WindowConfig{},
        RuntimeHooks{
            .on_started =
                [](std::string& hook_error) {
                    hook_error = "startup failure";
                    return false;
                },
        },
        error
    );

    REQUIRE(result == 2);
    REQUIRE(error == "startup failure");
    REQUIRE(host.create_called);
    REQUIRE(host.shutdown_called);
}

TEST_CASE("window runtime applies the 59.94 Hz pacing fallback when enabled", "[frontend]") {
    using Clock = vanguard8::frontend::RuntimeTimingHooks::Clock;

    FakeWindowHost host;
    host.event_batches = {
        {},
        {RuntimeEvent{.type = RuntimeEventType::quit}},
    };

    int frame_count = 0;
    Clock::time_point now{};
    std::vector<Clock::duration> sleep_calls;
    std::string error;
    const auto result = vanguard8::frontend::run_window_runtime(
        host,
        WindowConfig{.frame_pacing = true},
        RuntimeHooks{
            .on_frame = [&](std::string&) {
                ++frame_count;
                return true;
            },
        },
        vanguard8::frontend::RuntimeTimingHooks{
            .now = [&] { return now; },
            .sleep_for =
                [&](const Clock::duration duration) {
                    sleep_calls.push_back(duration);
                    now += duration;
                },
        },
        error
    );

    REQUIRE(result == 0);
    REQUIRE(error.empty());
    REQUIRE(frame_count == 2);
    REQUIRE(sleep_calls.size() == 1);
    REQUIRE(sleep_calls.front() == std::chrono::nanoseconds{16'683'333});
}

TEST_CASE("window runtime skips the pacing fallback when disabled", "[frontend]") {
    using Clock = vanguard8::frontend::RuntimeTimingHooks::Clock;

    FakeWindowHost host;
    host.event_batches = {
        {},
        {RuntimeEvent{.type = RuntimeEventType::quit}},
    };

    std::vector<Clock::duration> sleep_calls;
    std::string error;
    const auto result = vanguard8::frontend::run_window_runtime(
        host,
        WindowConfig{.frame_pacing = false},
        RuntimeHooks{
            .on_frame = [](std::string&) { return true; },
        },
        vanguard8::frontend::RuntimeTimingHooks{
            .now = [] { return Clock::time_point{}; },
            .sleep_for = [&](const Clock::duration duration) { sleep_calls.push_back(duration); },
        },
        error
    );

    REQUIRE(result == 0);
    REQUIRE(error.empty());
    REQUIRE(sleep_calls.empty());
}

TEST_CASE("window runtime opens pumps and closes frontend audio through fake device hooks", "[frontend]") {
    FakeWindowHost host;
    host.event_batches = {
        {},
        {},
        {RuntimeEvent{.type = RuntimeEventType::quit}},
    };

    FakeAudioOutputDevice audio;
    const vanguard8::frontend::AudioQueuePump pump(16'384);
    int frame_count = 0;
    bool audio_was_open_before_first_frame = false;
    std::string error;

    const auto result = vanguard8::frontend::run_window_runtime(
        host,
        WindowConfig{.frame_pacing = false},
        RuntimeHooks{
            .on_started = [&](std::string& hook_error) {
                return audio.open(vanguard8::frontend::AudioDeviceConfig{}, hook_error);
            },
            .on_frame = [&](std::string& hook_error) {
                if (frame_count == 0) {
                    audio_was_open_before_first_frame = audio.opened;
                }
                ++frame_count;
                return pump.pump(audio, std::vector<std::uint8_t>{0x10, 0x00, 0x20, 0x00}, hook_error);
            },
            .on_shutdown = [&] { audio.close(); },
        },
        error
    );

    REQUIRE(result == 0);
    REQUIRE(error.empty());
    REQUIRE(audio_was_open_before_first_frame);
    REQUIRE(audio.open_calls == 1);
    REQUIRE(frame_count == 3);
    REQUIRE(audio.queue_calls == frame_count);
    REQUIRE(audio.closed);
    REQUIRE(host.shutdown_called);
}

TEST_CASE("frontend CLI accepts the milestone 32 positional ROM launch form", "[frontend]") {
    ArgvBuilder args{"vanguard8_frontend", "build/showcase/showcase.rom"};

    const auto options =
        vanguard8::frontend::parse_frontend_options(static_cast<int>(args.argv.size()), args.argv.data());

    REQUIRE(options.rom_path.has_value());
    REQUIRE(*options.rom_path == std::filesystem::path("build/showcase/showcase.rom"));
}

TEST_CASE("frontend CLI still accepts ROM paths behind --rom", "[frontend]") {
    ArgvBuilder args{"vanguard8_frontend", "--rom", "build/showcase/showcase.rom"};

    const auto options =
        vanguard8::frontend::parse_frontend_options(static_cast<int>(args.argv.size()), args.argv.data());

    REQUIRE(options.rom_path.has_value());
    REQUIRE(*options.rom_path == std::filesystem::path("build/showcase/showcase.rom"));
}

TEST_CASE("frontend CLI rejects multiple ROM path sources", "[frontend]") {
    ArgvBuilder args{"vanguard8_frontend", "--rom", "one.rom", "two.rom"};

    REQUIRE_THROWS_WITH(
        vanguard8::frontend::parse_frontend_options(static_cast<int>(args.argv.size()), args.argv.data()),
        Catch::Matchers::ContainsSubstring("Multiple ROM paths")
    );
}

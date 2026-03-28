#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>

#include "core/audio/audio_mixer.hpp"
#include "core/video/compositor.hpp"
#include "frontend/audio_output.hpp"
#include "frontend/display.hpp"
#include "frontend/display_presenter.hpp"
#include "frontend/runtime.hpp"
#include "frontend/sdl_window.hpp"
#include "frontend/video_fixture.hpp"

#include <SDL.h>

#include <cstdlib>
#include <string>
#include <vector>

namespace {

class FakeAudioOutputDevice final : public vanguard8::frontend::AudioOutputDevice {
  public:
    bool opened = false;
    std::size_t queued = 0;
    std::vector<std::uint8_t> last_queued{};

    [[nodiscard]] auto open(const vanguard8::frontend::AudioDeviceConfig&, std::string&) -> bool override {
        opened = true;
        return true;
    }

    [[nodiscard]] auto queue_audio(const std::vector<std::uint8_t>& pcm_bytes, std::string&) -> bool override {
        last_queued = pcm_bytes;
        queued += pcm_bytes.size();
        return true;
    }

    [[nodiscard]] auto queued_bytes() const -> std::size_t override { return queued; }

    void close() override {
        opened = false;
        queued = 0;
        last_queued.clear();
    }
};

}  // namespace

TEST_CASE("audio queue pump only queues while under the configured bound", "[frontend]") {
    FakeAudioOutputDevice device;
    std::string error;
    REQUIRE(device.open(vanguard8::frontend::AudioDeviceConfig{}, error));

    const vanguard8::frontend::AudioQueuePump pump(16);
    REQUIRE(pump.pump(device, std::vector<std::uint8_t>{0x01, 0x02, 0x03, 0x04}, error));
    REQUIRE(device.last_queued.size() == 4);

    device.queued = 32;
    device.last_queued.clear();
    REQUIRE(pump.pump(device, std::vector<std::uint8_t>{0x05, 0x06}, error));
    REQUIRE(device.last_queued.empty());
}

TEST_CASE("audio mixer output bytes can be consumed without disturbing sample accounting", "[frontend]") {
    vanguard8::core::audio::AudioMixer mixer;
    mixer.set_common_sample(vanguard8::core::audio::StereoSample{.left = 0.25F, .right = -0.25F});
    for (std::uint64_t cycle = 0; cycle < 14'318'180ULL; ++cycle) {
        mixer.advance_output_clock();
    }

    REQUIRE(mixer.total_output_sample_count() == vanguard8::core::audio::AudioMixer::output_sample_rate);
    const auto bytes = mixer.consume_output_bytes();
    REQUIRE(bytes.size() == static_cast<std::size_t>(vanguard8::core::audio::AudioMixer::output_sample_rate * 4));
    REQUIRE(mixer.output_bytes().empty());
    REQUIRE(mixer.total_output_sample_count() == vanguard8::core::audio::AudioMixer::output_sample_rate);
}

TEST_CASE("OpenGL display presenter readback matches the uploaded frame through a real hidden window", "[frontend]") {
    vanguard8::frontend::SdlWindowHost window_host;
    std::string error;
    if (!window_host.create(
        vanguard8::frontend::WindowConfig{
            .title = "Presenter test",
            .logical_width = vanguard8::frontend::Display::frame_width,
            .logical_height = vanguard8::frontend::Display::frame_height,
            .scale = 1,
            .hidden = true,
        },
        error
    )) {
        SKIP("SDL/OpenGL backend unavailable for presenter test: " << error);
    }

    vanguard8::frontend::OpenGlDisplayPresenter presenter;
    REQUIRE(presenter.initialize(error));

    vanguard8::core::video::V9938 vdp_a;
    vanguard8::core::video::V9938 vdp_b;
    vanguard8::frontend::build_dual_vdp_fixture_frame(vdp_a, vdp_b);
    const auto frame = vanguard8::core::video::Compositor::compose_dual_vdp(vdp_a, vdp_b);

    vanguard8::frontend::Display display;
    display.upload_frame(frame);

    int drawable_width = 0;
    int drawable_height = 0;
    window_host.drawable_size(drawable_width, drawable_height);
    REQUIRE(presenter.present(display, drawable_width, drawable_height, error));

    const auto readback = presenter.readback_rgb_frame(error);
    REQUIRE(readback == frame);

    presenter.shutdown();
    window_host.shutdown();
}

TEST_CASE("SDL audio output device opens queues and closes against the dummy driver", "[frontend]") {
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);

    vanguard8::frontend::SdlAudioOutputDevice device;
    std::string error;
    REQUIRE(device.open(vanguard8::frontend::AudioDeviceConfig{}, error));
    REQUIRE(device.queue_audio(std::vector<std::uint8_t>(256, 0x11), error));
    REQUIRE(device.queued_bytes() >= 256);

    device.close();
    REQUIRE(device.queued_bytes() == 0);
}

TEST_CASE("SDL window host maps key and close events through the runtime seam", "[frontend]") {
    vanguard8::frontend::SdlWindowHost window_host;
    std::string error;
    if (!window_host.create(
        vanguard8::frontend::WindowConfig{
            .title = "Event test",
            .logical_width = 256,
            .logical_height = 212,
            .scale = 1,
            .hidden = true,
        },
        error
    )) {
        SKIP("SDL video backend unavailable for event-mapping test: " << error);
    }

    SDL_Event key_event{};
    key_event.type = SDL_KEYDOWN;
    key_event.key.keysym.scancode = SDL_SCANCODE_F11;
    key_event.key.repeat = 0;
    REQUIRE(SDL_PushEvent(&key_event) == 1);

    SDL_Event close_event{};
    close_event.type = SDL_WINDOWEVENT;
    close_event.window.event = SDL_WINDOWEVENT_CLOSE;
    REQUIRE(SDL_PushEvent(&close_event) == 1);

    std::vector<vanguard8::frontend::RuntimeEvent> events;
    window_host.pump_events(events);

    REQUIRE(events.size() == 2);
    REQUIRE(events[0].type == vanguard8::frontend::RuntimeEventType::key_down);
    REQUIRE(events[0].control_name == "F11");
    REQUIRE(events[1].type == vanguard8::frontend::RuntimeEventType::quit);

    window_host.shutdown();
}

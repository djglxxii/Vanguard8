#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>

#include "core/audio/audio_mixer.hpp"
#include "core/emulator.hpp"
#include "core/hash.hpp"
#include "core/memory/cartridge.hpp"
#include "core/scheduler.hpp"
#include "core/video/compositor.hpp"
#include "frontend/audio_output.hpp"
#include "frontend/display.hpp"
#include "frontend/display_presenter.hpp"
#include "frontend/runtime.hpp"
#include "frontend/sdl_window.hpp"
#include "frontend/video_fixture.hpp"

#include <SDL.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

class FakeAudioOutputDevice final : public vanguard8::frontend::AudioOutputDevice {
  public:
    bool opened = false;
    bool fail_open = false;
    bool fail_queue = false;
    bool report_zero_queue = false;
    int open_calls = 0;
    int queue_calls = 0;
    int close_calls = 0;
    std::size_t queued = 0;
    std::vector<std::uint8_t> last_queued{};
    std::vector<std::uint8_t> recorded{};

    [[nodiscard]] auto open(const vanguard8::frontend::AudioDeviceConfig&, std::string& error) -> bool override {
        ++open_calls;
        if (fail_open) {
            error = "fake audio open failure";
            return false;
        }
        opened = true;
        return true;
    }

    [[nodiscard]] auto queue_audio(const std::vector<std::uint8_t>& pcm_bytes, std::string& error) -> bool override {
        ++queue_calls;
        if (fail_queue) {
            error = "fake audio queue failure";
            return false;
        }
        last_queued = pcm_bytes;
        recorded.insert(recorded.end(), pcm_bytes.begin(), pcm_bytes.end());
        queued += pcm_bytes.size();
        return true;
    }

    [[nodiscard]] auto queued_bytes() const -> std::size_t override {
        return report_zero_queue ? 0U : queued;
    }

    void close() override {
        ++close_calls;
        opened = false;
        queued = 0;
        last_queued.clear();
    }
};

class ScopedSdlAudioDriver {
  public:
    explicit ScopedSdlAudioDriver(const char* driver) {
        if (const char* current = SDL_getenv("SDL_AUDIODRIVER")) {
            previous_ = current;
        }
        SDL_setenv("SDL_AUDIODRIVER", driver, 1);
    }

    ~ScopedSdlAudioDriver() {
        if (previous_.has_value()) {
            SDL_setenv("SDL_AUDIODRIVER", previous_->c_str(), 1);
        } else {
            unsetenv("SDL_AUDIODRIVER");
        }
    }

    ScopedSdlAudioDriver(const ScopedSdlAudioDriver&) = delete;
    auto operator=(const ScopedSdlAudioDriver&) -> ScopedSdlAudioDriver& = delete;

  private:
    std::optional<std::string> previous_{};
};

auto digest_hex(const vanguard8::core::Sha256Digest& digest) -> std::string {
    std::ostringstream stream;
    stream << std::hex << std::nouppercase << std::setfill('0');
    for (const auto byte : digest) {
        stream << std::setw(2) << static_cast<int>(byte);
    }
    return stream.str();
}

auto read_binary_file(const std::filesystem::path& path) -> std::vector<std::uint8_t> {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Unable to open binary file.");
    }
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
}

void advance_mixer_for_one_frame(vanguard8::core::audio::AudioMixer& mixer) {
    mixer.set_common_sample(vanguard8::core::audio::StereoSample{.left = 0.25F, .right = -0.25F});
    for (std::uint64_t cycle = 0; cycle < vanguard8::core::timing::master_per_frame; ++cycle) {
        mixer.advance_output_clock();
    }
}

auto make_frontend_audio_parity_rom() -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> rom(vanguard8::core::memory::CartridgeSlot::fixed_region_size, 0x00);
    std::size_t pc = 0;
    const auto emit = [&](const std::initializer_list<std::uint8_t> bytes) {
        for (const auto byte : bytes) {
            rom[pc++] = byte;
        }
    };

    // Program an audible AY channel A tone through the documented 0x50/0x51 ports.
    emit({0xF3});                    // DI
    emit({0x3E, 0x00, 0xD3, 0x50});  // R0 = channel A fine period
    emit({0x3E, 0x20, 0xD3, 0x51});
    emit({0x3E, 0x01, 0xD3, 0x50});  // R1 = channel A coarse period
    emit({0x3E, 0x00, 0xD3, 0x51});
    emit({0x3E, 0x07, 0xD3, 0x50});  // R7 = tone A enabled, B/C/noise disabled
    emit({0x3E, 0x3E, 0xD3, 0x51});
    emit({0x3E, 0x08, 0xD3, 0x50});  // R8 = fixed max volume on channel A
    emit({0x3E, 0x0F, 0xD3, 0x51});
    emit({0xC3, static_cast<std::uint8_t>(pc & 0xFFU), static_cast<std::uint8_t>((pc >> 8U) & 0xFFU)});
    return rom;
}

auto collect_headless_pcm_per_frame(vanguard8::core::Emulator& emulator, const std::uint64_t frames)
    -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> pcm;
    for (std::uint64_t frame = 0; frame < frames; ++frame) {
        emulator.run_frames(1);
        auto bytes = emulator.mutable_bus().mutable_audio_mixer().consume_output_bytes();
        pcm.insert(pcm.end(), bytes.begin(), bytes.end());
    }
    return pcm;
}

}  // namespace

TEST_CASE("audio queue pump only queues while under the configured bound", "[frontend]") {
    FakeAudioOutputDevice device;
    std::string error;
    REQUIRE(device.open(vanguard8::frontend::AudioDeviceConfig{}, error));

    const vanguard8::frontend::AudioQueuePump pump(16);
    REQUIRE(pump.pump(device, std::vector<std::uint8_t>{}, error));
    REQUIRE(device.queue_calls == 0);

    REQUIRE(pump.pump(device, std::vector<std::uint8_t>{0x01, 0x02, 0x03, 0x04}, error));
    REQUIRE(device.last_queued.size() == 4);
    REQUIRE(device.queue_calls == 1);

    device.queued = 32;
    device.last_queued.clear();
    REQUIRE(pump.pump(device, std::vector<std::uint8_t>{0x05, 0x06}, error));
    REQUIRE(device.last_queued.empty());
    REQUIRE(device.queue_calls == 1);
}

TEST_CASE("audio queue pump does not consume mixer bytes while back-pressured", "[frontend]") {
    FakeAudioOutputDevice device;
    std::string error;
    REQUIRE(device.open(vanguard8::frontend::AudioDeviceConfig{}, error));

    vanguard8::core::audio::AudioMixer mixer;
    advance_mixer_for_one_frame(mixer);
    const auto pending_bytes = mixer.output_bytes().size();
    REQUIRE(pending_bytes > 0U);

    const vanguard8::frontend::AudioQueuePump pump(16);
    device.queued = 16;
    REQUIRE(pump.pump(device, mixer, error));
    REQUIRE(mixer.output_bytes().size() == pending_bytes);
    REQUIRE(device.queue_calls == 0);

    device.queued = 0;
    REQUIRE(pump.pump(device, mixer, error));
    REQUIRE(mixer.output_bytes().empty());
    REQUIRE(device.recorded.size() == pending_bytes);
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
            .logical_width = vanguard8::frontend::Display::default_frame_width,
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

TEST_CASE("OpenGL display presenter readback preserves a 512 wide uploaded frame", "[frontend]") {
    vanguard8::frontend::SdlWindowHost window_host;
    std::string error;
    if (!window_host.create(
        vanguard8::frontend::WindowConfig{
            .title = "Presenter wide test",
            .logical_width = vanguard8::frontend::Display::default_frame_width,
            .logical_height = vanguard8::frontend::Display::frame_height,
            .scale = 1,
            .hidden = true,
        },
        error
    )) {
        SKIP("SDL/OpenGL backend unavailable for wide presenter test: " << error);
    }

    vanguard8::frontend::OpenGlDisplayPresenter presenter;
    REQUIRE(presenter.initialize(error));

    std::vector<std::uint8_t> frame(
        static_cast<std::size_t>(
            vanguard8::core::video::V9938::max_visible_width *
            vanguard8::frontend::Display::frame_height * 3
        ),
        0x00
    );
    for (std::size_t index = 0; index < frame.size(); ++index) {
        frame[index] = static_cast<std::uint8_t>(index & 0xFFU);
    }

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
    ScopedSdlAudioDriver driver("dummy");

    vanguard8::frontend::SdlAudioOutputDevice device;
    std::string error;
    REQUIRE(device.open(vanguard8::frontend::AudioDeviceConfig{}, error));
    REQUIRE(device.queue_audio(std::vector<std::uint8_t>{}, error));
    REQUIRE(device.queue_audio(std::vector<std::uint8_t>(256, 0x11), error));
    REQUIRE(device.queued_bytes() >= 256);

    device.close();
    REQUIRE(device.queued_bytes() == 0);

    REQUIRE(device.open(vanguard8::frontend::AudioDeviceConfig{}, error));
    REQUIRE(device.queue_audio(std::vector<std::uint8_t>(128, 0x22), error));
    device.close();
    REQUIRE(device.queued_bytes() == 0);
}

TEST_CASE("SDL audio output device reports queue and open failures", "[frontend]") {
    std::string error;

    vanguard8::frontend::SdlAudioOutputDevice unopened;
    REQUIRE_FALSE(unopened.queue_audio(std::vector<std::uint8_t>(16, 0x33), error));
    REQUIRE(error == "SDL audio device is not open.");

    ScopedSdlAudioDriver driver("vanguard8-missing-audio-driver");
    vanguard8::frontend::SdlAudioOutputDevice device;
    error.clear();
    REQUIRE_FALSE(device.open(vanguard8::frontend::AudioDeviceConfig{}, error));
    REQUIRE_FALSE(error.empty());
    REQUIRE(device.queued_bytes() == 0);
}

TEST_CASE("frontend audio queue path is byte-identical to headless per-frame mixer consumption", "[frontend]") {
    const auto rom = make_frontend_audio_parity_rom();

    vanguard8::core::Emulator headless;
    headless.load_rom_image(rom);
    const auto expected_pcm = collect_headless_pcm_per_frame(headless, 4);
    REQUIRE_FALSE(expected_pcm.empty());

    vanguard8::core::Emulator frontend;
    frontend.load_rom_image(rom);
    FakeAudioOutputDevice device;
    device.report_zero_queue = true;
    std::string error;
    REQUIRE(device.open(vanguard8::frontend::AudioDeviceConfig{}, error));
    const vanguard8::frontend::AudioQueuePump pump(1'000'000);

    for (int frame = 0; frame < 4; ++frame) {
        frontend.run_frames(1);
        REQUIRE(pump.pump(device, frontend.mutable_bus().mutable_audio_mixer(), error));
    }

    REQUIRE(device.recorded == expected_pcm);
}

TEST_CASE("PacManV8 T017 audio/video output is nonzero after instruction-granular audio timing", "[frontend]") {
    const auto rom_path = std::filesystem::path("/home/djglxxii/src/PacManV8/build/pacman.rom");
    if (!std::filesystem::is_regular_file(rom_path)) {
        SKIP("PacManV8 pacman.rom is not available at " << rom_path.string());
    }

    vanguard8::core::Emulator emulator;
    emulator.load_rom_image(read_binary_file(rom_path));
    emulator.run_frames(300);

    const auto& audio_bytes = emulator.bus().audio_mixer().output_bytes();
    bool audio_has_nonzero_byte = false;
    for (const auto byte : audio_bytes) {
        audio_has_nonzero_byte = audio_has_nonzero_byte || byte != 0U;
    }

    const auto frame = vanguard8::core::video::Compositor::compose_dual_vdp(emulator.vdp_a(), emulator.vdp_b());
    bool frame_has_nonzero_byte = false;
    for (const auto byte : frame) {
        frame_has_nonzero_byte = frame_has_nonzero_byte || byte != 0U;
    }

    const auto audio_hash = digest_hex(vanguard8::core::sha256(audio_bytes));
    REQUIRE(emulator.cpu().pc() != 0x2B8B);
    REQUIRE(audio_has_nonzero_byte);
    REQUIRE(frame_has_nonzero_byte);
    REQUIRE(audio_hash == "24ce40791e466f9f686ee472b5798128065458e06a51f826666ae444ddfb5c75");
}

TEST_CASE("Display accepts 512 wide RGB frames and emits the matching PPM header", "[frontend]") {
    vanguard8::frontend::Display display;
    std::vector<std::uint8_t> frame(
        static_cast<std::size_t>(
            vanguard8::core::video::V9938::max_visible_width *
            vanguard8::frontend::Display::frame_height * 3
        ),
        0x5A
    );

    display.upload_frame(frame);

    REQUIRE(display.frame_width() == vanguard8::core::video::V9938::max_visible_width);
    REQUIRE(display.uploaded_frame_height() == vanguard8::frontend::Display::frame_height);
    REQUIRE(display.dump_ppm_string().rfind("P6\n512 212\n255\n", 0) == 0);
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

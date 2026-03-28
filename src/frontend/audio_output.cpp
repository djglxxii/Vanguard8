#include "frontend/audio_output.hpp"

#include <SDL.h>

namespace vanguard8::frontend {

auto AudioQueuePump::pump(
    AudioOutputDevice& device,
    const std::vector<std::uint8_t>& pcm_bytes,
    std::string& error
) const -> bool {
    if (pcm_bytes.empty()) {
        return true;
    }
    if (device.queued_bytes() >= max_queue_bytes_) {
        return true;
    }
    return device.queue_audio(pcm_bytes, error);
}

SdlAudioOutputDevice::~SdlAudioOutputDevice() { close(); }

auto SdlAudioOutputDevice::open(const AudioDeviceConfig& config, std::string& error) -> bool {
    close();

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        error = SDL_GetError();
        return false;
    }
    audio_initialized_ = true;

    SDL_AudioSpec desired{};
    desired.freq = config.sample_rate;
    desired.format = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples = 1024;
    desired.callback = nullptr;

    SDL_AudioSpec obtained{};
    device_id_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (device_id_ == 0U) {
        error = SDL_GetError();
        close();
        return false;
    }

    SDL_PauseAudioDevice(device_id_, 0);
    return true;
}

auto SdlAudioOutputDevice::queue_audio(const std::vector<std::uint8_t>& pcm_bytes, std::string& error) -> bool {
    if (device_id_ == 0U) {
        error = "SDL audio device is not open.";
        return false;
    }
    if (pcm_bytes.empty()) {
        return true;
    }
    if (SDL_QueueAudio(device_id_, pcm_bytes.data(), static_cast<Uint32>(pcm_bytes.size())) != 0) {
        error = SDL_GetError();
        return false;
    }
    return true;
}

auto SdlAudioOutputDevice::queued_bytes() const -> std::size_t {
    if (device_id_ == 0U) {
        return 0;
    }
    return static_cast<std::size_t>(SDL_GetQueuedAudioSize(device_id_));
}

void SdlAudioOutputDevice::close() {
    if (device_id_ != 0U) {
        SDL_ClearQueuedAudio(device_id_);
        SDL_CloseAudioDevice(device_id_);
        device_id_ = 0;
    }
    if (audio_initialized_) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        audio_initialized_ = false;
    }
}

}  // namespace vanguard8::frontend

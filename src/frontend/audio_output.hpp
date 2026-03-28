#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace vanguard8::frontend {

struct AudioDeviceConfig {
    int sample_rate = 48'000;
    std::size_t max_queue_bytes = 16'384;
};

class AudioOutputDevice {
  public:
    virtual ~AudioOutputDevice() = default;

    [[nodiscard]] virtual auto open(const AudioDeviceConfig& config, std::string& error) -> bool = 0;
    [[nodiscard]] virtual auto queue_audio(const std::vector<std::uint8_t>& pcm_bytes, std::string& error) -> bool = 0;
    [[nodiscard]] virtual auto queued_bytes() const -> std::size_t = 0;
    virtual void close() = 0;
};

class AudioQueuePump {
  public:
    explicit AudioQueuePump(std::size_t max_queue_bytes) : max_queue_bytes_(max_queue_bytes) {}

    [[nodiscard]] auto pump(
        AudioOutputDevice& device,
        const std::vector<std::uint8_t>& pcm_bytes,
        std::string& error
    ) const -> bool;

  private:
    std::size_t max_queue_bytes_ = 0;
};

class SdlAudioOutputDevice final : public AudioOutputDevice {
  public:
    ~SdlAudioOutputDevice() override;

    [[nodiscard]] auto open(const AudioDeviceConfig& config, std::string& error) -> bool override;
    [[nodiscard]] auto queue_audio(const std::vector<std::uint8_t>& pcm_bytes, std::string& error) -> bool override;
    [[nodiscard]] auto queued_bytes() const -> std::size_t override;
    void close() override;

  private:
    unsigned int device_id_ = 0;
    bool audio_initialized_ = false;
};

}  // namespace vanguard8::frontend

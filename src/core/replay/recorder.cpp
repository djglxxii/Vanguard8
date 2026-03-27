#include "core/replay/recorder.hpp"

#include "core/hash.hpp"

#include <stdexcept>

namespace vanguard8::core::replay {

namespace {

class Writer {
  public:
    template <typename T>
    void pod(const T& value) {
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(&value);
        data_.insert(data_.end(), bytes, bytes + sizeof(T));
    }

    void bytes(const std::vector<std::uint8_t>& value) {
        pod<std::uint64_t>(value.size());
        data_.insert(data_.end(), value.begin(), value.end());
    }

    [[nodiscard]] auto finish() && -> std::vector<std::uint8_t> { return std::move(data_); }

  private:
    std::vector<std::uint8_t> data_{};
};

}  // namespace

void Recorder::reset() {
    recording_ = {};
    active_ = false;
}

void Recorder::begin_power_on(const std::vector<std::uint8_t>& rom_image) {
    recording_ = Recording{
        .rom_sha256 = sha256(rom_image),
        .anchor_type = AnchorType::power_on,
    };
    active_ = true;
}

void Recorder::begin_from_save_state(
    const std::vector<std::uint8_t>& rom_image,
    const std::vector<std::uint8_t>& anchor_save_state
) {
    recording_ = Recording{
        .rom_sha256 = sha256(rom_image),
        .anchor_type = AnchorType::embedded_save_state,
        .anchor_save_state = anchor_save_state,
    };
    active_ = true;
}

void Recorder::record_frame(const std::uint32_t frame_number, const io::ControllerPorts& controller_ports) {
    record_frame(frame_number, controller_ports.state_snapshot());
}

void Recorder::record_frame(const std::uint32_t frame_number, const io::ControllerPortsState& controller_state) {
    if (!active_) {
        throw std::logic_error("Replay recording has not been started.");
    }

    if (frame_number != recording_.frames.size()) {
        throw std::logic_error("Replay frames must be recorded in contiguous order.");
    }

    recording_.frames.push_back(FrameRecord{
        .frame_number = frame_number,
        .controller1 = controller_state.port_state[0],
        .controller2 = controller_state.port_state[1],
    });
}

auto Recorder::serialize() const -> std::vector<std::uint8_t> {
    if (!active_) {
        throw std::logic_error("Replay recording has not been started.");
    }

    Writer writer;
    writer.pod(magic);
    writer.pod(format_version);
    writer.pod(recording_.rom_sha256);
    writer.pod(static_cast<std::uint8_t>(recording_.anchor_type));
    writer.pod(static_cast<std::uint32_t>(recording_.frames.size()));
    if (recording_.anchor_type == AnchorType::embedded_save_state) {
        writer.bytes(recording_.anchor_save_state);
    }
    for (const auto& frame : recording_.frames) {
        writer.pod(frame.frame_number);
        writer.pod(frame.controller1);
        writer.pod(frame.controller2);
    }
    return std::move(writer).finish();
}

auto Recorder::recording() const -> const Recording& { return recording_; }

}  // namespace vanguard8::core::replay

#include "core/replay/replayer.hpp"

#include "core/hash.hpp"
#include "core/replay/recorder.hpp"

#include <cstring>
#include <stdexcept>

namespace vanguard8::core::replay {

namespace {

class Reader {
  public:
    explicit Reader(const std::vector<std::uint8_t>& bytes) : bytes_(bytes) {}

    template <typename T>
    auto pod() -> T {
        if (offset_ + sizeof(T) > bytes_.size()) {
            throw std::runtime_error("Replay truncated while reading POD value.");
        }
        T value{};
        std::memcpy(&value, bytes_.data() + offset_, sizeof(T));
        offset_ += sizeof(T);
        return value;
    }

    auto bytes() -> std::vector<std::uint8_t> {
        const auto size = pod<std::uint64_t>();
        if (offset_ + size > bytes_.size()) {
            throw std::runtime_error("Replay truncated while reading byte payload.");
        }
        std::vector<std::uint8_t> value(bytes_.begin() + static_cast<std::ptrdiff_t>(offset_),
                                        bytes_.begin() + static_cast<std::ptrdiff_t>(offset_ + size));
        offset_ += size;
        return value;
    }

    void expect_magic(const std::array<std::uint8_t, 4>& expected) {
        const auto actual = pod<std::array<std::uint8_t, 4>>();
        if (actual != expected) {
            throw std::runtime_error("Replay magic mismatch.");
        }
    }

    void expect_end() const {
        if (offset_ != bytes_.size()) {
            throw std::runtime_error("Replay has trailing bytes.");
        }
    }

  private:
    const std::vector<std::uint8_t>& bytes_;
    std::size_t offset_ = 0;
};

}  // namespace

void Replayer::reset() { recording_ = {}; }

void Replayer::load(const std::vector<std::uint8_t>& bytes) {
    Reader reader(bytes);
    reader.expect_magic(Recorder::magic);

    const auto version = reader.pod<std::uint8_t>();
    if (version != Recorder::format_version) {
        throw std::runtime_error("Unsupported replay version.");
    }

    recording_ = {};
    recording_.rom_sha256 = reader.pod<Sha256Digest>();
    const auto anchor_type = reader.pod<std::uint8_t>();
    if (anchor_type > static_cast<std::uint8_t>(AnchorType::embedded_save_state)) {
        throw std::runtime_error("Replay anchor type is invalid.");
    }
    recording_.anchor_type = static_cast<AnchorType>(anchor_type);

    const auto frame_count = reader.pod<std::uint32_t>();
    if (recording_.anchor_type == AnchorType::embedded_save_state) {
        recording_.anchor_save_state = reader.bytes();
    }

    recording_.frames.reserve(frame_count);
    for (std::uint32_t index = 0; index < frame_count; ++index) {
        const auto frame_number = reader.pod<std::uint32_t>();
        if (frame_number != index) {
            throw std::runtime_error("Replay frame records must be contiguous and zero-based.");
        }
        recording_.frames.push_back(FrameRecord{
            .frame_number = frame_number,
            .controller1 = reader.pod<std::uint8_t>(),
            .controller2 = reader.pod<std::uint8_t>(),
        });
    }
    reader.expect_end();
}

auto Replayer::rom_matches(const std::vector<std::uint8_t>& rom_image) const -> bool {
    return sha256(rom_image) == recording_.rom_sha256;
}

auto Replayer::frame_count() const -> std::size_t { return recording_.frames.size(); }

auto Replayer::recording() const -> const Recording& { return recording_; }

auto Replayer::frame(const std::uint32_t frame_number) const -> const FrameRecord& {
    if (frame_number >= recording_.frames.size()) {
        throw std::out_of_range("Replay frame is out of range.");
    }
    return recording_.frames[frame_number];
}

void Replayer::apply_frame(io::ControllerPorts& controller_ports, const std::uint32_t frame_number) const {
    const auto& entry = frame(frame_number);
    controller_ports.load_state(io::ControllerPortsState{
        .port_state = {entry.controller1, entry.controller2},
    });
}

}  // namespace vanguard8::core::replay

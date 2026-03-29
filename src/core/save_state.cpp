#include "core/save_state.hpp"

#include <array>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace vanguard8::core {

namespace {

constexpr std::array<std::uint8_t, 4> magic = {'V', '8', 'S', 'T'};

class Writer {
  public:
    template <typename T>
    void pod(const T& value) {
        static_assert(std::is_trivially_copyable_v<T>);
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(&value);
        data_.insert(data_.end(), bytes, bytes + sizeof(T));
    }

    template <typename T, std::size_t N>
    void array(const std::array<T, N>& value) {
        if constexpr (std::is_trivially_copyable_v<std::array<T, N>>) {
            pod(value);
        } else {
            for (const auto& entry : value) {
                array(entry);
            }
        }
    }

    template <typename T>
    void vector(const std::vector<T>& value) {
        pod<std::uint64_t>(value.size());
        if constexpr (std::is_trivially_copyable_v<T>) {
            const auto* bytes = reinterpret_cast<const std::uint8_t*>(value.data());
            data_.insert(data_.end(), bytes, bytes + sizeof(T) * value.size());
        } else {
            for (const auto& entry : value) {
                pod(entry);
            }
        }
    }

    [[nodiscard]] auto finish() && -> std::vector<std::uint8_t> { return std::move(data_); }

  private:
    std::vector<std::uint8_t> data_{};
};

class Reader {
  public:
    explicit Reader(const std::vector<std::uint8_t>& bytes) : bytes_(bytes) {}

    template <typename T>
    auto pod() -> T {
        static_assert(std::is_trivially_copyable_v<T>);
        if (offset_ + sizeof(T) > bytes_.size()) {
            throw std::runtime_error("Save state truncated while reading POD value.");
        }
        T value{};
        std::memcpy(&value, bytes_.data() + offset_, sizeof(T));
        offset_ += sizeof(T);
        return value;
    }

    template <typename T, std::size_t N>
    auto array() -> std::array<T, N> {
        if constexpr (std::is_trivially_copyable_v<std::array<T, N>>) {
            return pod<std::array<T, N>>();
        } else {
            std::array<T, N> value{};
            for (auto& entry : value) {
                entry = array<typename T::value_type, std::tuple_size_v<T>>();
            }
            return value;
        }
    }

    template <typename T>
    auto vector() -> std::vector<T> {
        const auto size = pod<std::uint64_t>();
        std::vector<T> value(size);
        if constexpr (std::is_trivially_copyable_v<T>) {
            const auto bytes_needed = sizeof(T) * size;
            if (offset_ + bytes_needed > bytes_.size()) {
                throw std::runtime_error("Save state truncated while reading vector.");
            }
            if (!value.empty()) {
                std::memcpy(value.data(), bytes_.data() + offset_, bytes_needed);
            }
            offset_ += bytes_needed;
        } else {
            for (auto& entry : value) {
                entry = pod<T>();
            }
        }
        return value;
    }

    void expect_magic(const std::array<std::uint8_t, 4>& expected) {
        const auto actual = pod<std::array<std::uint8_t, 4>>();
        if (actual != expected) {
            throw std::runtime_error("Save state magic mismatch.");
        }
    }

    void expect_end() {
        if (offset_ != bytes_.size()) {
            throw std::runtime_error("Save state has trailing bytes.");
        }
    }

  private:
    const std::vector<std::uint8_t>& bytes_;
    std::size_t offset_ = 0;
};

void write_vdp_state(Writer& writer, const video::V9938::State& state) {
    writer.array(state.vram);
    writer.array(state.reg);
    writer.array(state.status);
    writer.array(state.palette);
    writer.pod(state.vram_addr);
    writer.pod(state.control_latch);
    writer.pod(state.addr_latch_full);
    writer.pod(state.indirect_register);
    writer.pod(state.status_reg_select);
    writer.pod(state.palette_index);
    writer.pod(state.palette_phase);
    writer.pod(state.command);
    writer.array(state.background_line_buffer);
    writer.array(state.sprite_line_buffer);
    writer.array(state.line_buffer);
}

auto read_vdp_state(Reader& reader) -> video::V9938::State {
    video::V9938::State state{};
    state.vram = reader.pod<decltype(state.vram)>();
    state.reg = reader.pod<decltype(state.reg)>();
    state.status = reader.pod<decltype(state.status)>();
    state.palette = reader.pod<decltype(state.palette)>();
    state.vram_addr = reader.pod<std::uint16_t>();
    state.control_latch = reader.pod<std::uint8_t>();
    state.addr_latch_full = reader.pod<bool>();
    state.indirect_register = reader.pod<std::uint8_t>();
    state.status_reg_select = reader.pod<std::uint8_t>();
    state.palette_index = reader.pod<std::uint8_t>();
    state.palette_phase = reader.pod<std::uint8_t>();
    state.command = reader.pod<video::V9938::CommandStateSnapshot>();
    state.background_line_buffer = reader.pod<decltype(state.background_line_buffer)>();
    state.sprite_line_buffer = reader.pod<decltype(state.sprite_line_buffer)>();
    state.line_buffer = reader.pod<decltype(state.line_buffer)>();
    return state;
}

}  // namespace

auto SaveState::serialize(const Emulator& emulator) -> std::vector<std::uint8_t> {
    const auto state = emulator.state_snapshot(format_version);

    Writer writer;
    writer.pod(magic);
    writer.pod(state.format_version);

    writer.vector(state.bus.cartridge_image);
    writer.pod(state.bus.sram_bytes);
    writer.pod(state.bus.int0_state);
    writer.pod(state.bus.int1_pending_count);
    writer.pod(state.bus.controller_ports);
    write_vdp_state(writer, state.bus.vdp_a);
    write_vdp_state(writer, state.bus.vdp_b);
    writer.pod(state.bus.ym2151);
    writer.pod(state.bus.ay8910);
    writer.pod(state.bus.msm5205);
    writer.pod(state.bus.audio_mixer);

    writer.pod(state.cpu);
    writer.pod(state.scheduler.next_sequence);
    writer.vector(state.scheduler.events);
    writer.pod(state.master_cycle);
    writer.pod(state.cpu_tstates);
    writer.pod(state.cpu_master_remainder);
    writer.pod(state.cpu_execution_budget_master_cycles);
    writer.pod(state.completed_frames);
    writer.pod(state.frame_start_cycle);
    writer.pod(state.next_vclk_tick);
    writer.pod(state.current_scanline);
    writer.pod(state.paused);
    writer.vector(state.event_log);
    writer.pod(state.loaded_rom_size);

    return std::move(writer).finish();
}

void SaveState::load(Emulator& emulator, const std::vector<std::uint8_t>& bytes) {
    Reader reader(bytes);
    reader.expect_magic(magic);

    const auto version = reader.pod<std::uint32_t>();
    if (version != 1 && version != format_version) {
        throw std::runtime_error("Unsupported save state version.");
    }

    EmulatorState state;
    state.format_version = version;
    state.bus.cartridge_image = reader.vector<std::uint8_t>();
    state.bus.sram_bytes = reader.pod<decltype(state.bus.sram_bytes)>();
    state.bus.int0_state = reader.pod<Int0State>();
    state.bus.int1_pending_count = reader.pod<std::uint32_t>();
    state.bus.controller_ports = reader.pod<io::ControllerPortsState>();
    state.bus.vdp_a = read_vdp_state(reader);
    state.bus.vdp_b = read_vdp_state(reader);
    state.bus.ym2151 = reader.pod<audio::Ym2151State>();
    state.bus.ay8910 = reader.pod<audio::Ay8910State>();
    state.bus.msm5205 = reader.pod<audio::Msm5205State>();
    state.bus.audio_mixer = reader.pod<audio::AudioMixerState>();

    state.cpu = reader.pod<cpu::CpuStateSnapshot>();
    state.scheduler.next_sequence = reader.pod<std::uint64_t>();
    state.scheduler.events = reader.vector<Event>();
    state.master_cycle = reader.pod<std::uint64_t>();
    state.cpu_tstates = reader.pod<std::uint64_t>();
    state.cpu_master_remainder = reader.pod<std::uint64_t>();
    if (version >= 2) {
        state.cpu_execution_budget_master_cycles = reader.pod<std::uint64_t>();
    }
    state.completed_frames = reader.pod<std::uint64_t>();
    state.frame_start_cycle = reader.pod<std::uint64_t>();
    state.next_vclk_tick = reader.pod<std::uint64_t>();
    state.current_scanline = reader.pod<int>();
    state.paused = reader.pod<bool>();
    state.event_log = reader.vector<EventLogEntry>();
    state.loaded_rom_size = reader.pod<std::size_t>();
    reader.expect_end();

    emulator.load_state(state);
}

}  // namespace vanguard8::core

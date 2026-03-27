#pragma once

#include "core/audio/audio_mixer.hpp"
#include "core/audio/ay8910.hpp"
#include "core/audio/msm5205_adapter.hpp"
#include "core/audio/ym2151.hpp"
#include "core/video/v9938.hpp"
#include "core/io/controller.hpp"
#include "core/memory/cartridge.hpp"
#include "core/memory/sram.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace vanguard8::core {

struct Int0State {
    bool vdp_a_vblank = false;
    bool vdp_a_hblank = false;
    bool ym2151_timer = false;
};

struct PortStubState {
    std::string name;
    bool readable = false;
    bool writable = false;
    std::uint8_t read_value = 0xFF;
    std::optional<std::uint8_t> last_written;
};

struct BusState {
    std::vector<std::uint8_t> cartridge_image;
    std::array<std::uint8_t, memory::Sram::size> sram_bytes{};
    Int0State int0_state{};
    std::uint32_t int1_pending_count = 0;
    io::ControllerPortsState controller_ports{};
    video::V9938::State vdp_a{};
    video::V9938::State vdp_b{};
    audio::Ym2151State ym2151{};
    audio::Ay8910State ay8910{};
    audio::Msm5205State msm5205{};
    audio::AudioMixerState audio_mixer{};
};

class Bus {
  public:
    static constexpr std::uint8_t open_bus_value = 0xFF;

    Bus();
    Bus(memory::CartridgeSlot cartridge, memory::Sram sram = {});

    [[nodiscard]] auto read_memory(std::uint32_t physical_address) -> std::uint8_t;
    void write_memory(std::uint32_t physical_address, std::uint8_t value);

    [[nodiscard]] auto read_port(std::uint16_t port) -> std::uint8_t;
    void write_port(std::uint16_t port, std::uint8_t value);

    [[nodiscard]] auto cartridge() const -> const memory::CartridgeSlot&;
    [[nodiscard]] auto sram() const -> const memory::Sram&;
    [[nodiscard]] auto mutable_sram() -> memory::Sram&;
    [[nodiscard]] auto warnings() const -> const std::vector<std::string>&;
    [[nodiscard]] auto port_stub(std::uint16_t port) const -> const PortStubState*;
    [[nodiscard]] auto int0_state() const -> const Int0State&;
    [[nodiscard]] auto int0_asserted() const -> bool;
    [[nodiscard]] auto int1_asserted() const -> bool;
    [[nodiscard]] auto controller_ports() const -> const io::ControllerPorts&;
    [[nodiscard]] auto mutable_controller_ports() -> io::ControllerPorts&;
    [[nodiscard]] auto vdp_a() const -> const video::V9938&;
    [[nodiscard]] auto vdp_b() const -> const video::V9938&;
    [[nodiscard]] auto mutable_vdp_a() -> video::V9938&;
    [[nodiscard]] auto mutable_vdp_b() -> video::V9938&;
    [[nodiscard]] auto ym2151() const -> const audio::Ym2151&;
    [[nodiscard]] auto ay8910() const -> const audio::Ay8910&;
    [[nodiscard]] auto msm5205() const -> const audio::Msm5205Adapter&;
    [[nodiscard]] auto audio_mixer() const -> const audio::AudioMixer&;
    [[nodiscard]] auto mutable_ym2151() -> audio::Ym2151&;
    [[nodiscard]] auto mutable_ay8910() -> audio::Ay8910&;
    [[nodiscard]] auto mutable_msm5205() -> audio::Msm5205Adapter&;
    [[nodiscard]] auto mutable_audio_mixer() -> audio::AudioMixer&;
    [[nodiscard]] auto state_snapshot() const -> BusState;
    void load_state(const BusState& state);
    void run_audio(std::uint64_t master_cycles);
    void end_audio_frame();
    void trigger_msm5205_vclk();
    void sync_vdp_interrupt_lines();
    void set_ym2151_timer(bool asserted);
    void set_int1(bool asserted);
    void record_warning(std::string message);

  private:
    memory::CartridgeSlot cartridge_;
    memory::Sram sram_;
    std::unordered_map<std::uint16_t, PortStubState> port_stubs_;
    std::vector<std::string> warnings_;
    Int0State int0_state_{};
    std::uint32_t int1_pending_count_ = 0;
    io::ControllerPorts controller_ports_{};
    video::V9938 vdp_a_{};
    video::V9938 vdp_b_{};
    audio::Ym2151 ym2151_{};
    audio::Ay8910 ay8910_{};
    audio::Msm5205Adapter msm5205_{};
    audio::AudioMixer audio_mixer_{};

    void register_port(
        std::uint16_t port,
        std::string name,
        bool readable,
        bool writable,
        std::uint8_t read_value = open_bus_value
    );
    void warn(std::string message);
};

}  // namespace vanguard8::core

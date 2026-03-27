#include "core/bus.hpp"

#include <iomanip>
#include <sstream>

namespace vanguard8::core {

namespace {

[[nodiscard]] auto hex_string(const std::uint32_t value, const int width) -> std::string {
    std::ostringstream stream;
    stream << "0x" << std::uppercase << std::hex << std::setw(width) << std::setfill('0') << value;
    return stream.str();
}

}  // namespace

Bus::Bus() : Bus(memory::CartridgeSlot{}, memory::Sram{}) {}

Bus::Bus(memory::CartridgeSlot cartridge, memory::Sram sram)
    : cartridge_(std::move(cartridge)), sram_(std::move(sram)) {
    register_port(0x00, "Controller 1", true, false);
    register_port(0x01, "Controller 2", true, false);

    register_port(0x40, "YM2151 address/status", true, true);
    register_port(0x41, "YM2151 data", false, true);

    register_port(0x50, "AY-3-8910 address latch", false, true);
    register_port(0x51, "AY-3-8910 data", true, true);

    register_port(0x60, "MSM5205 control", false, true);
    register_port(0x61, "MSM5205 data", false, true);

    register_port(0x80, "VDP-A VRAM data", true, true);
    register_port(0x81, "VDP-A status/command", true, true);
    register_port(0x82, "VDP-A palette", false, true);
    register_port(0x83, "VDP-A register indirect", false, true);

    register_port(0x84, "VDP-B VRAM data", true, true);
    register_port(0x85, "VDP-B status/command", true, true);
    register_port(0x86, "VDP-B palette", false, true);
    register_port(0x87, "VDP-B register indirect", false, true);
}

auto Bus::read_memory(const std::uint32_t physical_address) -> std::uint8_t {
    if (memory::CartridgeSlot::contains_physical_address(physical_address)) {
        return cartridge_.read_physical(physical_address);
    }

    if (memory::Sram::contains_physical_address(physical_address)) {
        return sram_.read(physical_address);
    }

    warn("Open-bus memory read at " + hex_string(physical_address, 5));
    return open_bus_value;
}

void Bus::write_memory(const std::uint32_t physical_address, const std::uint8_t value) {
    if (memory::CartridgeSlot::contains_physical_address(physical_address)) {
        warn(
            "Ignored write to cartridge ROM at " + hex_string(physical_address, 5) + " value " +
            hex_string(value, 2)
        );
        return;
    }

    if (memory::Sram::contains_physical_address(physical_address)) {
        sram_.write(physical_address, value);
        return;
    }

    warn(
        "Open-bus memory write at " + hex_string(physical_address, 5) + " value " +
        hex_string(value, 2)
    );
}

auto Bus::read_port(const std::uint16_t port) -> std::uint8_t {
    if (port == 0x00 || port == 0x01) {
        return controller_ports_.read_port(port);
    }

    switch (port) {
    case 0x40: {
        const auto value = ym2151_.read_status();
        set_ym2151_timer(ym2151_.irq_pending());
        return value;
    }
    case 0x51:
        return ay8910_.read_data();
    case 0x80:
        return vdp_a_.read_data();
    case 0x81: {
        const auto value = vdp_a_.read_status();
        sync_vdp_interrupt_lines();
        return value;
    }
    case 0x84:
        return vdp_b_.read_data();
    case 0x85:
        return vdp_b_.read_status();
    default:
        break;
    }

    const auto stub = port_stubs_.find(port);
    if (stub == port_stubs_.end()) {
        warn("Open-bus port read at " + hex_string(port, 2));
        return open_bus_value;
    }

    if (!stub->second.readable) {
        warn("Unsupported read from write-only port " + hex_string(port, 2));
        return open_bus_value;
    }

    return stub->second.read_value;
}

void Bus::write_port(const std::uint16_t port, const std::uint8_t value) {
    if (const auto stub = port_stubs_.find(port); stub != port_stubs_.end()) {
        stub->second.last_written = value;
    }

    switch (port) {
    case 0x40:
        ym2151_.write_address(value);
        return;
    case 0x41:
        ym2151_.write_data(value);
        set_ym2151_timer(ym2151_.irq_pending());
        return;
    case 0x50:
        ay8910_.write_address(value);
        return;
    case 0x51:
        ay8910_.write_data(value);
        return;
    case 0x60:
        msm5205_.write_control(value);
        if (!msm5205_.vclk_enabled()) {
            int1_pending_count_ = 0;
        }
        return;
    case 0x61:
        msm5205_.write_data(value);
        return;
    case 0x80:
        vdp_a_.write_data(value);
        return;
    case 0x81:
        vdp_a_.write_control(value);
        sync_vdp_interrupt_lines();
        return;
    case 0x82:
        vdp_a_.write_palette(value);
        return;
    case 0x83:
        vdp_a_.write_register(value);
        sync_vdp_interrupt_lines();
        return;
    case 0x84:
        vdp_b_.write_data(value);
        return;
    case 0x85:
        vdp_b_.write_control(value);
        return;
    case 0x86:
        vdp_b_.write_palette(value);
        return;
    case 0x87:
        vdp_b_.write_register(value);
        return;
    default:
        break;
    }

    const auto stub = port_stubs_.find(port);
    if (stub == port_stubs_.end()) {
        warn("Open-bus port write at " + hex_string(port, 2) + " value " + hex_string(value, 2));
        return;
    }

    if (!stub->second.writable) {
        warn("Unsupported write to read-only port " + hex_string(port, 2));
        return;
    }

    stub->second.last_written = value;
}

auto Bus::cartridge() const -> const memory::CartridgeSlot& { return cartridge_; }

auto Bus::sram() const -> const memory::Sram& { return sram_; }

auto Bus::mutable_sram() -> memory::Sram& { return sram_; }

auto Bus::warnings() const -> const std::vector<std::string>& { return warnings_; }

auto Bus::port_stub(const std::uint16_t port) const -> const PortStubState* {
    const auto iterator = port_stubs_.find(port);
    return iterator == port_stubs_.end() ? nullptr : &iterator->second;
}

auto Bus::int0_state() const -> const Int0State& { return int0_state_; }

auto Bus::int0_asserted() const -> bool {
    return int0_state_.vdp_a_vblank || int0_state_.vdp_a_hblank || int0_state_.ym2151_timer;
}

auto Bus::int1_asserted() const -> bool { return int1_pending_count_ > 0U; }

auto Bus::controller_ports() const -> const io::ControllerPorts& { return controller_ports_; }

auto Bus::mutable_controller_ports() -> io::ControllerPorts& { return controller_ports_; }

auto Bus::vdp_a() const -> const video::V9938& { return vdp_a_; }

auto Bus::vdp_b() const -> const video::V9938& { return vdp_b_; }

auto Bus::mutable_vdp_a() -> video::V9938& { return vdp_a_; }

auto Bus::mutable_vdp_b() -> video::V9938& { return vdp_b_; }

auto Bus::ym2151() const -> const audio::Ym2151& { return ym2151_; }

auto Bus::ay8910() const -> const audio::Ay8910& { return ay8910_; }

auto Bus::msm5205() const -> const audio::Msm5205Adapter& { return msm5205_; }

auto Bus::audio_mixer() const -> const audio::AudioMixer& { return audio_mixer_; }

auto Bus::mutable_ym2151() -> audio::Ym2151& { return ym2151_; }

auto Bus::mutable_ay8910() -> audio::Ay8910& { return ay8910_; }

auto Bus::mutable_msm5205() -> audio::Msm5205Adapter& { return msm5205_; }

auto Bus::mutable_audio_mixer() -> audio::AudioMixer& { return audio_mixer_; }

auto Bus::state_snapshot() const -> BusState {
    return BusState{
        .cartridge_image = cartridge_.rom_image(),
        .sram_bytes = sram_.bytes(),
        .int0_state = int0_state_,
        .int1_pending_count = int1_pending_count_,
        .controller_ports = controller_ports_.state_snapshot(),
        .vdp_a = vdp_a_.state_snapshot(),
        .vdp_b = vdp_b_.state_snapshot(),
        .ym2151 = ym2151_.state_snapshot(),
        .ay8910 = ay8910_.state_snapshot(),
        .msm5205 = msm5205_.state_snapshot(),
        .audio_mixer = audio_mixer_.state_snapshot(),
    };
}

void Bus::load_state(const BusState& state) {
    cartridge_.load(state.cartridge_image);
    sram_.load_bytes(state.sram_bytes);
    int0_state_ = state.int0_state;
    int1_pending_count_ = state.int1_pending_count;
    controller_ports_.load_state(state.controller_ports);
    vdp_a_.load_state(state.vdp_a);
    vdp_b_.load_state(state.vdp_b);
    ym2151_.load_state(state.ym2151);
    ay8910_.load_state(state.ay8910);
    msm5205_.load_state(state.msm5205);
    audio_mixer_.load_state(state.audio_mixer);
}

void Bus::run_audio(const std::uint64_t master_cycles) {
    for (std::uint64_t cycle = 0; cycle < master_cycles; ++cycle) {
        ym2151_.advance_master_cycle();
        ay8910_.advance_master_cycle();

        if (audio_mixer_.advance_common_clock()) {
            const auto ym_sample = ym2151_.current_output();
            const auto ay_mono = ay8910_.consume_common_rate_sample();
            const auto msm_mono = msm5205_.current_output();
            audio_mixer_.set_common_sample(audio::StereoSample{
                .left = ym_sample.left + ay_mono + msm_mono,
                .right = ym_sample.right + ay_mono + msm_mono,
            });
        }

        audio_mixer_.advance_output_clock();
    }

    set_ym2151_timer(ym2151_.irq_pending());
}

void Bus::end_audio_frame() { audio_mixer_.end_frame(); }

void Bus::trigger_msm5205_vclk() {
    if (msm5205_.trigger_vclk()) {
        set_int1(true);
    }
}

void Bus::sync_vdp_interrupt_lines() {
    int0_state_.vdp_a_vblank = vdp_a_.vblank_irq_pending();
    int0_state_.vdp_a_hblank = vdp_a_.hblank_irq_pending();
}

void Bus::set_ym2151_timer(const bool asserted) { int0_state_.ym2151_timer = asserted; }

void Bus::set_int1(const bool asserted) {
    if (asserted) {
        ++int1_pending_count_;
        return;
    }

    if (int1_pending_count_ > 0U) {
        --int1_pending_count_;
    }
}

void Bus::record_warning(std::string message) { warn(std::move(message)); }

void Bus::register_port(
    const std::uint16_t port,
    std::string name,
    const bool readable,
    const bool writable,
    const std::uint8_t read_value
) {
    port_stubs_.emplace(
        port,
        PortStubState{
            .name = std::move(name),
            .readable = readable,
            .writable = writable,
            .read_value = read_value,
            .last_written = std::nullopt,
        }
    );
}

void Bus::warn(std::string message) { warnings_.push_back(std::move(message)); }

}  // namespace vanguard8::core

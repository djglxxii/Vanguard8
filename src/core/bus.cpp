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

auto Bus::int1_asserted() const -> bool { return int1_asserted_; }

void Bus::set_vdp_a_vblank(const bool asserted) { int0_state_.vdp_a_vblank = asserted; }

void Bus::set_vdp_a_hblank(const bool asserted) { int0_state_.vdp_a_hblank = asserted; }

void Bus::set_ym2151_timer(const bool asserted) { int0_state_.ym2151_timer = asserted; }

void Bus::set_int1(const bool asserted) { int1_asserted_ = asserted; }

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

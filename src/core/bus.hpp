#pragma once

#include "core/memory/cartridge.hpp"
#include "core/memory/sram.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace vanguard8::core {

struct PortStubState {
    std::string name;
    bool readable = false;
    bool writable = false;
    std::uint8_t read_value = 0xFF;
    std::optional<std::uint8_t> last_written;
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

  private:
    memory::CartridgeSlot cartridge_;
    memory::Sram sram_;
    std::unordered_map<std::uint16_t, PortStubState> port_stubs_;
    std::vector<std::string> warnings_;

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

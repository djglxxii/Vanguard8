#pragma once

#include "core/bus.hpp"
#include "third_party/z180/z180_core.hpp"

#include <cstdint>
#include <optional>

namespace vanguard8::core::cpu {

enum class InterruptSource {
    int0,
    int1,
    prt0,
    prt1,
};

enum class DmaChannel {
    channel0,
    channel1,
};

struct InterruptService {
    InterruptSource source;
    std::uint16_t handler_address = 0;
};

class Z180Adapter {
  public:
    explicit Z180Adapter(core::Bus& bus);

    void reset();

    [[nodiscard]] auto pc() const -> std::uint16_t;
    [[nodiscard]] auto accumulator() const -> std::uint8_t;
    [[nodiscard]] auto register_i() const -> std::uint8_t;
    [[nodiscard]] auto register_il() const -> std::uint8_t;
    [[nodiscard]] auto register_itc() const -> std::uint8_t;
    [[nodiscard]] auto cbar() const -> std::uint8_t;
    [[nodiscard]] auto cbr() const -> std::uint8_t;
    [[nodiscard]] auto bbr() const -> std::uint8_t;
    [[nodiscard]] auto interrupt_mode() const -> std::uint8_t;
    [[nodiscard]] auto halted() const -> bool;

    void set_register_i(std::uint8_t value);
    void set_interrupt_mode(std::uint8_t mode);
    void set_iff1(bool enabled);
    void set_iff2(bool enabled);

    [[nodiscard]] auto in0(std::uint8_t port) -> std::uint8_t;
    void out0(std::uint8_t port, std::uint8_t value);
    void execute_dma(DmaChannel channel);
    void advance_tstates(std::uint64_t tstates);

    [[nodiscard]] auto translate_logical_address(std::uint16_t logical_address) const
        -> std::uint32_t;
    [[nodiscard]] auto read_logical(std::uint16_t logical_address) -> std::uint8_t;
    void write_logical(std::uint16_t logical_address, std::uint8_t value);

    [[nodiscard]] auto service_pending_interrupt() -> std::optional<InterruptService>;
    void run_until_halt(std::size_t max_instructions);

  private:
    struct DmaRegisters {
        std::uint8_t sar0l = 0x00;
        std::uint8_t sar0h = 0x00;
        std::uint8_t sar0b = 0x00;
        std::uint8_t dar0l = 0x00;
        std::uint8_t dar0h = 0x00;
        std::uint8_t dar0b = 0x00;
        std::uint8_t bcr0l = 0x00;
        std::uint8_t bcr0h = 0x00;
        std::uint8_t mar1l = 0x00;
        std::uint8_t mar1h = 0x00;
        std::uint8_t mar1b = 0x00;
        std::uint8_t iar1l = 0x00;
        std::uint8_t iar1h = 0x00;
        std::uint8_t bcr1l = 0x00;
        std::uint8_t bcr1h = 0x00;
        std::uint8_t dstat = 0x00;
        std::uint8_t dmode = 0x00;
        std::uint8_t dcntl = 0x00;
    };

    core::Bus& bus_;
    third_party::z180::Core core_;
    DmaRegisters dma_{};

    [[nodiscard]] auto read_dma_register(std::uint8_t port) const -> std::optional<std::uint8_t>;
    auto write_dma_register(std::uint8_t port, std::uint8_t value) -> bool;
    [[nodiscard]] auto dma_channel0_source() const -> std::uint32_t;
    [[nodiscard]] auto dma_channel0_destination() const -> std::uint32_t;
    [[nodiscard]] auto dma_channel0_length() const -> std::uint16_t;
    [[nodiscard]] auto dma_channel1_memory_address() const -> std::uint32_t;
    [[nodiscard]] auto dma_channel1_port() const -> std::uint16_t;
    [[nodiscard]] auto dma_channel1_length() const -> std::uint16_t;
    auto dma_mode_supported(DmaChannel channel) -> bool;
};

}  // namespace vanguard8::core::cpu

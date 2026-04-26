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

enum class BreakpointType {
    pc,
    memory_read,
    memory_write,
    io_read,
    io_write,
};

struct Breakpoint {
    std::size_t id = 0;
    BreakpointType type = BreakpointType::pc;
    std::uint16_t address = 0;
    std::optional<std::uint8_t> value;
    bool enabled = true;
};

struct BreakpointHit {
    Breakpoint breakpoint;
    std::uint16_t pc = 0;
    std::uint16_t address = 0;
    std::optional<std::uint8_t> value;
};

struct BreakpointRunResult {
    bool halted = false;
    bool breakpoint_hit = false;
    std::size_t executed_instructions = 0;
    std::optional<BreakpointHit> hit;
};

struct BankSwitchEvent {
    std::uint64_t sequence = 0;
    std::uint16_t pc = 0;
    std::uint8_t bbr = 0;
    int bank = -1;
    bool legal = true;
};

struct DmaChannel0Snapshot {
    std::uint32_t source = 0;
    std::uint32_t destination = 0;
    std::uint16_t length = 0;
};

struct DmaChannel1Snapshot {
    std::uint32_t memory_address = 0;
    std::uint16_t port = 0;
    std::uint16_t length = 0;
};

struct CpuStateSnapshot {
    third_party::z180::RegisterSnapshot registers;
    DmaChannel0Snapshot dma0;
    DmaChannel1Snapshot dma1;
    std::uint8_t dstat = 0;
    std::uint8_t dmode = 0;
    std::uint8_t dcntl = 0;
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
    [[nodiscard]] auto state_snapshot() const -> CpuStateSnapshot;
    void load_state_snapshot(const CpuStateSnapshot& state);
    [[nodiscard]] auto breakpoints() const -> const std::vector<Breakpoint>&;
    [[nodiscard]] auto breakpoint_hits() const -> const std::vector<BreakpointHit>&;
    [[nodiscard]] auto bank_switch_log() const -> const std::vector<BankSwitchEvent>&;

    void set_register_i(std::uint8_t value);
    void set_interrupt_mode(std::uint8_t mode);
    void set_iff1(bool enabled);
    void set_iff2(bool enabled);
    auto add_breakpoint(Breakpoint breakpoint) -> std::size_t;
    void clear_breakpoints();
    void clear_breakpoint_hits();
    void clear_bank_switch_log();

    [[nodiscard]] auto in0(std::uint8_t port) -> std::uint8_t;
    void out0(std::uint8_t port, std::uint8_t value);
    void execute_dma(DmaChannel channel);
    void advance_tstates(std::uint64_t tstates);
    [[nodiscard]] auto next_scheduled_tstates() const -> std::uint64_t;
    [[nodiscard]] auto step_scheduled_instruction() -> std::uint64_t;

    [[nodiscard]] auto translate_logical_address(std::uint16_t logical_address) const
        -> std::uint32_t;
    [[nodiscard]] auto peek_logical(std::uint16_t logical_address) const -> std::uint8_t;
    [[nodiscard]] auto read_logical(std::uint16_t logical_address) -> std::uint8_t;
    void write_logical(std::uint16_t logical_address, std::uint8_t value);

    [[nodiscard]] auto service_pending_interrupt() -> std::optional<InterruptService>;
    void step_instruction();
    void run_until_halt(std::size_t max_instructions);
    [[nodiscard]] auto run_until_breakpoint_or_halt(std::size_t max_instructions) -> BreakpointRunResult;

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
    std::vector<Breakpoint> breakpoints_{};
    std::vector<BreakpointHit> breakpoint_hits_{};
    std::vector<BankSwitchEvent> bank_switch_log_{};
    std::size_t next_breakpoint_id_ = 1;
    std::uint64_t bank_switch_sequence_ = 0;
    std::optional<BreakpointHit> pending_breakpoint_hit_{};

    [[nodiscard]] auto read_dma_register(std::uint8_t port) const -> std::optional<std::uint8_t>;
    auto write_dma_register(std::uint8_t port, std::uint8_t value) -> bool;
    [[nodiscard]] auto dma_channel0_source() const -> std::uint32_t;
    [[nodiscard]] auto dma_channel0_destination() const -> std::uint32_t;
    [[nodiscard]] auto dma_channel0_length() const -> std::uint16_t;
    [[nodiscard]] auto dma_channel1_memory_address() const -> std::uint32_t;
    [[nodiscard]] auto dma_channel1_port() const -> std::uint16_t;
    [[nodiscard]] auto dma_channel1_length() const -> std::uint16_t;
    auto dma_mode_supported(DmaChannel channel) -> bool;
    [[nodiscard]] auto next_interrupt_service_source() const -> std::optional<InterruptSource>;
    [[nodiscard]] auto interrupt_service_tstates(InterruptSource source) const -> std::uint64_t;
    void record_breakpoint_hit(
        BreakpointType type,
        std::uint16_t address,
        std::optional<std::uint8_t> value = std::nullopt
    );
    void record_bank_switch(std::uint8_t value);
};

}  // namespace vanguard8::core::cpu

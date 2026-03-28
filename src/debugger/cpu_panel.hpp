#pragma once

#include "core/cpu/z180_adapter.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace vanguard8::core {
class Emulator;
class SymbolTable;
}

namespace vanguard8::debugger {

enum class ExecutionCommand {
    pause,
    resume,
    step_frame,
};

struct DecodedInstruction {
    std::uint16_t address = 0;
    std::array<std::uint8_t, 4> bytes{};
    std::size_t length = 0;
    std::string mnemonic;
    std::string address_label;
    bool current_pc = false;
};

struct FlagState {
    bool sign = false;
    bool zero = false;
    bool half_carry = false;
    bool parity_overflow = false;
    bool subtract = false;
    bool carry = false;
};

struct MmuMapSummary {
    std::uint16_t ca0_end = 0;
    std::uint16_t bank_start = 0;
    std::uint16_t ca1_start = 0;
    std::uint32_t bank_physical_base = 0;
    std::uint32_t ca1_physical_base = 0;
};

struct CpuPanelSnapshot {
    core::cpu::CpuStateSnapshot cpu;
    FlagState flags;
    MmuMapSummary mmu;
    bool emulator_paused = false;
    std::vector<DecodedInstruction> disassembly;
    std::vector<core::cpu::Breakpoint> breakpoints;
    std::vector<core::cpu::BreakpointHit> breakpoint_hits;
};

class CpuPanel {
  public:
    [[nodiscard]] auto snapshot(const core::Emulator& emulator) const -> CpuPanelSnapshot;
    [[nodiscard]] auto snapshot(const core::Emulator& emulator, const core::SymbolTable& symbols) const
        -> CpuPanelSnapshot;

    void queue_command(ExecutionCommand command);
    void apply_pending(core::Emulator& emulator);
    [[nodiscard]] auto pending_command_count() const -> std::size_t;
    auto add_breakpoint(core::Emulator& emulator, core::cpu::Breakpoint breakpoint) const -> std::size_t;
    void clear_breakpoints(core::Emulator& emulator) const;
    void clear_breakpoint_hits(core::Emulator& emulator) const;
    [[nodiscard]] auto run_until_breakpoint_or_halt(
        core::Emulator& emulator,
        std::size_t max_instructions
    ) const -> core::cpu::BreakpointRunResult;

  private:
    std::vector<ExecutionCommand> pending_commands_;
};

[[nodiscard]] auto decode_instruction(
    const core::cpu::Z180Adapter& cpu,
    std::uint16_t address,
    std::uint16_t current_pc,
    const core::SymbolTable* symbols = nullptr
) -> DecodedInstruction;

}  // namespace vanguard8::debugger

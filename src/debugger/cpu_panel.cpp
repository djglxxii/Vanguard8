#include "debugger/cpu_panel.hpp"

#include "core/emulator.hpp"

#include <iomanip>
#include <sstream>

namespace vanguard8::debugger {

namespace {

constexpr std::uint8_t flag_sign = 0x80;
constexpr std::uint8_t flag_zero = 0x40;
constexpr std::uint8_t flag_half_carry = 0x10;
constexpr std::uint8_t flag_parity_overflow = 0x04;
constexpr std::uint8_t flag_subtract = 0x02;
constexpr std::uint8_t flag_carry = 0x01;

auto hex8(const std::uint8_t value) -> std::string {
    std::ostringstream stream;
    stream << "0x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(value);
    return stream.str();
}

auto hex16(const std::uint16_t value) -> std::string {
    std::ostringstream stream;
    stream << "0x" << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << value;
    return stream.str();
}

}  // namespace

auto decode_instruction(
    const vanguard8::core::cpu::Z180Adapter& cpu,
    const std::uint16_t address,
    const std::uint16_t current_pc
) -> DecodedInstruction {
    DecodedInstruction row;
    row.address = address;
    row.current_pc = address == current_pc;

    const auto byte0 = cpu.peek_logical(address);
    row.bytes[0] = byte0;
    row.length = 1;

    const auto byte1 = cpu.peek_logical(static_cast<std::uint16_t>(address + 1U));
    const auto byte2 = cpu.peek_logical(static_cast<std::uint16_t>(address + 2U));
    row.bytes[1] = byte1;
    row.bytes[2] = byte2;

    switch (byte0) {
    case 0x00:
        row.mnemonic = "NOP";
        break;
    case 0x21:
        row.length = 3;
        row.mnemonic = "LD HL," + hex16(static_cast<std::uint16_t>(byte1 | (byte2 << 8U)));
        break;
    case 0x22:
        row.length = 3;
        row.mnemonic = "LD (" + hex16(static_cast<std::uint16_t>(byte1 | (byte2 << 8U))) + "),HL";
        break;
    case 0x31:
        row.length = 3;
        row.mnemonic = "LD SP," + hex16(static_cast<std::uint16_t>(byte1 | (byte2 << 8U)));
        break;
    case 0x32:
        row.length = 3;
        row.mnemonic = "LD (" + hex16(static_cast<std::uint16_t>(byte1 | (byte2 << 8U))) + "),A";
        break;
    case 0x3A:
        row.length = 3;
        row.mnemonic = "LD A,(" + hex16(static_cast<std::uint16_t>(byte1 | (byte2 << 8U))) + ")";
        break;
    case 0x3E:
        row.length = 2;
        row.mnemonic = "LD A," + hex8(byte1);
        break;
    case 0x76:
        row.mnemonic = "HALT";
        break;
    case 0xC3:
        row.length = 3;
        row.mnemonic = "JP " + hex16(static_cast<std::uint16_t>(byte1 | (byte2 << 8U)));
        break;
    case 0xC9:
        row.mnemonic = "RET";
        break;
    case 0xCD:
        row.length = 3;
        row.mnemonic = "CALL " + hex16(static_cast<std::uint16_t>(byte1 | (byte2 << 8U)));
        break;
    case 0xED: {
        const auto ed = byte1;
        row.length = 2;
        row.bytes[1] = ed;
        switch (ed) {
        case 0x39:
            row.length = 3;
            row.bytes[2] = byte2;
            row.mnemonic = "OUT0 (" + hex8(byte2) + "),A";
            break;
        case 0x47:
            row.mnemonic = "LD I,A";
            break;
        case 0x4D:
            row.mnemonic = "RETI";
            break;
        default:
            row.mnemonic = "DB " + hex8(byte0) + "," + hex8(ed);
            break;
        }
        break;
    }
    case 0xF3:
        row.mnemonic = "DI";
        break;
    case 0xFB:
        row.mnemonic = "EI";
        break;
    default:
        row.mnemonic = "DB " + hex8(byte0);
        break;
    }

    return row;
}

auto CpuPanel::snapshot(const core::Emulator& emulator) const -> CpuPanelSnapshot {
    const auto cpu_state = emulator.cpu().state_snapshot();
    const auto flags = cpu_state.registers.af & 0x00FFU;
    const auto cbar = cpu_state.registers.cbar;
    const auto ca0_end = static_cast<std::uint16_t>((cbar >> 4U) << 12U);
    const auto ca1_start = static_cast<std::uint16_t>((cbar & 0x0FU) << 12U);

    CpuPanelSnapshot snapshot{
        .cpu = cpu_state,
        .flags =
            FlagState{
                .sign = (flags & flag_sign) != 0U,
                .zero = (flags & flag_zero) != 0U,
                .half_carry = (flags & flag_half_carry) != 0U,
                .parity_overflow = (flags & flag_parity_overflow) != 0U,
                .subtract = (flags & flag_subtract) != 0U,
                .carry = (flags & flag_carry) != 0U,
            },
        .mmu =
            MmuMapSummary{
                .ca0_end = ca0_end,
                .bank_start = ca0_end,
                .ca1_start = ca1_start,
                .bank_physical_base = static_cast<std::uint32_t>(cpu_state.registers.bbr) << 12U,
                .ca1_physical_base = static_cast<std::uint32_t>(cpu_state.registers.cbr) << 12U,
            },
        .emulator_paused = emulator.paused(),
        .breakpoints = emulator.cpu().breakpoints(),
        .breakpoint_hits = emulator.cpu().breakpoint_hits(),
    };

    std::uint16_t pc = cpu_state.registers.pc;
    snapshot.disassembly.reserve(20);
    for (std::size_t index = 0; index < 20; ++index) {
        const auto row = decode_instruction(emulator.cpu(), pc, cpu_state.registers.pc);
        snapshot.disassembly.push_back(row);
        pc = static_cast<std::uint16_t>(pc + row.length);
    }

    return snapshot;
}

void CpuPanel::queue_command(const ExecutionCommand command) { pending_commands_.push_back(command); }

void CpuPanel::apply_pending(core::Emulator& emulator) {
    for (const auto command : pending_commands_) {
        switch (command) {
        case ExecutionCommand::pause:
            emulator.pause();
            break;
        case ExecutionCommand::resume:
            emulator.resume();
            break;
        case ExecutionCommand::step_frame:
            emulator.step_frame();
            break;
        }
    }
    pending_commands_.clear();
}

auto CpuPanel::pending_command_count() const -> std::size_t { return pending_commands_.size(); }

auto CpuPanel::add_breakpoint(core::Emulator& emulator, core::cpu::Breakpoint breakpoint) const
    -> std::size_t {
    return emulator.mutable_cpu().add_breakpoint(std::move(breakpoint));
}

void CpuPanel::clear_breakpoints(core::Emulator& emulator) const { emulator.mutable_cpu().clear_breakpoints(); }

void CpuPanel::clear_breakpoint_hits(core::Emulator& emulator) const {
    emulator.mutable_cpu().clear_breakpoint_hits();
}

auto CpuPanel::run_until_breakpoint_or_halt(core::Emulator& emulator, const std::size_t max_instructions) const
    -> core::cpu::BreakpointRunResult {
    return emulator.mutable_cpu().run_until_breakpoint_or_halt(max_instructions);
}

}  // namespace vanguard8::debugger

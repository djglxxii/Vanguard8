#include "debugger/trace_panel.hpp"

#include "core/emulator.hpp"
#include "debugger/cpu_panel.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace vanguard8::debugger {

namespace {

auto hex16(const std::uint16_t value) -> std::string {
    std::ostringstream stream;
    stream << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << value;
    return stream.str();
}

auto hex20(const std::uint32_t value) -> std::string {
    std::ostringstream stream;
    stream << std::uppercase << std::hex << std::setw(5) << std::setfill('0') << (value & 0xFFFFFU);
    return stream.str();
}

auto bytes_string(const DecodedInstruction& row) -> std::string {
    std::ostringstream stream;
    stream << std::uppercase << std::hex << std::setfill('0');
    for (std::size_t index = 0; index < row.length; ++index) {
        if (index != 0) {
            stream << ' ';
        }
        stream << std::setw(2) << static_cast<int>(row.bytes[index]);
    }
    return stream.str();
}

auto bank_label(const core::cpu::Z180Adapter& cpu, const std::uint16_t pc) -> std::string {
    if (pc < 0x4000U) {
        return "F";
    }
    if (pc >= 0x8000U) {
        return "S";
    }

    const auto bbr = cpu.bbr();
    if (bbr < 0x04U || bbr >= 0xF0U) {
        return "?";
    }

    return std::to_string(static_cast<int>(bbr / 4U) - 1);
}

}  // namespace

auto TracePanel::write_to_file(
    core::Emulator& emulator,
    const std::filesystem::path& path,
    const std::size_t max_instructions
) const -> TraceWriteResult {
    if (max_instructions == 0U) {
        throw std::invalid_argument("Trace instruction budget must be greater than zero.");
    }

    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Unable to open trace output file.");
    }

    output << "STEP         PC    PHYS   BANK  BYTES        MNEMONIC\n";

    TraceWriteResult result;
    for (; result.line_count < max_instructions && !emulator.cpu().halted(); ++result.line_count) {
        const auto pc = emulator.cpu().pc();
        const auto row = decode_instruction(emulator.cpu(), pc, pc);
        const auto physical = emulator.cpu().translate_logical_address(pc);

        output << std::dec << std::setw(12) << std::setfill('0') << result.line_count << "  "
               << hex16(pc) << "  " << hex20(physical) << "  " << std::setw(4) << std::setfill(' ')
               << bank_label(emulator.cpu(), pc) << "  " << std::left << std::setw(11)
               << bytes_string(row) << std::right << "  " << row.mnemonic << '\n';

        emulator.mutable_cpu().step_instruction();
    }

    result.halted = emulator.cpu().halted();
    return result;
}

auto format_trace_runtime_summary(const core::Emulator& emulator, const std::string_view rom_label) -> std::string {
    std::ostringstream stream;
    stream << "Runtime status: " << rom_label << '\n';
    stream << "Frame count: " << emulator.completed_frames() << '\n';
    stream << "Scanline: " << emulator.current_scanline() << '\n';
    stream << "Master cycle: " << emulator.master_cycle() << '\n';
    stream << "CPU t-states: " << emulator.cpu_tstates() << '\n';
    stream << "VCLK rate: " << core::Emulator::vclk_rate_name(emulator.vclk_rate()) << '\n';
    stream << "Event log size: " << emulator.event_log().size() << '\n';
    stream << "Loaded ROM size: " << emulator.loaded_rom_size() << '\n';
    return stream.str();
}

}  // namespace vanguard8::debugger

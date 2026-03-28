#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

namespace vanguard8::core {
class Emulator;
class SymbolTable;
}

namespace vanguard8::debugger {

struct TraceWriteResult {
    std::size_t line_count = 0;
    bool halted = false;
};

class TracePanel {
  public:
    [[nodiscard]] auto write_to_file(
        core::Emulator& emulator,
        const std::filesystem::path& path,
        std::size_t max_instructions,
        const core::SymbolTable* symbols = nullptr
    ) const -> TraceWriteResult;
};

[[nodiscard]] auto format_trace_runtime_summary(
    const core::Emulator& emulator,
    std::string_view rom_label
) -> std::string;

}  // namespace vanguard8::debugger

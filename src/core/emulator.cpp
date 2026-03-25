#include "core/emulator.hpp"

#include <sstream>

namespace vanguard8::core {

auto Emulator::build_summary() const -> std::string {
    std::ostringstream summary;
    summary << "Vanguard 8 Emulator " << "0.1.0" << '\n';
    summary << "Build: scaffold milestone 0" << '\n';
    summary << "Core status: no-op shell";
    return summary.str();
}

}  // namespace vanguard8::core

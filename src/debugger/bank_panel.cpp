#include "debugger/bank_panel.hpp"

#include "core/emulator.hpp"

namespace vanguard8::debugger {

auto BankPanel::snapshot(const core::Emulator& emulator) const
    -> std::vector<core::cpu::BankSwitchEvent> {
    return emulator.cpu().bank_switch_log();
}

}  // namespace vanguard8::debugger

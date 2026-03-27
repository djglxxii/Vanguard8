#pragma once

#include "core/cpu/z180_adapter.hpp"

#include <vector>

namespace vanguard8::core {
class Emulator;
}

namespace vanguard8::debugger {

class BankPanel {
  public:
    [[nodiscard]] auto snapshot(const core::Emulator& emulator) const
        -> std::vector<core::cpu::BankSwitchEvent>;
};

}  // namespace vanguard8::debugger

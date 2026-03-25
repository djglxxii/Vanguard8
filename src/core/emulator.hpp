#pragma once

#include <string>

namespace vanguard8::core {

class Emulator {
  public:
    Emulator() = default;

    [[nodiscard]] auto build_summary() const -> std::string;
};

}  // namespace vanguard8::core

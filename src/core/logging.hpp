#pragma once

#include <string_view>

namespace vanguard8::core {

enum class LogLevel {
    trace,
    info,
    warning,
    error,
};

void log(LogLevel level, std::string_view message);

}  // namespace vanguard8::core

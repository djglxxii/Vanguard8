#include "core/logging.hpp"

#include <iostream>

#if defined(VANGUARD8_HAS_SPDLOG)
#include <spdlog/spdlog.h>
#endif

namespace vanguard8::core {

namespace {

[[nodiscard]] constexpr auto to_string(const LogLevel level) -> const char* {
    switch (level) {
    case LogLevel::trace:
        return "trace";
    case LogLevel::info:
        return "info";
    case LogLevel::warning:
        return "warning";
    case LogLevel::error:
        return "error";
    }

    return "unknown";
}

}  // namespace

void log(const LogLevel level, const std::string_view message) {
#if defined(VANGUARD8_HAS_SPDLOG)
    switch (level) {
    case LogLevel::trace:
        spdlog::trace(message);
        break;
    case LogLevel::info:
        spdlog::info(message);
        break;
    case LogLevel::warning:
        spdlog::warn(message);
        break;
    case LogLevel::error:
        spdlog::error(message);
        break;
    }
#else
    std::clog << "[vanguard8][" << to_string(level) << "] " << message << '\n';
#endif
}

}  // namespace vanguard8::core

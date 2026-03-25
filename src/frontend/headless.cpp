#include "frontend/headless.hpp"

#include "core/config.hpp"
#include "core/emulator.hpp"
#include "core/logging.hpp"

#include <iostream>

namespace vanguard8::frontend {

auto run_headless_app(int argc, char** argv) -> int {
    (void)argc;
    (void)argv;

    const auto config = core::load_or_create_config(core::default_config_path());
    (void)config;

    const core::Emulator emulator;
    core::log(core::LogLevel::info, "Launching headless scaffold.");
    std::cout << emulator.build_summary() << '\n';
    std::cout << "Headless status: no-op shell" << '\n';
    return 0;
}

}  // namespace vanguard8::frontend

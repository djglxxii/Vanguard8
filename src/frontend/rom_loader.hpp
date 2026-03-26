#pragma once

#include "core/config.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace vanguard8::frontend {

struct LoadedRom {
    std::filesystem::path path;
    std::vector<std::uint8_t> bytes;
};

[[nodiscard]] auto load_rom_file(const std::filesystem::path& path) -> LoadedRom;
void record_recent_rom(core::AppConfig& config, const std::filesystem::path& path);

}  // namespace vanguard8::frontend

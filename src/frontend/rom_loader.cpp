#include "frontend/rom_loader.hpp"

#include "core/memory/cartridge.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace vanguard8::frontend {

auto load_rom_file(const std::filesystem::path& path) -> LoadedRom {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Unable to open ROM file.");
    }

    std::vector<std::uint8_t> bytes(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>()
    );

    if (bytes.empty()) {
        throw std::invalid_argument("ROM image is empty.");
    }

    (void)core::memory::CartridgeSlot(bytes);
    return LoadedRom{.path = path, .bytes = std::move(bytes)};
}

void record_recent_rom(core::AppConfig& config, const std::filesystem::path& path) {
    const auto normalized = path.lexically_normal().string();
    config.recent_roms.erase(
        std::remove(config.recent_roms.begin(), config.recent_roms.end(), normalized),
        config.recent_roms.end()
    );
    config.recent_roms.insert(config.recent_roms.begin(), normalized);
    constexpr std::size_t recent_limit = 8;
    if (config.recent_roms.size() > recent_limit) {
        config.recent_roms.resize(recent_limit);
    }
}

}  // namespace vanguard8::frontend

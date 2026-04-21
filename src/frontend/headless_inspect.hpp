#pragma once

#include "core/emulator.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace vanguard8::frontend {

struct HeadlessMemoryRange {
    std::uint32_t address = 0;
    std::size_t length = 1;
};

struct HeadlessRunUntilPcSummary {
    bool requested = false;
    std::uint16_t target_pc = 0;
    std::uint64_t max_frames = 0;
    bool hit = false;
    std::uint64_t frame = 0;
    std::uint64_t master_cycle = 0;
};

struct HeadlessVramDumpSummary {
    char vdp = 'a';
    std::filesystem::path path;
    std::string sha256;
    std::size_t bytes = 0;
};

struct HeadlessInspectionConfig {
    std::uint64_t frame = 0;
    bool dump_cpu = false;
    bool dump_vdp_regs = false;
    std::vector<HeadlessMemoryRange> physical_peeks;
    std::vector<HeadlessMemoryRange> logical_peeks;
    std::vector<HeadlessVramDumpSummary> vram_dumps;
};

[[nodiscard]] auto format_headless_inspection_report(
    core::Emulator& emulator,
    const HeadlessInspectionConfig& config,
    const HeadlessRunUntilPcSummary& run_until_pc
) -> std::string;

[[nodiscard]] auto dump_headless_vram(
    const core::video::V9938& vdp,
    char vdp_label,
    const std::filesystem::path& path
) -> HeadlessVramDumpSummary;

}  // namespace vanguard8::frontend

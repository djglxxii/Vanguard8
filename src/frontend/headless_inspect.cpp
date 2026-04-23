#include "frontend/headless_inspect.hpp"

#include "core/hash.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace vanguard8::frontend {

namespace {

constexpr std::size_t bytes_per_dump_row = 16;

auto digest_hex(const core::Sha256Digest& digest) -> std::string {
    std::ostringstream stream;
    stream << std::hex << std::nouppercase << std::setfill('0');
    for (const auto byte : digest) {
        stream << std::setw(2) << static_cast<int>(byte);
    }
    return stream.str();
}

auto hex_value(const std::uint64_t value, const int width) -> std::string {
    std::ostringstream stream;
    stream << "0x" << std::hex << std::nouppercase << std::setfill('0') << std::setw(width)
           << value;
    return stream.str();
}

auto hex8(const std::uint8_t value) -> std::string { return hex_value(value, 2); }

auto hex16(const std::uint16_t value) -> std::string { return hex_value(value, 4); }

auto hex20(const std::uint32_t value) -> std::string { return hex_value(value, 5); }

auto hex9(const std::uint16_t value) -> std::string { return hex_value(value, 3); }

auto bool_text(const bool value) -> const char* { return value ? "true" : "false"; }

auto bank_label(const std::uint8_t bbr) -> std::string {
    if (bbr < 0x04U || bbr >= 0xF0U || (bbr % 4U) != 0U) {
        return "invalid";
    }
    return std::to_string(static_cast<int>(bbr / 4U) - 1);
}

auto ca0_end_exclusive(const std::uint8_t cbar) -> std::uint32_t {
    if (cbar == 0xFFU) {
        return 0x1'0000U;
    }
    return static_cast<std::uint32_t>((cbar >> 4U) << 12U);
}

auto ca1_start(const std::uint8_t cbar) -> std::uint32_t {
    if (cbar == 0xFFU) {
        return 0x1'0000U;
    }
    return static_cast<std::uint32_t>((cbar & 0x0FU) << 12U);
}

auto logical_region_name(const std::uint16_t logical, const std::uint8_t cbar) -> const char* {
    if (cbar == 0xFFU) {
        return "flat";
    }
    if (logical < ca0_end_exclusive(cbar)) {
        return "ca0";
    }
    if (logical >= ca1_start(cbar)) {
        return "ca1";
    }
    return "bank";
}

auto logical_region_end_exclusive(const std::uint16_t logical, const std::uint8_t cbar)
    -> std::uint32_t {
    if (cbar == 0xFFU) {
        return 0x1'0000U;
    }
    if (logical < ca0_end_exclusive(cbar)) {
        return ca0_end_exclusive(cbar);
    }
    if (logical >= ca1_start(cbar)) {
        return 0x1'0000U;
    }
    return ca1_start(cbar);
}

void append_byte_row(
    std::ostream& stream,
    const std::uint32_t display_address,
    const int display_width,
    const std::vector<std::uint8_t>& bytes,
    const std::size_t offset
) {
    stream << "  "
           << hex_value(display_address + static_cast<std::uint32_t>(offset), display_width) << ":";
    const auto count = std::min(bytes_per_dump_row, bytes.size() - offset);
    for (std::size_t index = 0; index < count; ++index) {
        stream << ' ' << std::hex << std::nouppercase << std::setfill('0') << std::setw(2)
               << static_cast<int>(bytes[offset + index]) << std::dec;
    }
    stream << '\n';
}

auto read_physical_bytes(
    core::Emulator& emulator,
    const std::uint32_t address,
    const std::size_t length
) -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(length);
    for (std::size_t offset = 0; offset < length; ++offset) {
        bytes.push_back(emulator.mutable_bus().read_memory(address + static_cast<std::uint32_t>(offset)));
    }
    return bytes;
}

void append_physical_peek(
    std::ostream& stream,
    core::Emulator& emulator,
    const HeadlessMemoryRange& range
) {
    const auto bytes = read_physical_bytes(emulator, range.address, range.length);
    stream << "physical " << hex20(range.address) << " length " << range.length << '\n';
    for (std::size_t offset = 0; offset < bytes.size(); offset += bytes_per_dump_row) {
        append_byte_row(stream, range.address, 5, bytes, offset);
    }
}

void append_logical_peek(
    std::ostream& stream,
    core::Emulator& emulator,
    const HeadlessMemoryRange& range
) {
    const auto cbar = emulator.cpu().state_snapshot().registers.cbar;
    std::uint32_t consumed = 0;
    while (consumed < range.length) {
        const auto logical = static_cast<std::uint16_t>((range.address + consumed) & 0xFFFFU);
        const auto region_end = logical_region_end_exclusive(logical, cbar);
        const auto until_boundary = region_end - logical;
        const auto chunk_length =
            std::min<std::uint32_t>(static_cast<std::uint32_t>(range.length - consumed), until_boundary);
        const auto physical = emulator.cpu().translate_logical_address(logical);
        const auto bytes = read_physical_bytes(emulator, physical, chunk_length);
        stream << "logical " << hex16(logical) << " physical " << hex20(physical) << " region "
               << logical_region_name(logical, cbar) << " length " << chunk_length << '\n';
        for (std::size_t offset = 0; offset < bytes.size(); offset += bytes_per_dump_row) {
            append_byte_row(stream, logical, 4, bytes, offset);
        }
        consumed += chunk_length;
    }
}

void append_cpu(std::ostream& stream, const core::Emulator& emulator) {
    const auto state = emulator.cpu().state_snapshot();
    const auto& r = state.registers;
    const auto flags = static_cast<std::uint8_t>(r.af & 0x00FFU);

    stream << "[cpu]\n";
    stream << "pc: " << hex16(r.pc) << '\n';
    stream << "a: " << hex8(static_cast<std::uint8_t>(r.af >> 8U)) << '\n';
    stream << "af: " << hex16(r.af) << '\n';
    stream << "bc: " << hex16(r.bc) << '\n';
    stream << "de: " << hex16(r.de) << '\n';
    stream << "hl: " << hex16(r.hl) << '\n';
    stream << "af_alt: " << hex16(r.af_alt) << '\n';
    stream << "bc_alt: " << hex16(r.bc_alt) << '\n';
    stream << "de_alt: " << hex16(r.de_alt) << '\n';
    stream << "hl_alt: " << hex16(r.hl_alt) << '\n';
    stream << "ix: " << hex16(r.ix) << '\n';
    stream << "iy: " << hex16(r.iy) << '\n';
    stream << "sp: " << hex16(r.sp) << '\n';
    stream << "i: " << hex8(r.i) << '\n';
    stream << "r: " << hex8(r.r) << '\n';
    stream << "iff1: " << bool_text(r.iff1) << '\n';
    stream << "iff2: " << bool_text(r.iff2) << '\n';
    stream << "halted: " << bool_text(r.halted) << '\n';
    stream << "flags: s=" << ((flags & 0x80U) != 0U) << " z=" << ((flags & 0x40U) != 0U)
           << " h=" << ((flags & 0x10U) != 0U) << " pv=" << ((flags & 0x04U) != 0U)
           << " n=" << ((flags & 0x02U) != 0U) << " c=" << ((flags & 0x01U) != 0U) << '\n';
    stream << "cbar: " << hex8(r.cbar) << '\n';
    stream << "cbr: " << hex8(r.cbr) << '\n';
    stream << "bbr: " << hex8(r.bbr) << '\n';
    stream << "active_bank: " << bank_label(r.bbr) << '\n';
    if (r.cbar == 0xFFU) {
        stream << "mmu.ca0: flat " << hex16(0x0000) << "-" << hex16(0xFFFF) << " -> "
               << hex20(0x00000) << '\n';
    } else {
        const auto ca0_end = ca0_end_exclusive(r.cbar);
        const auto ca1 = ca1_start(r.cbar);
        stream << "mmu.ca0: " << hex16(0x0000) << "-" << hex16(static_cast<std::uint16_t>(ca0_end - 1U))
               << " -> " << hex20(0x00000) << '\n';
        stream << "mmu.bank: " << hex16(static_cast<std::uint16_t>(ca0_end)) << "-"
               << hex16(static_cast<std::uint16_t>(ca1 - 1U)) << " -> " << hex20(r.bbr << 12U)
               << '\n';
        stream << "mmu.ca1: " << hex16(static_cast<std::uint16_t>(ca1)) << "-" << hex16(0xFFFF)
               << " -> " << hex20(r.cbr << 12U) << '\n';
    }
    stream << '\n';
}

auto palette_raw_9bit(const std::array<std::uint8_t, 2>& raw) -> std::uint16_t {
    const auto red = static_cast<std::uint16_t>((raw[0] >> 4U) & 0x07U);
    const auto green = static_cast<std::uint16_t>(raw[0] & 0x07U);
    const auto blue = static_cast<std::uint16_t>(raw[1] & 0x07U);
    return static_cast<std::uint16_t>((red << 6U) | (green << 3U) | blue);
}

void append_vdp(std::ostream& stream, const char label, const core::video::V9938& vdp) {
    const auto snapshot = vdp.state_snapshot();
    stream << "[vdp-" << static_cast<char>(std::tolower(static_cast<unsigned char>(label))) << "]\n";
    stream << "current_visible_width: " << vdp.current_visible_width() << '\n';
    stream << "registers:";
    for (std::uint8_t index = 0; index <= 46; ++index) {
        if ((index % 8U) == 0U) {
            stream << "\n ";
        }
        stream << " r" << std::dec << std::setfill('0') << std::setw(2) << static_cast<int>(index)
               << "=" << hex8(vdp.register_value(index));
    }
    stream << "\nstatus:";
    for (std::uint8_t index = 0; index < 10; ++index) {
        if ((index % 8U) == 0U) {
            stream << "\n ";
        }
        stream << " s" << std::dec << std::setfill('0') << std::setw(2) << static_cast<int>(index)
               << "=" << hex8(vdp.status_value(index));
    }
    stream << "\npalette:\n";
    for (std::uint8_t index = 0; index < 16; ++index) {
        const auto raw = vdp.palette_entry_raw(index);
        const auto rgb = vdp.palette_entry_rgb(index);
        stream << " p" << std::dec << std::setfill('0') << std::setw(2) << static_cast<int>(index)
               << ": raw_bytes=" << hex8(raw[0]) << "," << hex8(raw[1])
               << " raw_9bit=" << hex9(palette_raw_9bit(raw)) << " rgb=" << hex8(rgb[0]) << ","
               << hex8(rgb[1]) << "," << hex8(rgb[2]) << '\n';
    }
    stream << "command.type: " << hex8(snapshot.command.type) << '\n';
    stream << "command.active: " << bool_text(snapshot.command.active) << '\n';
    stream << "command.transfer_ready: " << bool_text(snapshot.command.transfer_ready) << '\n';
    stream << "command.cycles_remaining: " << snapshot.command.cycles_remaining << '\n';
    stream << "command.stream_x: " << snapshot.command.stream_x << '\n';
    stream << "command.stream_y: " << snapshot.command.stream_y << '\n';
    stream << "command.remaining_x: " << snapshot.command.remaining_x << '\n';
    stream << "command.remaining_y: " << snapshot.command.remaining_y << "\n\n";
}

}  // namespace

auto dump_headless_vram(
    const core::video::V9938& vdp,
    const char vdp_label,
    const std::filesystem::path& path
) -> HeadlessVramDumpSummary {
    const auto& vram = vdp.vram();
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("Unable to open VRAM dump path.");
    }
    output.write(reinterpret_cast<const char*>(vram.data()), static_cast<std::streamsize>(vram.size()));
    if (!output) {
        throw std::runtime_error("Unable to write VRAM dump.");
    }

    const std::vector<std::uint8_t> bytes(vram.begin(), vram.end());
    return HeadlessVramDumpSummary{
        .vdp = static_cast<char>(std::tolower(static_cast<unsigned char>(vdp_label))),
        .path = path.filename(),
        .sha256 = digest_hex(core::sha256(bytes)),
        .bytes = bytes.size(),
    };
}

auto format_headless_inspection_report(
    core::Emulator& emulator,
    const HeadlessInspectionConfig& config,
    const HeadlessRunUntilPcSummary& run_until_pc
) -> std::string {
    std::ostringstream stream;
    stream << "vanguard8 headless inspection v1\n";
    stream << "frame: " << config.frame << '\n';
    stream << "completed_frames: " << emulator.completed_frames() << '\n';
    stream << "master_cycle: " << emulator.master_cycle() << '\n';
    stream << "cpu_tstates: " << emulator.cpu_tstates() << '\n';
    stream << "event_log_digest: " << emulator.event_log_digest() << '\n';
    stream << '\n';

    stream << "[run-until-pc]\n";
    stream << "requested: " << bool_text(run_until_pc.requested) << '\n';
    if (run_until_pc.requested) {
        stream << "target_pc: " << hex16(run_until_pc.target_pc) << '\n';
        stream << "max_frames: " << run_until_pc.max_frames << '\n';
        stream << "result: " << (run_until_pc.hit ? "hit" : "not-hit") << '\n';
        stream << "frame: " << run_until_pc.frame << '\n';
        stream << "master_cycle: " << run_until_pc.master_cycle << '\n';
    }
    stream << '\n';

    if (config.dump_cpu) {
        append_cpu(stream, emulator);
    }

    if (config.dump_vdp_regs) {
        append_vdp(stream, 'a', emulator.vdp_a());
        append_vdp(stream, 'b', emulator.vdp_b());
    }

    if (!config.physical_peeks.empty()) {
        stream << "[peek-mem]\n";
        for (const auto& range : config.physical_peeks) {
            append_physical_peek(stream, emulator, range);
        }
        stream << '\n';
    }

    if (!config.logical_peeks.empty()) {
        stream << "[peek-logical]\n";
        for (const auto& range : config.logical_peeks) {
            append_logical_peek(stream, emulator, range);
        }
        stream << '\n';
    }

    if (!config.vram_dumps.empty()) {
        stream << "[vram-dumps]\n";
        for (const auto& dump : config.vram_dumps) {
            stream << "vdp-" << dump.vdp << ": path=" << dump.path.string() << " bytes=" << dump.bytes
                   << " sha256=" << dump.sha256 << '\n';
        }
        stream << '\n';
    }

    return stream.str();
}

}  // namespace vanguard8::frontend

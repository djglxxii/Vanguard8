#include "core/emulator.hpp"

#include "core/memory/cartridge.hpp"

#include <algorithm>
#include <array>
#include <sstream>
#include <stdexcept>

namespace vanguard8::core {

namespace {

auto make_idle_rom() -> std::vector<std::uint8_t> {
    return std::vector<std::uint8_t>(memory::CartridgeSlot::fixed_region_size, 0x00);
}

}  // namespace

Emulator::Emulator() : bus_(memory::CartridgeSlot(make_idle_rom())), cpu_(bus_) { reset(); }

auto Emulator::build_summary() const -> std::string {
    std::ostringstream summary;
    summary << "Vanguard 8 Emulator " << "0.1.0" << '\n';
    summary << "Build: milestone 3" << '\n';
    summary << "Core status: event-scheduled headless runtime";
    return summary.str();
}

void Emulator::reset() {
    bus_ = Bus(memory::CartridgeSlot(make_idle_rom()));
    cpu_.reset();
    scheduler_.reset();
    master_cycle_ = 0;
    cpu_tstates_ = 0;
    cpu_master_remainder_ = 0;
    completed_frames_ = 0;
    frame_start_cycle_ = 0;
    next_vclk_tick_ = 0;
    current_scanline_ = 0;
    event_log_.clear();
    populate_scheduler_for_frame();
}

void Emulator::pause() { paused_ = true; }

void Emulator::resume() { paused_ = false; }

auto Emulator::paused() const -> bool { return paused_; }

void Emulator::set_vclk_rate(const VclkRate rate) {
    vclk_rate_ = rate;
    next_vclk_tick_ = 0;
    scheduler_.reset();
    populate_scheduler_for_frame();
}

auto Emulator::vclk_rate() const -> VclkRate { return vclk_rate_; }

void Emulator::run_frames(const std::uint64_t frame_count) {
    if (paused_) {
        return;
    }

    for (std::uint64_t index = 0; index < frame_count; ++index) {
        run_single_frame();
    }
}

void Emulator::step_frame() {
    const auto was_paused = paused_;
    paused_ = false;
    run_single_frame();
    paused_ = true;
    if (!was_paused) {
        paused_ = true;
    }
}

void Emulator::clear_event_log() { event_log_.clear(); }

auto Emulator::master_cycle() const -> std::uint64_t { return master_cycle_; }

auto Emulator::cpu_tstates() const -> std::uint64_t { return cpu_tstates_; }

auto Emulator::completed_frames() const -> std::uint64_t { return completed_frames_; }

auto Emulator::current_scanline() const -> int { return current_scanline_; }

auto Emulator::event_log() const -> const std::vector<EventLogEntry>& { return event_log_; }

auto Emulator::event_log_digest() const -> std::uint64_t {
    std::uint64_t digest = 1469598103934665603ULL;
    for (const auto& event : event_log_) {
        const std::array<std::uint64_t, 5> parts = {
            static_cast<std::uint64_t>(event.type),
            event.master_cycle,
            event.cpu_tstates,
            event.frame_index,
            static_cast<std::uint64_t>(event.scanline + 1),
        };
        for (const auto part : parts) {
            digest ^= part;
            digest *= 1099511628211ULL;
        }
    }
    return digest;
}

auto Emulator::scheduler_size() const -> std::size_t { return scheduler_.size(); }

auto Emulator::event_type_name(const EventType type) -> const char* {
    switch (type) {
    case EventType::hblank:
        return "hblank";
    case EventType::vblank:
        return "vblank";
    case EventType::vblank_end:
        return "vblank_end";
    case EventType::vclk:
        return "vclk";
    }
    return "unknown";
}

auto Emulator::vclk_rate_name(const VclkRate rate) -> const char* {
    switch (rate) {
    case VclkRate::stopped:
        return "off";
    case VclkRate::hz_4000:
        return "4000";
    case VclkRate::hz_6000:
        return "6000";
    case VclkRate::hz_8000:
        return "8000";
    }
    return "off";
}

auto Emulator::parse_vclk_rate(const std::string& value) -> VclkRate {
    if (value == "off" || value == "stop" || value == "stopped") {
        return VclkRate::stopped;
    }
    if (value == "4000" || value == "4k") {
        return VclkRate::hz_4000;
    }
    if (value == "6000" || value == "6k") {
        return VclkRate::hz_6000;
    }
    if (value == "8000" || value == "8k") {
        return VclkRate::hz_8000;
    }
    throw std::invalid_argument("Unsupported VCLK rate string.");
}

void Emulator::populate_scheduler_for_frame() {
    scheduler_.reset();

    for (std::uint32_t scanline = 0; scanline < timing::active_lines; ++scanline) {
        scheduler_.schedule(
            EventType::hblank,
            frame_start_cycle_ + static_cast<std::uint64_t>(scanline) * timing::master_per_line +
                timing::master_hblank_start,
            static_cast<int>(scanline)
        );
    }

    scheduler_.schedule(
        EventType::vblank,
        frame_start_cycle_ + static_cast<std::uint64_t>(timing::active_lines) * timing::master_per_line,
        static_cast<int>(timing::active_lines)
    );
    scheduler_.schedule(
        EventType::vblank_end,
        frame_start_cycle_ + timing::master_per_frame,
        static_cast<int>(timing::total_lines)
    );

    if (vclk_rate_ == VclkRate::stopped) {
        return;
    }

    const auto frame_end = frame_start_cycle_ + timing::master_per_frame;
    while (vclk_due_cycle(next_vclk_tick_ + 1U) <= frame_end) {
        ++next_vclk_tick_;
        scheduler_.schedule(EventType::vclk, vclk_due_cycle(next_vclk_tick_));
    }
}

void Emulator::run_single_frame() {
    if (scheduler_.empty()) {
        populate_scheduler_for_frame();
    }

    while (const auto next = scheduler_.pop()) {
        run_cpu_until(next->master_cycle_due);
        fire_event(*next);
        if (next->type == EventType::vblank_end) {
            ++completed_frames_;
            frame_start_cycle_ = next->master_cycle_due;
            current_scanline_ = 0;
            populate_scheduler_for_frame();
            break;
        }
    }
}

void Emulator::run_cpu_until(const std::uint64_t target_master_cycle) {
    if (target_master_cycle < master_cycle_) {
        throw std::logic_error("Target master cycle moved backwards.");
    }

    const auto delta = target_master_cycle - master_cycle_;
    cpu_master_remainder_ += delta;
    cpu_tstates_ += cpu_master_remainder_ / timing::cpu_divider;
    cpu_master_remainder_ %= timing::cpu_divider;
    master_cycle_ = target_master_cycle;
}

void Emulator::fire_event(const Event& event) {
    switch (event.type) {
    case EventType::hblank:
        current_scanline_ = event.scanline;
        break;
    case EventType::vblank:
        current_scanline_ = event.scanline;
        break;
    case EventType::vblank_end:
        current_scanline_ = 0;
        break;
    case EventType::vclk:
        break;
    }

    event_log_.push_back(EventLogEntry{
        .type = event.type,
        .master_cycle = event.master_cycle_due,
        .cpu_tstates = cpu_tstates_,
        .frame_index = completed_frames_,
        .scanline = event.scanline,
    });
}

auto Emulator::vclk_rate_hz() const -> std::uint32_t {
    switch (vclk_rate_) {
    case VclkRate::stopped:
        return 0;
    case VclkRate::hz_4000:
        return 4'000;
    case VclkRate::hz_6000:
        return 6'000;
    case VclkRate::hz_8000:
        return 8'000;
    }
    return 0;
}

auto Emulator::vclk_due_cycle(const std::uint64_t tick_index) const -> std::uint64_t {
    const auto rate = vclk_rate_hz();
    if (rate == 0) {
        return timing::master_per_frame + frame_start_cycle_;
    }

    return ((tick_index * timing::master_hz) + (rate / 2U)) / rate;
}

}  // namespace vanguard8::core

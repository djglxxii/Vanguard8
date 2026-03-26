#pragma once

#include "core/bus.hpp"
#include "core/cpu/z180_adapter.hpp"
#include "core/memory/cartridge.hpp"
#include "core/scheduler.hpp"
#include "core/video/v9938.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace vanguard8::core {

enum class VclkRate {
    stopped,
    hz_4000,
    hz_6000,
    hz_8000,
};

struct EventLogEntry {
    EventType type = EventType::hblank;
    std::uint64_t master_cycle = 0;
    std::uint64_t cpu_tstates = 0;
    std::uint64_t frame_index = 0;
    int scanline = -1;
};

class Emulator {
  public:
    Emulator();

    [[nodiscard]] auto build_summary() const -> std::string;

    void reset();
    void load_rom_image(const std::vector<std::uint8_t>& rom_image);
    void pause();
    void resume();
    [[nodiscard]] auto paused() const -> bool;

    void set_vclk_rate(VclkRate rate);
    [[nodiscard]] auto vclk_rate() const -> VclkRate;

    void run_frames(std::uint64_t frame_count);
    void step_frame();
    void clear_event_log();

    [[nodiscard]] auto master_cycle() const -> std::uint64_t;
    [[nodiscard]] auto cpu_tstates() const -> std::uint64_t;
    [[nodiscard]] auto completed_frames() const -> std::uint64_t;
    [[nodiscard]] auto current_scanline() const -> int;
    [[nodiscard]] auto event_log() const -> const std::vector<EventLogEntry>&;
    [[nodiscard]] auto event_log_digest() const -> std::uint64_t;
    [[nodiscard]] auto bus() const -> const Bus&;
    [[nodiscard]] auto mutable_bus() -> Bus&;
    [[nodiscard]] auto vdp_a() const -> const video::V9938&;
    [[nodiscard]] auto vdp_b() const -> const video::V9938&;
    [[nodiscard]] auto loaded_rom_size() const -> std::size_t;

    [[nodiscard]] auto scheduler_size() const -> std::size_t;

    [[nodiscard]] static auto event_type_name(EventType type) -> const char*;
    [[nodiscard]] static auto vclk_rate_name(VclkRate rate) -> const char*;
    [[nodiscard]] static auto parse_vclk_rate(const std::string& value) -> VclkRate;

  private:
    Bus bus_;
    cpu::Z180Adapter cpu_;
    Scheduler scheduler_;
    std::uint64_t master_cycle_ = 0;
    std::uint64_t cpu_tstates_ = 0;
    std::uint64_t cpu_master_remainder_ = 0;
    std::uint64_t completed_frames_ = 0;
    std::uint64_t frame_start_cycle_ = 0;
    std::uint64_t next_vclk_tick_ = 0;
    int current_scanline_ = 0;
    bool paused_ = false;
    VclkRate vclk_rate_ = VclkRate::stopped;
    std::vector<EventLogEntry> event_log_;
    std::size_t loaded_rom_size_ = memory::CartridgeSlot::fixed_region_size;

    void populate_scheduler_for_frame();
    void run_single_frame();
    void run_cpu_until(std::uint64_t target_master_cycle);
    void fire_event(const Event& event);
    [[nodiscard]] auto vclk_rate_hz() const -> std::uint32_t;
    [[nodiscard]] auto vclk_due_cycle(std::uint64_t tick_index) const -> std::uint64_t;
};

}  // namespace vanguard8::core

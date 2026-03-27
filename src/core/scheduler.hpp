#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace vanguard8::core {

namespace timing {

constexpr std::uint64_t master_hz = 14'318'180;
constexpr std::uint32_t cpu_divider = 2;
constexpr std::uint32_t active_lines = 212;
constexpr std::uint32_t total_lines = 262;
constexpr std::uint32_t master_per_line = 910;
constexpr std::uint32_t master_hblank_start = 512;
constexpr std::uint64_t master_per_frame =
    static_cast<std::uint64_t>(total_lines) * master_per_line;

}  // namespace timing

enum class EventType {
    hblank,
    vblank,
    vblank_end,
    vclk,
};

struct Event {
    std::uint64_t master_cycle_due = 0;
    EventType type = EventType::hblank;
    int scanline = -1;
    std::uint64_t sequence = 0;
};

struct SchedulerState {
    std::vector<Event> events;
    std::uint64_t next_sequence = 0;
};

class Scheduler {
  public:
    void reset();
    void schedule(EventType type, std::uint64_t master_cycle_due, int scanline = -1);
    [[nodiscard]] auto peek() const -> std::optional<Event>;
    [[nodiscard]] auto pop() -> std::optional<Event>;
    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> std::size_t;
    [[nodiscard]] auto state_snapshot() const -> SchedulerState;
    void load_state(const SchedulerState& state);

  private:
    std::vector<Event> events_;
    std::uint64_t next_sequence_ = 0;
};

}  // namespace vanguard8::core

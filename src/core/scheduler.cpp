#include "core/scheduler.hpp"

#include <algorithm>

namespace vanguard8::core {

void Scheduler::reset() {
    events_.clear();
    next_sequence_ = 0;
}

void Scheduler::schedule(const EventType type, const std::uint64_t master_cycle_due, const int scanline) {
    events_.push_back(Event{
        .master_cycle_due = master_cycle_due,
        .type = type,
        .scanline = scanline,
        .sequence = next_sequence_++,
    });

    std::stable_sort(events_.begin(), events_.end(), [](const Event& lhs, const Event& rhs) {
        if (lhs.master_cycle_due != rhs.master_cycle_due) {
            return lhs.master_cycle_due < rhs.master_cycle_due;
        }
        return lhs.sequence < rhs.sequence;
    });
}

auto Scheduler::peek() const -> std::optional<Event> {
    if (events_.empty()) {
        return std::nullopt;
    }
    return events_.front();
}

auto Scheduler::pop() -> std::optional<Event> {
    if (events_.empty()) {
        return std::nullopt;
    }

    const auto next = events_.front();
    events_.erase(events_.begin());
    return next;
}

auto Scheduler::empty() const -> bool { return events_.empty(); }

auto Scheduler::size() const -> std::size_t { return events_.size(); }

auto Scheduler::state_snapshot() const -> SchedulerState {
    return SchedulerState{
        .events = events_,
        .next_sequence = next_sequence_,
    };
}

void Scheduler::load_state(const SchedulerState& state) {
    events_ = state.events;
    next_sequence_ = state.next_sequence;
}

}  // namespace vanguard8::core

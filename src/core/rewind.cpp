#include "core/rewind.hpp"

#include "core/save_state.hpp"

#include <algorithm>
#include <cstddef>

namespace vanguard8::core {

RewindBuffer::RewindBuffer(const std::size_t capacity) : capacity_(capacity) {}

void RewindBuffer::clear() {
    snapshots_.clear();
    cursor_live_ = true;
    cursor_index_ = 0;
}

void RewindBuffer::set_capacity(const std::size_t capacity) {
    capacity_ = capacity;
    trim_to_capacity();
}

void RewindBuffer::capture(const Emulator& emulator) {
    if (!cursor_live_ && !snapshots_.empty()) {
        snapshots_.erase(snapshots_.begin() + static_cast<std::ptrdiff_t>(cursor_index_ + 1U), snapshots_.end());
        cursor_live_ = true;
        cursor_index_ = snapshots_.empty() ? 0 : (snapshots_.size() - 1U);
    }

    const auto frame = emulator.completed_frames();
    const auto bytes = SaveState::serialize(emulator);
    if (!snapshots_.empty() && snapshots_.back().frame == frame) {
        snapshots_.back().bytes = bytes;
        cursor_index_ = snapshots_.size() - 1U;
        return;
    }

    snapshots_.push_back(Snapshot{
        .frame = frame,
        .bytes = bytes,
    });
    trim_to_capacity();
    cursor_live_ = true;
    cursor_index_ = snapshots_.empty() ? 0 : (snapshots_.size() - 1U);
}

auto RewindBuffer::rewind_step(Emulator& emulator) -> bool {
    if (snapshots_.empty()) {
        return false;
    }

    if (cursor_live_) {
        cursor_index_ = snapshots_.size() > 1U ? snapshots_.size() - 2U : 0U;
        cursor_live_ = false;
    } else if (cursor_index_ > 0U) {
        --cursor_index_;
    }

    SaveState::load(emulator, snapshots_[cursor_index_].bytes);
    return true;
}

auto RewindBuffer::empty() const -> bool { return snapshots_.empty(); }

auto RewindBuffer::size() const -> std::size_t { return snapshots_.size(); }

auto RewindBuffer::current_frame() const -> std::uint64_t {
    if (snapshots_.empty()) {
        return 0;
    }
    const auto index = cursor_live_ ? snapshots_.size() - 1U : cursor_index_;
    return snapshots_[index].frame;
}

void RewindBuffer::trim_to_capacity() {
    if (capacity_ == 0U) {
        clear();
        return;
    }

    if (snapshots_.size() <= capacity_) {
        return;
    }

    const auto overflow = snapshots_.size() - capacity_;
    snapshots_.erase(snapshots_.begin(), snapshots_.begin() + static_cast<std::ptrdiff_t>(overflow));
    if (!cursor_live_) {
        cursor_index_ = cursor_index_ > overflow ? cursor_index_ - overflow : 0U;
    }
}

}  // namespace vanguard8::core

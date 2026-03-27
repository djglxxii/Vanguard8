#pragma once

#include "core/emulator.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace vanguard8::core {

class RewindBuffer {
  public:
    explicit RewindBuffer(std::size_t capacity = 900);

    void clear();
    void set_capacity(std::size_t capacity);
    void capture(const Emulator& emulator);
    [[nodiscard]] auto rewind_step(Emulator& emulator) -> bool;
    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> std::size_t;
    [[nodiscard]] auto current_frame() const -> std::uint64_t;

  private:
    struct Snapshot {
        std::uint64_t frame = 0;
        std::vector<std::uint8_t> bytes;
    };

    std::size_t capacity_ = 0;
    std::vector<Snapshot> snapshots_{};
    bool cursor_live_ = true;
    std::size_t cursor_index_ = 0;

    void trim_to_capacity();
};

}  // namespace vanguard8::core

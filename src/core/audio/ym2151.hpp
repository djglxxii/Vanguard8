#pragma once

#include "core/audio/sample.hpp"

#include <cstdint>

extern "C" {
#include "third_party/nuked-opm/opm.h"
}

namespace vanguard8::core::audio {

class Ym2151 {
  public:
    static constexpr std::uint32_t master_cycles_per_ym_clock = 4;
    static constexpr std::uint32_t ym_clocks_per_sample = 64;

    Ym2151() { reset(); }

    void reset();
    void write_address(std::uint8_t address);
    void write_data(std::uint8_t data);
    [[nodiscard]] auto read_status() const -> std::uint8_t;
    void advance_master_cycle();
    [[nodiscard]] auto irq_pending() const -> bool;
    [[nodiscard]] auto current_output() const -> StereoSample;
    [[nodiscard]] auto latched_address() const -> std::uint8_t;

  private:
    opm_t chip_{};
    StereoSample current_output_{};
    std::uint32_t master_cycle_divider_ = 0;
    std::uint32_t sample_clock_divider_ = 0;
    std::uint8_t latched_address_ = 0;
};

}  // namespace vanguard8::core::audio

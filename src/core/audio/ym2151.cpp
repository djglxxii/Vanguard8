#include "core/audio/ym2151.hpp"

#include <algorithm>

namespace vanguard8::core::audio {

namespace {

[[nodiscard]] auto clamp_unit(const float value) -> float {
    return std::clamp(value, -1.0F, 1.0F);
}

}  // namespace

void Ym2151::reset() {
    chip_ = opm_t{};
    OPM_Reset(&chip_, opm_flags_none);
    current_output_ = {};
    master_cycle_divider_ = 0;
    sample_clock_divider_ = 0;
    latched_address_ = 0;
}

void Ym2151::write_address(const std::uint8_t address) {
    latched_address_ = address;
    OPM_Write(&chip_, 0, address);
}

void Ym2151::write_data(const std::uint8_t data) { OPM_Write(&chip_, 1, data); }

auto Ym2151::read_status() const -> std::uint8_t { return OPM_Read(const_cast<opm_t*>(&chip_), 1); }

void Ym2151::advance_master_cycle() {
    ++master_cycle_divider_;
    if (master_cycle_divider_ < master_cycles_per_ym_clock) {
        return;
    }

    master_cycle_divider_ = 0;

    int32_t output[2] = {0, 0};
    std::uint8_t sh1 = 0;
    std::uint8_t sh2 = 0;
    std::uint8_t so = 0;
    OPM_Clock(&chip_, output, &sh1, &sh2, &so);

    ++sample_clock_divider_;
    if (sample_clock_divider_ < ym_clocks_per_sample) {
        return;
    }

    sample_clock_divider_ = 0;
    current_output_ = StereoSample{
        .left = clamp_unit(static_cast<float>(output[0]) / 32768.0F),
        .right = clamp_unit(static_cast<float>(output[1]) / 32768.0F),
    };
}

auto Ym2151::irq_pending() const -> bool { return OPM_ReadIRQ(const_cast<opm_t*>(&chip_)) != 0U; }

auto Ym2151::current_output() const -> StereoSample { return current_output_; }

auto Ym2151::latched_address() const -> std::uint8_t { return latched_address_; }

}  // namespace vanguard8::core::audio

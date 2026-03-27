#pragma once

#include <array>
#include <cstdint>

namespace vanguard8::third_party::msm5205 {

class Core {
  public:
    struct State {
        int signal = 0;
        int step = 0;
    };

    Core();

    void reset();
    void clock(std::uint8_t nibble);

    [[nodiscard]] auto sample() const -> std::int16_t;
    [[nodiscard]] auto state_snapshot() const -> State;
    void load_state(const State& state);

  private:
    std::array<int, 49 * 16> diff_lookup_{};
    int signal_ = 0;
    int step_ = 0;

    void compute_tables();
};

}  // namespace vanguard8::third_party::msm5205

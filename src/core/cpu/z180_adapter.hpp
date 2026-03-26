#pragma once

#include "core/bus.hpp"
#include "third_party/z180/z180_core.hpp"

#include <cstdint>
#include <optional>

namespace vanguard8::core::cpu {

enum class InterruptSource {
    int0,
    int1,
};

struct InterruptService {
    InterruptSource source;
    std::uint16_t handler_address = 0;
};

class Z180Adapter {
  public:
    explicit Z180Adapter(core::Bus& bus);

    void reset();

    [[nodiscard]] auto pc() const -> std::uint16_t;
    [[nodiscard]] auto accumulator() const -> std::uint8_t;
    [[nodiscard]] auto register_i() const -> std::uint8_t;
    [[nodiscard]] auto register_il() const -> std::uint8_t;
    [[nodiscard]] auto register_itc() const -> std::uint8_t;
    [[nodiscard]] auto cbar() const -> std::uint8_t;
    [[nodiscard]] auto cbr() const -> std::uint8_t;
    [[nodiscard]] auto bbr() const -> std::uint8_t;
    [[nodiscard]] auto interrupt_mode() const -> std::uint8_t;
    [[nodiscard]] auto halted() const -> bool;

    void set_register_i(std::uint8_t value);
    void set_interrupt_mode(std::uint8_t mode);
    void set_iff1(bool enabled);
    void set_iff2(bool enabled);

    [[nodiscard]] auto in0(std::uint8_t port) const -> std::uint8_t;
    void out0(std::uint8_t port, std::uint8_t value);

    [[nodiscard]] auto translate_logical_address(std::uint16_t logical_address) const
        -> std::uint32_t;
    [[nodiscard]] auto read_logical(std::uint16_t logical_address) -> std::uint8_t;
    void write_logical(std::uint16_t logical_address, std::uint8_t value);

    [[nodiscard]] auto service_pending_interrupt() -> std::optional<InterruptService>;
    void run_until_halt(std::size_t max_instructions);

  private:
    core::Bus& bus_;
    third_party::z180::Core core_;
};

}  // namespace vanguard8::core::cpu

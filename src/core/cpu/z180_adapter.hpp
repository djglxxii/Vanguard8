#pragma once

#include "core/bus.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

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
    std::uint16_t pc_ = 0x0000;
    std::uint16_t hl_ = 0x0000;
    std::uint8_t a_ = 0x00;
    std::uint8_t i_ = 0x00;
    std::uint8_t il_ = 0x00;
    std::uint8_t itc_ = 0x00;
    std::uint8_t cbar_ = 0xFF;
    std::uint8_t cbr_ = 0x00;
    std::uint8_t bbr_ = 0x00;
    std::uint8_t interrupt_mode_ = 0x00;
    bool iff1_ = false;
    bool iff2_ = false;
    bool halted_ = false;

    [[nodiscard]] auto fetch_byte() -> std::uint8_t;
    [[nodiscard]] auto fetch_word() -> std::uint16_t;
    void execute_one();
    void maybe_warn_illegal_bbr();
};

}  // namespace vanguard8::core::cpu

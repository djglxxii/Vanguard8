#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace vanguard8::third_party::z180 {

enum class InterruptSource {
    int0,
    int1,
};

struct InterruptService {
    InterruptSource source;
    std::uint16_t handler_address = 0;
};

struct Callbacks {
    std::function<std::uint8_t(std::uint32_t)> read_memory;
    std::function<void(std::uint32_t, std::uint8_t)> write_memory;
    std::function<std::uint8_t(std::uint16_t)> read_port;
    std::function<void(std::uint16_t, std::uint8_t)> write_port;
    std::function<void(std::string)> record_warning;
    std::function<void()> acknowledge_int1;
};

class Core {
  public:
    explicit Core(Callbacks callbacks);

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

    [[nodiscard]] auto service_pending_interrupt(bool int0_asserted, bool int1_asserted)
        -> std::optional<InterruptService>;
    void run_until_halt(std::size_t max_instructions);

  private:
    using Handler = void (Core::*)();

    struct Pair {
        union {
            std::uint16_t value = 0;
            struct {
                std::uint8_t lo;
                std::uint8_t hi;
            } bytes;
        };
    };

    Callbacks callbacks_;
    Pair af_{};
    Pair bc_{};
    Pair de_{};
    Pair hl_{};
    Pair sp_{};
    Pair pc_{};
    Pair af2_{};
    Pair bc2_{};
    Pair de2_{};
    Pair hl2_{};
    Pair ix_{};
    Pair iy_{};
    std::uint8_t r_ = 0x00;
    std::uint8_t r2_ = 0x00;
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
    std::vector<Handler> opcodes_;
    std::vector<Handler> ed_opcodes_;

    void initialize_tables();

    [[nodiscard]] auto fetch_byte() -> std::uint8_t;
    [[nodiscard]] auto fetch_word() -> std::uint16_t;
    void write_word(std::uint16_t address, std::uint16_t value);
    [[nodiscard]] auto read_word(std::uint16_t address) -> std::uint16_t;
    void push_word(std::uint16_t value);
    [[nodiscard]] auto pop_word() -> std::uint16_t;
    void execute_one();
    void maybe_warn_illegal_bbr();
    void acknowledge_reti();
    [[noreturn]] void unsupported_opcode(std::uint8_t opcode);
    [[noreturn]] void unsupported_ed_opcode(std::uint8_t opcode);
    void op_unimplemented();
    void op_nop();
    void op_ld_hl_nn();
    void op_ld_mem_nn_hl();
    void op_ld_sp_nn();
    void op_ld_mem_nn_a();
    void op_ld_a_mem_nn();
    void op_ld_a_n();
    void op_halt();
    void op_jp_nn();
    void op_ret();
    void op_call_nn();
    void op_di();
    void op_ei();
    void op_ed_prefix();
    void op_ed_out0_n_a();
    void op_ed_ld_i_a();
    void op_ed_reti();
};

}  // namespace vanguard8::third_party::z180

#include "third_party/z180/z180_core.hpp"

#include <iomanip>
#include <sstream>

namespace vanguard8::third_party::z180 {

namespace {

constexpr std::uint8_t flag_zero = 0x40;
constexpr std::uint8_t itc_ite1 = 0x02;

[[nodiscard]] auto hex_string(const std::uint32_t value, const int width) -> std::string {
    std::ostringstream stream;
    stream << "0x" << std::uppercase << std::hex << std::setw(width) << std::setfill('0') << value;
    return stream.str();
}

}  // namespace

Core::Core(Callbacks callbacks) : callbacks_(std::move(callbacks)) { initialize_tables(); reset(); }

void Core::reset() {
    af_ = {};
    bc_ = {};
    de_ = {};
    hl_ = {};
    sp_ = {};
    pc_ = {};
    af2_ = {};
    bc2_ = {};
    de2_ = {};
    hl2_ = {};
    ix_.value = 0xFFFF;
    iy_.value = 0xFFFF;
    r_ = 0x00;
    r2_ = 0x00;
    i_ = 0x00;
    il_ = 0x00;
    itc_ = 0x00;
    cbar_ = 0xFF;
    cbr_ = 0x00;
    bbr_ = 0x00;
    interrupt_mode_ = 0x00;
    iff1_ = false;
    iff2_ = false;
    halted_ = false;
    af_.bytes.lo = flag_zero;
}

auto Core::pc() const -> std::uint16_t { return pc_.value; }

auto Core::accumulator() const -> std::uint8_t { return af_.bytes.hi; }

auto Core::register_i() const -> std::uint8_t { return i_; }

auto Core::register_il() const -> std::uint8_t { return il_; }

auto Core::register_itc() const -> std::uint8_t { return itc_; }

auto Core::cbar() const -> std::uint8_t { return cbar_; }

auto Core::cbr() const -> std::uint8_t { return cbr_; }

auto Core::bbr() const -> std::uint8_t { return bbr_; }

auto Core::interrupt_mode() const -> std::uint8_t { return interrupt_mode_; }

auto Core::halted() const -> bool { return halted_; }

void Core::set_register_i(const std::uint8_t value) { i_ = value; }

void Core::set_interrupt_mode(const std::uint8_t mode) { interrupt_mode_ = static_cast<std::uint8_t>(mode & 0x03U); }

void Core::set_iff1(const bool enabled) { iff1_ = enabled; }

void Core::set_iff2(const bool enabled) { iff2_ = enabled; }

auto Core::in0(const std::uint8_t port) const -> std::uint8_t {
    switch (port) {
    case 0x33:
        return il_;
    case 0x34:
        return itc_;
    case 0x38:
        return cbr_;
    case 0x39:
        return bbr_;
    case 0x3A:
        return cbar_;
    default:
        return 0x00;
    }
}

void Core::out0(const std::uint8_t port, const std::uint8_t value) {
    switch (port) {
    case 0x33:
        il_ = value;
        break;
    case 0x34:
        itc_ = value;
        break;
    case 0x38:
        cbr_ = value;
        break;
    case 0x39:
        bbr_ = value;
        maybe_warn_illegal_bbr();
        break;
    case 0x3A:
        cbar_ = value;
        break;
    default:
        break;
    }
}

auto Core::translate_logical_address(const std::uint16_t logical_address) const -> std::uint32_t {
    // Adapted from MAME's z180_mmu() remap logic, with Vanguard 8's documented flat-reset
    // behavior preserved from the repo spec: CBAR=0xFF keeps logical addresses untranslated.
    if (cbar_ == 0xFF) {
        return logical_address;
    }

    const auto ca0_end_exclusive = static_cast<std::uint16_t>((cbar_ >> 4) << 12);
    const auto ca1_start = static_cast<std::uint16_t>((cbar_ & 0x0F) << 12);

    if (logical_address < ca0_end_exclusive) {
        return logical_address;
    }

    if (logical_address >= ca1_start) {
        return (static_cast<std::uint32_t>(cbr_) << 12) + (logical_address - ca1_start);
    }

    return (static_cast<std::uint32_t>(bbr_) << 12) + (logical_address - ca0_end_exclusive);
}

auto Core::read_logical(const std::uint16_t logical_address) -> std::uint8_t {
    return callbacks_.read_memory(translate_logical_address(logical_address));
}

void Core::write_logical(const std::uint16_t logical_address, const std::uint8_t value) {
    callbacks_.write_memory(translate_logical_address(logical_address), value);
}

auto Core::service_pending_interrupt(const bool int0_asserted, const bool int1_asserted)
    -> std::optional<InterruptService> {
    if (int0_asserted && iff1_ && interrupt_mode_ == 1) {
        halted_ = false;
        iff1_ = false;
        iff2_ = false;
        push_word(pc_.value);
        pc_.value = 0x0038;
        return InterruptService{.source = InterruptSource::int0, .handler_address = pc_.value};
    }

    if (int1_asserted && (itc_ & itc_ite1) != 0U) {
        halted_ = false;
        iff1_ = false;
        iff2_ = false;
        const auto vector_pointer =
            static_cast<std::uint16_t>((static_cast<std::uint16_t>(i_) << 8) | (il_ & 0xF8));
        const auto handler_address = read_word(vector_pointer);
        push_word(pc_.value);
        pc_.value = handler_address;
        if (callbacks_.acknowledge_int1) {
            callbacks_.acknowledge_int1();
        }
        return InterruptService{.source = InterruptSource::int1, .handler_address = pc_.value};
    }

    return std::nullopt;
}

void Core::run_until_halt(const std::size_t max_instructions) {
    std::size_t executed = 0;
    while (!halted_) {
        if (executed >= max_instructions) {
            throw std::runtime_error("Test ROM did not halt within the instruction budget.");
        }
        execute_one();
        ++executed;
    }
}

void Core::initialize_tables() {
    opcodes_.assign(256, &Core::op_unimplemented);
    ed_opcodes_.assign(256, &Core::op_unimplemented);

    opcodes_[0x00] = &Core::op_nop;
    opcodes_[0x21] = &Core::op_ld_hl_nn;
    opcodes_[0x22] = &Core::op_ld_mem_nn_hl;
    opcodes_[0x31] = &Core::op_ld_sp_nn;
    opcodes_[0x32] = &Core::op_ld_mem_nn_a;
    opcodes_[0x3A] = &Core::op_ld_a_mem_nn;
    opcodes_[0x3E] = &Core::op_ld_a_n;
    opcodes_[0x76] = &Core::op_halt;
    opcodes_[0xC3] = &Core::op_jp_nn;
    opcodes_[0xC9] = &Core::op_ret;
    opcodes_[0xCD] = &Core::op_call_nn;
    opcodes_[0xED] = &Core::op_ed_prefix;
    opcodes_[0xF3] = &Core::op_di;
    opcodes_[0xFB] = &Core::op_ei;

    ed_opcodes_[0x39] = &Core::op_ed_out0_n_a;
    ed_opcodes_[0x47] = &Core::op_ed_ld_i_a;
    ed_opcodes_[0x4D] = &Core::op_ed_reti;
}

auto Core::fetch_byte() -> std::uint8_t {
    const auto value = read_logical(pc_.value);
    pc_.value = static_cast<std::uint16_t>(pc_.value + 1U);
    return value;
}

auto Core::fetch_word() -> std::uint16_t {
    const auto low = fetch_byte();
    const auto high = fetch_byte();
    return static_cast<std::uint16_t>(low | (static_cast<std::uint16_t>(high) << 8));
}

void Core::write_word(const std::uint16_t address, const std::uint16_t value) {
    write_logical(address, static_cast<std::uint8_t>(value & 0x00FFU));
    write_logical(
        static_cast<std::uint16_t>(address + 1U),
        static_cast<std::uint8_t>((value >> 8) & 0x00FFU)
    );
}

auto Core::read_word(const std::uint16_t address) -> std::uint16_t {
    const auto low = read_logical(address);
    const auto high = read_logical(static_cast<std::uint16_t>(address + 1U));
    return static_cast<std::uint16_t>(low | (static_cast<std::uint16_t>(high) << 8));
}

void Core::push_word(const std::uint16_t value) {
    sp_.value = static_cast<std::uint16_t>(sp_.value - 2U);
    write_word(sp_.value, value);
}

auto Core::pop_word() -> std::uint16_t {
    const auto value = read_word(sp_.value);
    sp_.value = static_cast<std::uint16_t>(sp_.value + 2U);
    return value;
}

void Core::execute_one() {
    ++r_;
    const auto opcode = fetch_byte();
    (this->*opcodes_[opcode])();
}

void Core::maybe_warn_illegal_bbr() {
    if (bbr_ >= 0xF0U && callbacks_.record_warning) {
        callbacks_.record_warning(
            "Illegal BBR write: " + hex_string(bbr_, 2) + " at PC=" + hex_string(pc_.value, 4) +
            " - maps into SRAM region"
        );
    }
}

void Core::acknowledge_reti() {}

[[noreturn]] void Core::unsupported_opcode(const std::uint8_t opcode) {
    throw std::runtime_error("Unsupported extracted Z180 opcode " + hex_string(opcode, 2));
}

[[noreturn]] void Core::unsupported_ed_opcode(const std::uint8_t opcode) {
    throw std::runtime_error("Unsupported extracted Z180 ED opcode " + hex_string(opcode, 2));
}

void Core::op_unimplemented() { unsupported_opcode(read_logical(static_cast<std::uint16_t>(pc_.value - 1U))); }

// Extracted and adapted from MAME z180op.hxx milestone-2 opcode subset.
void Core::op_nop() {}

void Core::op_ld_hl_nn() { hl_.value = fetch_word(); }

void Core::op_ld_mem_nn_hl() { write_word(fetch_word(), hl_.value); }

void Core::op_ld_sp_nn() { sp_.value = fetch_word(); }

void Core::op_ld_mem_nn_a() { write_logical(fetch_word(), af_.bytes.hi); }

void Core::op_ld_a_mem_nn() { af_.bytes.hi = read_logical(fetch_word()); }

void Core::op_ld_a_n() { af_.bytes.hi = fetch_byte(); }

void Core::op_halt() {
    pc_.value = static_cast<std::uint16_t>(pc_.value - 1U);
    halted_ = true;
}

void Core::op_jp_nn() { pc_.value = fetch_word(); }

void Core::op_ret() { pc_.value = pop_word(); }

void Core::op_call_nn() {
    const auto target = fetch_word();
    push_word(pc_.value);
    pc_.value = target;
}

void Core::op_di() { iff1_ = false; iff2_ = false; }

void Core::op_ei() { iff1_ = true; iff2_ = true; }

void Core::op_ed_prefix() {
    const auto opcode = fetch_byte();
    const auto handler = ed_opcodes_[opcode];
    if (handler == &Core::op_unimplemented) {
        unsupported_ed_opcode(opcode);
    }
    (this->*handler)();
}

void Core::op_ed_out0_n_a() { out0(fetch_byte(), af_.bytes.hi); }

void Core::op_ed_ld_i_a() { i_ = af_.bytes.hi; }

void Core::op_ed_reti() {
    pc_.value = pop_word();
    acknowledge_reti();
}

}  // namespace vanguard8::third_party::z180

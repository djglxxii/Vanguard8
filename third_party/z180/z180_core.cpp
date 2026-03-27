#include "third_party/z180/z180_core.hpp"

#include <iomanip>
#include <sstream>

namespace vanguard8::third_party::z180 {

namespace {

constexpr std::uint8_t flag_zero = 0x40;
constexpr std::uint8_t itc_ite1 = 0x02;
constexpr std::uint8_t vector_code_int1 = 0x00;
constexpr std::uint8_t vector_code_prt0 = 0x04;
constexpr std::uint8_t vector_code_prt1 = 0x06;
constexpr std::uint8_t tcr_tif1 = 0x80;
constexpr std::uint8_t tcr_tif0 = 0x40;
constexpr std::uint8_t tcr_tie1 = 0x20;
constexpr std::uint8_t tcr_tie0 = 0x10;
constexpr std::uint8_t tcr_tde1 = 0x02;
constexpr std::uint8_t tcr_tde0 = 0x01;

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
    prt0_ = {};
    prt1_ = {};
    prt0_.tmdr.value = 0xFFFF;
    prt0_.rldr.value = 0xFFFF;
    prt1_.tmdr.value = 0xFFFF;
    prt1_.rldr.value = 0xFFFF;
    prt0_.high_read_latch = 0xFF;
    prt1_.high_read_latch = 0xFF;
    prt_prescaler_ = 0x00;
    tcr_ = 0x00;
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

auto Core::register_snapshot() const -> RegisterSnapshot {
    return RegisterSnapshot{
        .af = af_.value,
        .bc = bc_.value,
        .de = de_.value,
        .hl = hl_.value,
        .sp = sp_.value,
        .pc = pc_.value,
        .af_alt = af2_.value,
        .bc_alt = bc2_.value,
        .de_alt = de2_.value,
        .hl_alt = hl2_.value,
        .ix = ix_.value,
        .iy = iy_.value,
        .r = r_,
        .i = i_,
        .il = il_,
        .itc = itc_,
        .cbar = cbar_,
        .cbr = cbr_,
        .bbr = bbr_,
        .interrupt_mode = interrupt_mode_,
        .iff1 = iff1_,
        .iff2 = iff2_,
        .halted = halted_,
        .tcr = tcr_,
        .tmdr0 = prt0_.tmdr.value,
        .rldr0 = prt0_.rldr.value,
        .tmdr1 = prt1_.tmdr.value,
        .rldr1 = prt1_.rldr.value,
    };
}

void Core::load_register_snapshot(const RegisterSnapshot& state) {
    af_.value = state.af;
    bc_.value = state.bc;
    de_.value = state.de;
    hl_.value = state.hl;
    sp_.value = state.sp;
    pc_.value = state.pc;
    af2_.value = state.af_alt;
    bc2_.value = state.bc_alt;
    de2_.value = state.de_alt;
    hl2_.value = state.hl_alt;
    ix_.value = state.ix;
    iy_.value = state.iy;
    r_ = state.r;
    i_ = state.i;
    il_ = state.il;
    itc_ = state.itc;
    cbar_ = state.cbar;
    cbr_ = state.cbr;
    bbr_ = state.bbr;
    interrupt_mode_ = state.interrupt_mode;
    iff1_ = state.iff1;
    iff2_ = state.iff2;
    halted_ = state.halted;
    tcr_ = state.tcr;
    prt0_.tmdr.value = state.tmdr0;
    prt0_.rldr.value = state.rldr0;
    prt1_.tmdr.value = state.tmdr1;
    prt1_.rldr.value = state.rldr1;
}

void Core::set_register_i(const std::uint8_t value) { i_ = value; }

void Core::set_interrupt_mode(const std::uint8_t mode) { interrupt_mode_ = static_cast<std::uint8_t>(mode & 0x03U); }

void Core::set_iff1(const bool enabled) { iff1_ = enabled; }

void Core::set_iff2(const bool enabled) { iff2_ = enabled; }

void Core::advance_tstates(const std::uint64_t tstates) {
    for (std::uint64_t count = 0; count < tstates; ++count) {
        ++prt_prescaler_;
        if (prt_prescaler_ < 20U) {
            continue;
        }

        prt_prescaler_ = 0U;
        for (int channel = 0; channel < 2; ++channel) {
            const auto tde_mask = channel == 0 ? tcr_tde0 : tcr_tde1;
            const auto tif_mask = channel == 0 ? tcr_tif0 : tcr_tif1;
            if ((tcr_ & tde_mask) == 0U) {
                continue;
            }

            auto& prt = timer_channel(channel);
            if (prt.tmdr.value > 0U) {
                --prt.tmdr.value;
            }

            if (prt.tmdr.value == 0U) {
                tcr_ = static_cast<std::uint8_t>(tcr_ | tif_mask);
                prt.tif_clear_armed = false;
                prt.tmdr.value = prt.rldr.value;
            }
        }
    }
}

auto Core::in0(const std::uint8_t port) -> std::uint8_t {
    auto observe = [this, port](const std::uint8_t value) {
        if (callbacks_.observe_internal_io_read) {
            callbacks_.observe_internal_io_read(port, value);
        }
        return value;
    };

    switch (port) {
    case 0x0C:
        prt0_.high_read_latch = prt0_.tmdr.bytes.hi;
        clear_timer_flag_on_tmdr_read(0);
        return observe(prt0_.tmdr.bytes.lo);
    case 0x0D:
        clear_timer_flag_on_tmdr_read(0);
        return observe(prt0_.high_read_latch);
    case 0x0E:
        return observe(prt0_.rldr.bytes.lo);
    case 0x0F:
        return observe(prt0_.rldr.bytes.hi);
    case 0x10:
        prt0_.tif_clear_armed = (tcr_ & tcr_tif0) != 0U;
        prt1_.tif_clear_armed = (tcr_ & tcr_tif1) != 0U;
        return observe(tcr_);
    case 0x14:
        prt1_.high_read_latch = prt1_.tmdr.bytes.hi;
        clear_timer_flag_on_tmdr_read(1);
        return observe(prt1_.tmdr.bytes.lo);
    case 0x15:
        clear_timer_flag_on_tmdr_read(1);
        return observe(prt1_.high_read_latch);
    case 0x16:
        return observe(prt1_.rldr.bytes.lo);
    case 0x17:
        return observe(prt1_.rldr.bytes.hi);
    case 0x33:
        return observe(il_);
    case 0x34:
        return observe(itc_);
    case 0x38:
        return observe(cbr_);
    case 0x39:
        return observe(bbr_);
    case 0x3A:
        return observe(cbar_);
    default:
        return observe(0x00);
    }
}

void Core::out0(const std::uint8_t port, const std::uint8_t value) {
    switch (port) {
    case 0x0C:
        prt0_.tmdr.bytes.lo = value;
        break;
    case 0x0D:
        prt0_.tmdr.bytes.hi = value;
        break;
    case 0x0E:
        prt0_.rldr.bytes.lo = value;
        break;
    case 0x0F:
        prt0_.rldr.bytes.hi = value;
        break;
    case 0x10:
        tcr_ = static_cast<std::uint8_t>((tcr_ & (tcr_tif1 | tcr_tif0)) | (value & 0x3FU));
        break;
    case 0x14:
        prt1_.tmdr.bytes.lo = value;
        break;
    case 0x15:
        prt1_.tmdr.bytes.hi = value;
        break;
    case 0x16:
        prt1_.rldr.bytes.lo = value;
        break;
    case 0x17:
        prt1_.rldr.bytes.hi = value;
        break;
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

    if (callbacks_.observe_internal_io_write) {
        callbacks_.observe_internal_io_write(port, value);
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
    const auto value = callbacks_.read_memory(translate_logical_address(logical_address));
    if (callbacks_.observe_logical_memory_read) {
        callbacks_.observe_logical_memory_read(logical_address, value);
    }
    return value;
}

void Core::write_logical(const std::uint16_t logical_address, const std::uint8_t value) {
    callbacks_.write_memory(translate_logical_address(logical_address), value);
    if (callbacks_.observe_logical_memory_write) {
        callbacks_.observe_logical_memory_write(logical_address, value);
    }
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

    if (int1_asserted && iff1_ && (itc_ & itc_ite1) != 0U) {
        service_vectored_interrupt(InterruptSource::int1, vector_code_int1);
        if (callbacks_.acknowledge_int1) {
            callbacks_.acknowledge_int1();
        }
        return InterruptService{.source = InterruptSource::int1, .handler_address = pc_.value};
    }

    if (timer_interrupt_pending(0)) {
        service_vectored_interrupt(InterruptSource::prt0, vector_code_prt0);
        return InterruptService{.source = InterruptSource::prt0, .handler_address = pc_.value};
    }

    if (timer_interrupt_pending(1)) {
        service_vectored_interrupt(InterruptSource::prt1, vector_code_prt1);
        return InterruptService{.source = InterruptSource::prt1, .handler_address = pc_.value};
    }

    return std::nullopt;
}

void Core::step_one() { execute_one(); }

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

auto Core::timer_channel(const int index) -> TimerChannel& { return index == 0 ? prt0_ : prt1_; }

auto Core::timer_channel(const int index) const -> const TimerChannel& { return index == 0 ? prt0_ : prt1_; }

auto Core::timer_interrupt_pending(const int index) const -> bool {
    if (!iff1_) {
        return false;
    }

    const auto tie_mask = index == 0 ? tcr_tie0 : tcr_tie1;
    const auto tif_mask = index == 0 ? tcr_tif0 : tcr_tif1;
    return (tcr_ & tie_mask) != 0U && (tcr_ & tif_mask) != 0U;
}

void Core::clear_timer_flag_on_tmdr_read(const int index) {
    auto& prt = timer_channel(index);
    if (!prt.tif_clear_armed) {
        return;
    }

    const auto tif_mask = index == 0 ? tcr_tif0 : tcr_tif1;
    tcr_ = static_cast<std::uint8_t>(tcr_ & ~tif_mask);
    prt.tif_clear_armed = false;
}

auto Core::vectored_handler_address(const std::uint8_t fixed_code) -> std::uint16_t {
    const auto vector_pointer =
        static_cast<std::uint16_t>((static_cast<std::uint16_t>(i_) << 8) | ((il_ & 0xE0U) | fixed_code));
    return read_word(vector_pointer);
}

void Core::service_vectored_interrupt(const InterruptSource source, const std::uint8_t fixed_code) {
    static_cast<void>(source);
    halted_ = false;
    iff1_ = false;
    iff2_ = false;
    const auto handler_address = vectored_handler_address(fixed_code);
    push_word(pc_.value);
    pc_.value = handler_address;
}

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

#include "third_party/z180/z180_core.hpp"

#include <iomanip>
#include <sstream>
#include <utility>

namespace vanguard8::third_party::z180 {

namespace {

constexpr std::uint8_t flag_sign = 0x80;
constexpr std::uint8_t flag_zero = 0x40;
constexpr std::uint8_t flag_half = 0x10;
constexpr std::uint8_t flag_parity_overflow = 0x04;
constexpr std::uint8_t flag_subtract = 0x02;
constexpr std::uint8_t flag_carry = 0x01;
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

[[nodiscard]] auto has_even_parity(const std::uint8_t value) -> bool {
    auto bits = value;
    bool parity = true;
    while (bits != 0U) {
        parity = !parity;
        bits = static_cast<std::uint8_t>(bits & static_cast<std::uint8_t>(bits - 1U));
    }
    return parity;
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
    ei_delay_ = 0;
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
        .ei_delay = ei_delay_,
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
    ei_delay_ = state.ei_delay;
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
        wake_from_halt_for_interrupt();
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

    for (std::size_t i = 0x40U; i <= 0x7FU; ++i) {
        opcodes_[i] = &Core::op_ld_r_r_main;
    }

    opcodes_[0x00] = &Core::op_nop;
    opcodes_[0x01] = &Core::op_ld_bc_nn;
    opcodes_[0x03] = &Core::op_inc_bc;
    opcodes_[0x04] = &Core::op_inc_r_main;
    opcodes_[0x05] = &Core::op_dec_r_main;
    opcodes_[0x0C] = &Core::op_inc_r_main;
    opcodes_[0x0D] = &Core::op_dec_r_main;
    opcodes_[0x09] = &Core::op_add_hl_ss_main;
    opcodes_[0x0B] = &Core::op_dec_bc;
    opcodes_[0x10] = &Core::op_djnz_e;
    opcodes_[0x13] = &Core::op_inc_de;
    opcodes_[0x14] = &Core::op_inc_r_main;
    opcodes_[0x15] = &Core::op_dec_r_main;
    opcodes_[0x1C] = &Core::op_inc_r_main;
    opcodes_[0x1D] = &Core::op_dec_r_main;
    opcodes_[0x19] = &Core::op_add_hl_ss_main;
    opcodes_[0x24] = &Core::op_inc_r_main;
    opcodes_[0x25] = &Core::op_dec_r_main;
    opcodes_[0x29] = &Core::op_add_hl_ss_main;
    opcodes_[0x2C] = &Core::op_inc_r_main;
    opcodes_[0x2D] = &Core::op_dec_r_main;
    opcodes_[0x30] = &Core::op_jr_nc_e;
    opcodes_[0x34] = &Core::op_inc_r_main;
    opcodes_[0x35] = &Core::op_dec_r_main;
    opcodes_[0x37] = &Core::op_scf;
    opcodes_[0x38] = &Core::op_jr_c_e;
    opcodes_[0x39] = &Core::op_add_hl_ss_main;
    opcodes_[0x3C] = &Core::op_inc_r_main;
    opcodes_[0x3D] = &Core::op_dec_r_main;
    opcodes_[0x06] = &Core::op_ld_b_n;
    opcodes_[0x0E] = &Core::op_ld_c_n;
    opcodes_[0x11] = &Core::op_ld_de_nn;
    opcodes_[0x16] = &Core::op_ld_d_n;
    opcodes_[0x1E] = &Core::op_ld_e_n;
    opcodes_[0x26] = &Core::op_ld_h_n;
    opcodes_[0x2E] = &Core::op_ld_l_n;
    opcodes_[0x1B] = &Core::op_dec_de;
    opcodes_[0x0F] = &Core::op_rrca;
    opcodes_[0x18] = &Core::op_jr_e;
    opcodes_[0x20] = &Core::op_jr_nz_e;
    opcodes_[0x28] = &Core::op_jr_z_e;
    opcodes_[0x21] = &Core::op_ld_hl_nn;
    opcodes_[0x22] = &Core::op_ld_mem_nn_hl;
    opcodes_[0x23] = &Core::op_inc_hl;
    opcodes_[0x2A] = &Core::op_ld_hl_mem_nn;
    opcodes_[0x2B] = &Core::op_dec_hl;
    opcodes_[0x2F] = &Core::op_cpl;
    opcodes_[0x31] = &Core::op_ld_sp_nn;
    opcodes_[0x32] = &Core::op_ld_mem_nn_a;
    opcodes_[0x3A] = &Core::op_ld_a_mem_nn;
    opcodes_[0x3E] = &Core::op_ld_a_n;
    opcodes_[0x78] = &Core::op_ld_a_b;
    opcodes_[0x79] = &Core::op_ld_a_c;
    opcodes_[0x7A] = &Core::op_ld_a_d;
    opcodes_[0x7B] = &Core::op_ld_a_e;
    opcodes_[0x7C] = &Core::op_ld_a_h;
    opcodes_[0x7D] = &Core::op_ld_a_l;
    opcodes_[0x7E] = &Core::op_ld_a_mem_hl;
    opcodes_[0x7F] = &Core::op_ld_a_a;
    for (std::size_t i = 0x80U; i <= 0x87U; ++i) {
        opcodes_[i] = &Core::op_add_a_r_main;
    }
    opcodes_[0x91] = &Core::op_sub_a_r_main;
    opcodes_[0x93] = &Core::op_sub_a_r_main;
    opcodes_[0x76] = &Core::op_halt;
    opcodes_[0xAF] = &Core::op_xor_a;
    opcodes_[0xC2] = &Core::op_jp_nz_nn;
    opcodes_[0xC3] = &Core::op_jp_nn;
    opcodes_[0xCA] = &Core::op_jp_z_nn;
    opcodes_[0xC0] = &Core::op_ret_nz;
    opcodes_[0xC8] = &Core::op_ret_z;
    opcodes_[0xD0] = &Core::op_ret_nc;
    opcodes_[0xD8] = &Core::op_ret_c;
    opcodes_[0xC9] = &Core::op_ret;
    opcodes_[0xCD] = &Core::op_call_nn;
    opcodes_[0xD3] = &Core::op_out_n_a;
    opcodes_[0xDB] = &Core::op_in_a_n;
    opcodes_[0xCB] = &Core::op_cb_prefix;
    opcodes_[0xED] = &Core::op_ed_prefix;
    opcodes_[0xB0] = &Core::op_or_b;
    opcodes_[0xB1] = &Core::op_or_c;
    opcodes_[0xB2] = &Core::op_or_d;
    opcodes_[0xB3] = &Core::op_or_e;
    opcodes_[0xB4] = &Core::op_or_h;
    opcodes_[0xB5] = &Core::op_or_l;
    opcodes_[0xB6] = &Core::op_or_mem_hl;
    opcodes_[0xB7] = &Core::op_or_a;
    opcodes_[0xC1] = &Core::op_pop_bc;
    opcodes_[0xC5] = &Core::op_push_bc;
    opcodes_[0xD1] = &Core::op_pop_de;
    opcodes_[0xD5] = &Core::op_push_de;
    opcodes_[0xC6] = &Core::op_add_a_n;
    opcodes_[0xE1] = &Core::op_pop_hl;
    opcodes_[0xE5] = &Core::op_push_hl;
    opcodes_[0xEB] = &Core::op_ex_de_hl;
    opcodes_[0xE6] = &Core::op_and_n;
    opcodes_[0xF6] = &Core::op_or_n;
    opcodes_[0xFE] = &Core::op_cp_n;
    opcodes_[0xB8] = &Core::op_cp_r_main;
    opcodes_[0xB9] = &Core::op_cp_r_main;
    opcodes_[0xBA] = &Core::op_cp_r_main;
    opcodes_[0xBB] = &Core::op_cp_r_main;
    opcodes_[0xBC] = &Core::op_cp_r_main;
    opcodes_[0xBD] = &Core::op_cp_r_main;
    opcodes_[0xBE] = &Core::op_cp_r_main;
    opcodes_[0xBF] = &Core::op_cp_r_main;
    opcodes_[0xF1] = &Core::op_pop_af;
    opcodes_[0xF3] = &Core::op_di;
    opcodes_[0xF5] = &Core::op_push_af;
    opcodes_[0xFB] = &Core::op_ei;

    ed_opcodes_[0x39] = &Core::op_ed_out0_n_a;
    ed_opcodes_[0x47] = &Core::op_ed_ld_i_a;
    ed_opcodes_[0x4D] = &Core::op_ed_reti;
    ed_opcodes_[0x56] = &Core::op_ed_im_1;
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
    last_opcode_ = opcode;
    (this->*opcodes_[opcode])();
    if (ei_delay_ > 0U) {
        --ei_delay_;
        if (ei_delay_ == 0U) {
            iff1_ = true;
            iff2_ = true;
        }
    }
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
    wake_from_halt_for_interrupt();
    iff1_ = false;
    iff2_ = false;
    const auto handler_address = vectored_handler_address(fixed_code);
    push_word(pc_.value);
    pc_.value = handler_address;
}

void Core::wake_from_halt_for_interrupt() {
    // `op_halt` decrements `pc_` so a snapshot taken mid-halt points at the
    // HALT opcode itself. Before pushing `pc_` as the interrupt return
    // address, bump it past the HALT so `RETI` resumes at the instruction
    // after HALT instead of re-entering halt forever.
    if (halted_) {
        halted_ = false;
        pc_.value = static_cast<std::uint16_t>(pc_.value + 1U);
    }
}

auto Core::register8_from_code(const std::uint8_t code) -> std::uint8_t& {
    switch (code & 0x07U) {
    case 0x00:
        return bc_.bytes.hi;
    case 0x01:
        return bc_.bytes.lo;
    case 0x02:
        return de_.bytes.hi;
    case 0x03:
        return de_.bytes.lo;
    case 0x04:
        return hl_.bytes.hi;
    case 0x05:
        return hl_.bytes.lo;
    case 0x07:
        return af_.bytes.hi;
    default:
        throw std::runtime_error("Unsupported HD64180 register selector " + hex_string(code, 2));
    }
}

auto Core::register_pair_from_code(const std::uint8_t code) -> Pair& {
    switch (code & 0x03U) {
    case 0x00:
        return bc_;
    case 0x01:
        return de_;
    case 0x02:
        return hl_;
    case 0x03:
        return sp_;
    default:
        throw std::runtime_error("Unsupported HD64180 register-pair selector " + hex_string(code, 2));
    }
}

void Core::update_tst_flags(const std::uint8_t value) {
    af_.bytes.lo = 0x00;
    if ((value & 0x80U) != 0U) {
        af_.bytes.lo = static_cast<std::uint8_t>(af_.bytes.lo | flag_sign);
    }
    if (value == 0U) {
        af_.bytes.lo = static_cast<std::uint8_t>(af_.bytes.lo | flag_zero);
    }
    if (has_even_parity(value)) {
        af_.bytes.lo = static_cast<std::uint8_t>(af_.bytes.lo | flag_parity_overflow);
    }
    af_.bytes.lo = static_cast<std::uint8_t>(af_.bytes.lo | flag_half);
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

void Core::op_dec_r_main() {
    const auto reg_code = static_cast<std::uint8_t>((last_opcode_ >> 3U) & 0x07U);
    std::uint8_t old_value = 0U;
    std::uint8_t result = 0U;
    if (reg_code == 0x06U) {
        old_value = read_logical(hl_.value);
        result = static_cast<std::uint8_t>(old_value - 1U);
        write_logical(hl_.value, result);
    } else {
        auto& reg = register8_from_code(reg_code);
        old_value = reg;
        result = static_cast<std::uint8_t>(old_value - 1U);
        reg = result;
    }
    auto flags = static_cast<std::uint8_t>(af_.bytes.lo & flag_carry);
    flags = static_cast<std::uint8_t>(flags | flag_subtract);
    if ((result & 0x80U) != 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_sign);
    }
    if (result == 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_zero);
    }
    if ((old_value & 0x0FU) == 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_half);
    }
    if (old_value == 0x80U) {
        flags = static_cast<std::uint8_t>(flags | flag_parity_overflow);
    }
    af_.bytes.lo = flags;
}

void Core::op_inc_r_main() {
    const auto reg_code = static_cast<std::uint8_t>((last_opcode_ >> 3U) & 0x07U);
    std::uint8_t old_value = 0U;
    std::uint8_t result = 0U;
    if (reg_code == 0x06U) {
        old_value = read_logical(hl_.value);
        result = static_cast<std::uint8_t>(old_value + 1U);
        write_logical(hl_.value, result);
    } else {
        auto& reg = register8_from_code(reg_code);
        old_value = reg;
        result = static_cast<std::uint8_t>(old_value + 1U);
        reg = result;
    }
    auto flags = static_cast<std::uint8_t>(af_.bytes.lo & flag_carry);
    if ((result & 0x80U) != 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_sign);
    }
    if (result == 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_zero);
    }
    if ((old_value & 0x0FU) == 0x0FU) {
        flags = static_cast<std::uint8_t>(flags | flag_half);
    }
    if (old_value == 0x7FU) {
        flags = static_cast<std::uint8_t>(flags | flag_parity_overflow);
    }
    af_.bytes.lo = flags;
}

void Core::op_dec_b() {
    const auto old_value = bc_.bytes.hi;
    const auto result = static_cast<std::uint8_t>(old_value - 1U);
    bc_.bytes.hi = result;

    auto flags = static_cast<std::uint8_t>(af_.bytes.lo & flag_carry);
    flags = static_cast<std::uint8_t>(flags | flag_subtract);
    if ((result & 0x80U) != 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_sign);
    }
    if (result == 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_zero);
    }
    if ((old_value & 0x0FU) == 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_half);
    }
    if (old_value == 0x80U) {
        flags = static_cast<std::uint8_t>(flags | flag_parity_overflow);
    }
    af_.bytes.lo = flags;
}

void Core::op_ld_b_n() { bc_.bytes.hi = fetch_byte(); }

void Core::op_ld_c_n() { bc_.bytes.lo = fetch_byte(); }

void Core::op_ld_d_n() { de_.bytes.hi = fetch_byte(); }

void Core::op_ld_e_n() { de_.bytes.lo = fetch_byte(); }

void Core::op_ld_h_n() { hl_.bytes.hi = fetch_byte(); }

void Core::op_ld_l_n() { hl_.bytes.lo = fetch_byte(); }

void Core::op_djnz_e() {
    const auto displacement = static_cast<std::int8_t>(fetch_byte());
    bc_.bytes.hi = static_cast<std::uint8_t>(bc_.bytes.hi - 1U);
    if (bc_.bytes.hi != 0U) {
        pc_.value = static_cast<std::uint16_t>(pc_.value + displacement);
    }
}

void Core::op_rrca() {
    const auto value = af_.bytes.hi;
    const auto carry = static_cast<std::uint8_t>(value & 0x01U);
    af_.bytes.hi = static_cast<std::uint8_t>((value >> 1U) | (carry << 7U));
    af_.bytes.lo = static_cast<std::uint8_t>((af_.bytes.lo & (flag_sign | flag_zero | flag_parity_overflow)) | carry);
}

void Core::op_inc_bc() { bc_.value = static_cast<std::uint16_t>(bc_.value + 1U); }

void Core::op_dec_bc() { bc_.value = static_cast<std::uint16_t>(bc_.value - 1U); }

void Core::op_inc_de() { de_.value = static_cast<std::uint16_t>(de_.value + 1U); }

void Core::op_inc_hl() { hl_.value = static_cast<std::uint16_t>(hl_.value + 1U); }

void Core::op_dec_hl() { hl_.value = static_cast<std::uint16_t>(hl_.value - 1U); }

void Core::op_jr_nz_e() {
    const auto displacement = static_cast<std::int8_t>(fetch_byte());
    if ((af_.bytes.lo & flag_zero) == 0U) {
        pc_.value = static_cast<std::uint16_t>(pc_.value + displacement);
    }
}

void Core::op_jr_nc_e() {
    const auto displacement = static_cast<std::int8_t>(fetch_byte());
    if ((af_.bytes.lo & flag_carry) == 0U) {
        pc_.value = static_cast<std::uint16_t>(pc_.value + displacement);
    }
}

void Core::op_jr_z_e() {
    const auto displacement = static_cast<std::int8_t>(fetch_byte());
    if ((af_.bytes.lo & flag_zero) != 0U) {
        pc_.value = static_cast<std::uint16_t>(pc_.value + displacement);
    }
}

void Core::op_jr_c_e() {
    const auto displacement = static_cast<std::int8_t>(fetch_byte());
    if ((af_.bytes.lo & flag_carry) != 0U) {
        pc_.value = static_cast<std::uint16_t>(pc_.value + displacement);
    }
}

void Core::op_ld_bc_nn() { bc_.value = fetch_word(); }

void Core::op_ld_de_nn() { de_.value = fetch_word(); }

void Core::op_ld_hl_nn() { hl_.value = fetch_word(); }

void Core::op_ld_hl_mem_nn() { hl_.value = read_word(fetch_word()); }

void Core::op_ld_mem_nn_hl() { write_word(fetch_word(), hl_.value); }

void Core::op_ld_sp_nn() { sp_.value = fetch_word(); }

void Core::op_ld_mem_nn_a() { write_logical(fetch_word(), af_.bytes.hi); }

void Core::op_ld_a_b() { af_.bytes.hi = bc_.bytes.hi; }

void Core::op_ld_a_c() { af_.bytes.hi = bc_.bytes.lo; }

void Core::op_ld_a_d() { af_.bytes.hi = de_.bytes.hi; }

void Core::op_ld_a_e() { af_.bytes.hi = de_.bytes.lo; }

void Core::op_ld_a_h() { af_.bytes.hi = hl_.bytes.hi; }

void Core::op_ld_a_l() { af_.bytes.hi = hl_.bytes.lo; }

void Core::op_ld_a_a() {}

void Core::op_ld_a_mem_hl() { af_.bytes.hi = read_logical(hl_.value); }

void Core::op_ld_r_r_main() {
    const auto dst = static_cast<std::uint8_t>((last_opcode_ >> 3U) & 0x07U);
    const auto src = static_cast<std::uint8_t>(last_opcode_ & 0x07U);
    if (src == 0x06U) {
        register8_from_code(dst) = read_logical(hl_.value);
        return;
    }
    if (dst == 0x06U) {
        write_logical(hl_.value, register8_from_code(src));
        return;
    }
    register8_from_code(dst) = register8_from_code(src);
}

void Core::op_ld_a_mem_nn() { af_.bytes.hi = read_logical(fetch_word()); }

void Core::op_ld_a_n() { af_.bytes.hi = fetch_byte(); }

void Core::op_add_a_r_main() {
    const auto reg_code = static_cast<std::uint8_t>(last_opcode_ & 0x07U);
    const auto operand = (reg_code == 0x06U) ? read_logical(hl_.value) : register8_from_code(reg_code);
    const auto old_a = af_.bytes.hi;
    const auto result = static_cast<std::uint16_t>(old_a) + operand;
    af_.bytes.hi = static_cast<std::uint8_t>(result & 0x00FFU);
    apply_add_flags(old_a, operand, result);
}

void Core::apply_add_flags(const std::uint8_t old_a, const std::uint8_t operand, const std::uint16_t result) {
    const auto result8 = static_cast<std::uint8_t>(result & 0x00FFU);
    std::uint8_t flags = 0;
    if ((result8 & 0x80U) != 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_sign);
    }
    if (result8 == 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_zero);
    }
    if (((old_a & 0x0FU) + (operand & 0x0FU)) > 0x0FU) {
        flags = static_cast<std::uint8_t>(flags | flag_half);
    }
    if (((~(old_a ^ operand)) & (old_a ^ result8) & 0x80U) != 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_parity_overflow);
    }
    if (result > 0x00FFU) {
        flags = static_cast<std::uint8_t>(flags | flag_carry);
    }
    af_.bytes.lo = flags;
}

void Core::op_add_a_n() {
    const auto old_a = af_.bytes.hi;
    const auto operand = fetch_byte();
    const auto result = static_cast<std::uint16_t>(old_a) + operand;
    af_.bytes.hi = static_cast<std::uint8_t>(result & 0x00FFU);
    apply_add_flags(old_a, operand, result);
}

void Core::apply_sub_flags(const std::uint8_t old_a, const std::uint8_t operand, const std::uint8_t result) {
    std::uint8_t flags = flag_subtract;
    if ((result & 0x80U) != 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_sign);
    }
    if (result == 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_zero);
    }
    if (((old_a ^ operand ^ result) & 0x10U) != 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_half);
    }
    if (((old_a ^ operand) & (old_a ^ result) & 0x80U) != 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_parity_overflow);
    }
    if (old_a < operand) {
        flags = static_cast<std::uint8_t>(flags | flag_carry);
    }
    af_.bytes.lo = flags;
}

void Core::op_sub_a_r_main() {
    const auto reg_code = static_cast<std::uint8_t>(last_opcode_ & 0x07U);
    const auto operand = register8_from_code(reg_code);
    const auto old_a = af_.bytes.hi;
    const auto result = static_cast<std::uint8_t>(old_a - operand);
    af_.bytes.hi = result;
    apply_sub_flags(old_a, operand, result);
}

void Core::op_add_hl_ss_main() {
    const auto pair_code = static_cast<std::uint8_t>((last_opcode_ >> 4U) & 0x03U);
    const auto operand = register_pair_from_code(pair_code).value;
    const auto old_hl = hl_.value;
    const auto result = static_cast<std::uint32_t>(old_hl) + operand;

    hl_.value = static_cast<std::uint16_t>(result & 0xFFFFU);

    auto flags = static_cast<std::uint8_t>(af_.bytes.lo & (flag_sign | flag_zero | flag_parity_overflow));
    if (((old_hl & 0x0FFFU) + (operand & 0x0FFFU)) > 0x0FFFU) {
        flags = static_cast<std::uint8_t>(flags | flag_half);
    }
    if (result > 0xFFFFU) {
        flags = static_cast<std::uint8_t>(flags | flag_carry);
    }
    af_.bytes.lo = flags;
}

void Core::op_and_n() {
    af_.bytes.hi = static_cast<std::uint8_t>(af_.bytes.hi & fetch_byte());
    af_.bytes.lo = flag_half;
    if ((af_.bytes.hi & 0x80U) != 0U) {
        af_.bytes.lo = static_cast<std::uint8_t>(af_.bytes.lo | flag_sign);
    }
    if (af_.bytes.hi == 0U) {
        af_.bytes.lo = static_cast<std::uint8_t>(af_.bytes.lo | flag_zero);
    }
    if (has_even_parity(af_.bytes.hi)) {
        af_.bytes.lo = static_cast<std::uint8_t>(af_.bytes.lo | flag_parity_overflow);
    }
}

void Core::op_or_n() {
    af_.bytes.hi = static_cast<std::uint8_t>(af_.bytes.hi | fetch_byte());
    af_.bytes.lo = 0;
    if ((af_.bytes.hi & 0x80U) != 0U) {
        af_.bytes.lo = static_cast<std::uint8_t>(af_.bytes.lo | flag_sign);
    }
    if (af_.bytes.hi == 0U) {
        af_.bytes.lo = static_cast<std::uint8_t>(af_.bytes.lo | flag_zero);
    }
    if (has_even_parity(af_.bytes.hi)) {
        af_.bytes.lo = static_cast<std::uint8_t>(af_.bytes.lo | flag_parity_overflow);
    }
}

void Core::op_or_e() {
    af_.bytes.hi = static_cast<std::uint8_t>(af_.bytes.hi | de_.bytes.lo);
    af_.bytes.lo = 0;
    if ((af_.bytes.hi & 0x80U) != 0U) {
        af_.bytes.lo = static_cast<std::uint8_t>(af_.bytes.lo | flag_sign);
    }
    if (af_.bytes.hi == 0U) {
        af_.bytes.lo = static_cast<std::uint8_t>(af_.bytes.lo | flag_zero);
    }
    if (has_even_parity(af_.bytes.hi)) {
        af_.bytes.lo = static_cast<std::uint8_t>(af_.bytes.lo | flag_parity_overflow);
    }
}

void Core::apply_or_flags() {
    af_.bytes.lo = 0;
    if ((af_.bytes.hi & 0x80U) != 0U) {
        af_.bytes.lo = static_cast<std::uint8_t>(af_.bytes.lo | flag_sign);
    }
    if (af_.bytes.hi == 0U) {
        af_.bytes.lo = static_cast<std::uint8_t>(af_.bytes.lo | flag_zero);
    }
    if (has_even_parity(af_.bytes.hi)) {
        af_.bytes.lo = static_cast<std::uint8_t>(af_.bytes.lo | flag_parity_overflow);
    }
}

void Core::op_or_b() { af_.bytes.hi = static_cast<std::uint8_t>(af_.bytes.hi | bc_.bytes.hi); apply_or_flags(); }
void Core::op_or_c() { af_.bytes.hi = static_cast<std::uint8_t>(af_.bytes.hi | bc_.bytes.lo); apply_or_flags(); }
void Core::op_or_d() { af_.bytes.hi = static_cast<std::uint8_t>(af_.bytes.hi | de_.bytes.hi); apply_or_flags(); }
void Core::op_or_h() { af_.bytes.hi = static_cast<std::uint8_t>(af_.bytes.hi | hl_.bytes.hi); apply_or_flags(); }
void Core::op_or_l() { af_.bytes.hi = static_cast<std::uint8_t>(af_.bytes.hi | hl_.bytes.lo); apply_or_flags(); }
void Core::op_or_mem_hl() { af_.bytes.hi = static_cast<std::uint8_t>(af_.bytes.hi | read_logical(hl_.value)); apply_or_flags(); }
void Core::op_or_a() { apply_or_flags(); }

void Core::apply_cp_flags(const std::uint8_t a_value, const std::uint8_t operand) {
    const auto result = static_cast<std::uint8_t>(a_value - operand);
    apply_sub_flags(a_value, operand, result);
}

void Core::op_cp_n() {
    const auto operand = fetch_byte();
    apply_cp_flags(af_.bytes.hi, operand);
}

void Core::op_cp_r_main() {
    const auto reg_code = static_cast<std::uint8_t>(last_opcode_ & 0x07U);
    const auto operand = (reg_code == 0x06U) ? read_logical(hl_.value) : register8_from_code(reg_code);
    apply_cp_flags(af_.bytes.hi, operand);
}

void Core::op_dec_de() { de_.value = static_cast<std::uint16_t>(de_.value - 1U); }

void Core::op_ret_z() {
    if ((af_.bytes.lo & flag_zero) != 0U) {
        pc_.value = pop_word();
    }
}

void Core::op_ret_nz() {
    if ((af_.bytes.lo & flag_zero) == 0U) {
        pc_.value = pop_word();
    }
}

void Core::op_ret_nc() {
    if ((af_.bytes.lo & flag_carry) == 0U) {
        pc_.value = pop_word();
    }
}

void Core::op_ret_c() {
    if ((af_.bytes.lo & flag_carry) != 0U) {
        pc_.value = pop_word();
    }
}

void Core::op_jr_e() {
    const auto displacement = static_cast<std::int8_t>(fetch_byte());
    pc_.value = static_cast<std::uint16_t>(pc_.value + displacement);
}

void Core::op_halt() {
    pc_.value = static_cast<std::uint16_t>(pc_.value - 1U);
    halted_ = true;
}

void Core::op_jp_nn() { pc_.value = fetch_word(); }

void Core::op_jp_z_nn() {
    const auto target = fetch_word();
    if ((af_.bytes.lo & flag_zero) != 0U) {
        pc_.value = target;
    }
}

void Core::op_jp_nz_nn() {
    const auto target = fetch_word();
    if ((af_.bytes.lo & flag_zero) == 0U) {
        pc_.value = target;
    }
}

void Core::op_ret() { pc_.value = pop_word(); }

void Core::op_call_nn() {
    const auto target = fetch_word();
    push_word(pc_.value);
    pc_.value = target;
}

void Core::op_in_a_n() { af_.bytes.hi = callbacks_.read_port(fetch_byte()); }

void Core::op_pop_af() { af_.value = pop_word(); }

void Core::op_pop_bc() { bc_.value = pop_word(); }

void Core::op_pop_de() { de_.value = pop_word(); }

void Core::op_pop_hl() { hl_.value = pop_word(); }

void Core::op_xor_a() {
    af_.bytes.hi = 0x00;
    af_.bytes.lo = flag_zero | flag_parity_overflow;
}

void Core::op_out_n_a() { callbacks_.write_port(fetch_byte(), af_.bytes.hi); }

void Core::op_di() {
    iff1_ = false;
    iff2_ = false;
    ei_delay_ = 0;
}

void Core::op_push_af() { push_word(af_.value); }

void Core::op_push_bc() { push_word(bc_.value); }

void Core::op_push_de() { push_word(de_.value); }

void Core::op_push_hl() { push_word(hl_.value); }

void Core::op_ex_de_hl() { std::swap(de_.value, hl_.value); }

void Core::op_cpl() {
    af_.bytes.hi = static_cast<std::uint8_t>(~af_.bytes.hi);
    af_.bytes.lo = static_cast<std::uint8_t>(af_.bytes.lo | flag_half | flag_subtract);
}

void Core::op_scf() {
    constexpr auto preserved = static_cast<std::uint8_t>(flag_sign | flag_zero | flag_parity_overflow);
    af_.bytes.lo = static_cast<std::uint8_t>((af_.bytes.lo & preserved) | flag_carry);
}

void Core::op_ei() { ei_delay_ = 2; }

void Core::op_cb_prefix() {
    const auto opcode = fetch_byte();
    switch (opcode) {
    case 0x1D:
        op_cb_rr_l();
        return;
    case 0x3C:
        op_cb_srl_h();
        return;
    case 0x3F:
        op_cb_srl_a();
        return;
    case 0x67:
    case 0x6F:
    case 0x77:
    case 0x7F:
        op_cb_bit_a(static_cast<std::uint8_t>((opcode >> 3U) & 0x07U));
        return;
    default:
        {
            std::ostringstream stream;
            stream << "Unsupported timed Z180 opcode 0xCB 0x" << std::uppercase << std::hex
                   << static_cast<int>(opcode) << " at PC 0x"
                   << static_cast<std::uint16_t>(pc_.value - 2U);
            throw std::runtime_error(stream.str());
        }
    }
}

void Core::op_cb_srl_a() {
    const auto old_a = af_.bytes.hi;
    const auto result = static_cast<std::uint8_t>(old_a >> 1U);
    af_.bytes.hi = result;

    auto flags = static_cast<std::uint8_t>(old_a & flag_carry);
    if (result == 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_zero);
    }
    if (has_even_parity(result)) {
        flags = static_cast<std::uint8_t>(flags | flag_parity_overflow);
    }
    af_.bytes.lo = flags;
}

void Core::op_cb_srl_h() {
    const auto old_h = hl_.bytes.hi;
    const auto result = static_cast<std::uint8_t>(old_h >> 1U);
    hl_.bytes.hi = result;

    auto flags = static_cast<std::uint8_t>(old_h & flag_carry);
    if (result == 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_zero);
    }
    if (has_even_parity(result)) {
        flags = static_cast<std::uint8_t>(flags | flag_parity_overflow);
    }
    af_.bytes.lo = flags;
}

void Core::op_cb_rr_l() {
    const auto old_l = hl_.bytes.lo;
    const auto old_carry = static_cast<std::uint8_t>(af_.bytes.lo & flag_carry);
    const auto result =
        static_cast<std::uint8_t>((old_l >> 1U) | (old_carry != 0U ? 0x80U : 0x00U));
    hl_.bytes.lo = result;

    auto flags = static_cast<std::uint8_t>(old_l & flag_carry);
    if ((result & 0x80U) != 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_sign);
    }
    if (result == 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_zero);
    }
    if (has_even_parity(result)) {
        flags = static_cast<std::uint8_t>(flags | flag_parity_overflow);
    }
    af_.bytes.lo = flags;
}

void Core::op_cb_bit_a(const std::uint8_t bit) {
    const auto mask = static_cast<std::uint8_t>(1U << bit);
    const auto bit_set = (af_.bytes.hi & mask) != 0U;
    auto flags = static_cast<std::uint8_t>((af_.bytes.lo & flag_carry) | flag_half);
    if (!bit_set) {
        flags = static_cast<std::uint8_t>(flags | flag_zero | flag_parity_overflow);
    }
    if (bit == 7U && bit_set) {
        flags = static_cast<std::uint8_t>(flags | flag_sign);
    }
    af_.bytes.lo = flags;
}

void Core::op_ed_prefix() {
    const auto opcode = fetch_byte();
    if ((opcode & 0xC7U) == 0x00U && ((opcode >> 3U) & 0x07U) != 0x06U) {
        op_ed_in0_r_n(static_cast<std::uint8_t>((opcode >> 3U) & 0x07U));
        return;
    }
    if ((opcode & 0xC7U) == 0x01U && ((opcode >> 3U) & 0x07U) != 0x06U) {
        op_ed_out0_n_r(static_cast<std::uint8_t>((opcode >> 3U) & 0x07U));
        return;
    }
    if ((opcode & 0xC7U) == 0x04U && ((opcode >> 3U) & 0x07U) != 0x06U) {
        op_ed_tst_r(static_cast<std::uint8_t>((opcode >> 3U) & 0x07U));
        return;
    }
    if ((opcode & 0xCFU) == 0x4CU) {
        op_ed_mlt_rr(static_cast<std::uint8_t>((opcode >> 4U) & 0x03U));
        return;
    }
    if ((opcode & 0xCFU) == 0x42U) {
        op_ed_sbc_hl_ss(static_cast<std::uint8_t>((opcode >> 4U) & 0x03U));
        return;
    }
    if (opcode == 0x64U) {
        op_ed_tst_n();
        return;
    }
    const auto handler = ed_opcodes_[opcode];
    if (handler == &Core::op_unimplemented) {
        unsupported_ed_opcode(opcode);
    }
    (this->*handler)();
}

void Core::op_ed_in0_r_n(const std::uint8_t reg_code) { register8_from_code(reg_code) = in0(fetch_byte()); }

void Core::op_ed_out0_n_r(const std::uint8_t reg_code) { out0(fetch_byte(), register8_from_code(reg_code)); }

void Core::op_ed_tst_r(const std::uint8_t reg_code) {
    const auto value = static_cast<std::uint8_t>(af_.bytes.hi & register8_from_code(reg_code));
    update_tst_flags(value);
}

void Core::op_ed_tst_n() {
    const auto value = static_cast<std::uint8_t>(af_.bytes.hi & fetch_byte());
    update_tst_flags(value);
}

void Core::op_ed_mlt_rr(const std::uint8_t pair_code) {
    auto& reg = register_pair_from_code(pair_code);
    reg.value = static_cast<std::uint16_t>(reg.bytes.hi * reg.bytes.lo);
}

void Core::op_ed_sbc_hl_ss(const std::uint8_t pair_code) {
    const auto operand = register_pair_from_code(pair_code).value;
    const auto old_hl = hl_.value;
    const auto carry = (af_.bytes.lo & flag_carry) != 0U ? 1U : 0U;
    const auto subtrahend = static_cast<std::uint32_t>(operand) + carry;
    const auto result = static_cast<std::uint32_t>(old_hl) - subtrahend;
    const auto result16 = static_cast<std::uint16_t>(result & 0xFFFFU);

    hl_.value = result16;

    std::uint8_t flags = flag_subtract;
    if ((result16 & 0x8000U) != 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_sign);
    }
    if (result16 == 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_zero);
    }
    if (((old_hl ^ operand ^ result16) & 0x1000U) != 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_half);
    }
    if (((old_hl ^ operand) & (old_hl ^ result16) & 0x8000U) != 0U) {
        flags = static_cast<std::uint8_t>(flags | flag_parity_overflow);
    }
    if (static_cast<std::uint32_t>(old_hl) < subtrahend) {
        flags = static_cast<std::uint8_t>(flags | flag_carry);
    }
    af_.bytes.lo = flags;
}

void Core::op_ed_out0_n_a() { out0(fetch_byte(), af_.bytes.hi); }

void Core::op_ed_im_1() { interrupt_mode_ = 0x01; }

void Core::op_ed_ld_i_a() { i_ = af_.bytes.hi; }

void Core::op_ed_reti() {
    pc_.value = pop_word();
    acknowledge_reti();
}

}  // namespace vanguard8::third_party::z180

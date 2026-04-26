// license:BSD-3-Clause
// copyright-holders:Juergen Buchmueller
//
// Vanguard 8 Z180 CPU core — wraps the MAME HD64180/Z180 execution engine.
// Imported from mamedev/mame src/devices/cpu/z180/ at commit
// c331217dffc1f8efde2e5f0e162049e39dd8717d.
//
// Full Z80 + HD64180 instruction set, on-chip MMU, PRT timers, DMA registers,
// and interrupt response.  Opcode dispatch and cycle timing are supplied by the
// imported core; no per-opcode tables exist outside this translation unit.
#pragma once

#include "third_party/z180/z180_compat.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace vanguard8::third_party::z180 {

class Core {
  public:
    explicit Core(Callbacks callbacks);

    void reset();

    // Register accessors
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
    [[nodiscard]] auto register_snapshot() const -> RegisterSnapshot;
    void load_register_snapshot(const RegisterSnapshot& state);

    void set_register_i(std::uint8_t value);
    void set_interrupt_mode(std::uint8_t mode);
    void set_iff1(bool enabled);
    void set_iff2(bool enabled);
    void advance_tstates(std::uint64_t tstates);

    // Internal I/O (ports not intercepted by the adapter)
    [[nodiscard]] auto in0(std::uint8_t port) -> std::uint8_t;
    void out0(std::uint8_t port, std::uint8_t value);

    // Memory translation and access
    [[nodiscard]] auto translate_logical_address(std::uint16_t logical_address) const
        -> std::uint32_t;
    [[nodiscard]] auto read_logical(std::uint16_t logical_address) -> std::uint8_t;
    void write_logical(std::uint16_t logical_address, std::uint8_t value);

    // Interrupt service
    [[nodiscard]] auto service_pending_interrupt(bool int0_asserted, bool int1_asserted)
        -> std::optional<InterruptService>;

    // Execution
    void step_one();  // Execute one instruction
    void run_until_halt(std::size_t max_instructions);
    [[nodiscard]] auto next_instruction_tstates() const -> std::uint64_t;
    [[nodiscard]] auto peek_next_opcode() const -> std::uint8_t;

  private:
    // ---- MAME-compatible state ----
    // These macros expand to member accesses; the names must match exactly what
    // the imported .hxx files expect via the OP() / TABLE() macros.
    // clang-format off

    // Register file
    PAIR  m_PREPC{}, m_PC{}, m_SP{}, m_AF{}, m_BC{}, m_DE{}, m_HL{}, m_IX{}, m_IY{};
    PAIR  m_AF2{}, m_BC2{}, m_DE2{}, m_HL2{};
    u8    m_R = 0, m_R2 = 0, m_IFF1 = 0, m_IFF2 = 0, m_HALT = 0, m_IM = 0, m_I = 0;
    u8    m_tmdr_latch = 0;
    u8    m_read_tcr_tmdr[2] = {0, 0};
    u32   m_iol = 0;
    PAIR16 m_tmdr[2] = {};
    PAIR16 m_rldr[2] = {};
    u8    m_tcr = 0;
    u8    m_frc = 0;
    u8    m_frc_prescale = 0;
    PAIR  m_dma_sar0{}, m_dma_dar0{}, m_dma_mar1{}, m_dma_iar1{};
    PAIR16 m_dma_bcr[2] = {};
    u8    m_dstat = 0;
    u8    m_dmode = 0;
    u8    m_dcntl = 0;
    u8    m_mmu_cbr = 0;
    u8    m_mmu_bbr = 0;
    u8    m_mmu_cbar = 0;
    u8    m_omcr = 0;
    u8    m_iocr = 0;
    u8    m_rcr = 0;
    u8    m_il = 0;
    u8    m_itc = 0;
    offs_t m_mmu[16] = {};
    u8    m_tmdrh[2] = {0, 0};
    u16   m_tmdr_value[2] = {0xFFFF, 0xFFFF};
    u8    m_nmi_state = 0, m_nmi_pending = 0;
    u8    m_irq_state[3] = {0, 0, 0};
    u8    m_int_pending[12] = {};
    u8    m_after_EI = 0;
    u32   m_ea = 0;

    // Extra cycles accumulated during instruction execution (wait states, I/O etc.)
    int    m_extra_cycles = 0;

    // Cycle-count table pointers (set up in ctor)
    u8*    m_cc[6] = {};

    Callbacks callbacks_;
    u8 last_opcode_ = 0;

    // Opcode function pointer type and table (matching MAME's s_z180ops)
    using opcode_func = void (Core::*)();
    static const opcode_func s_ops[6][0x100];

    // ---- MAME-derived helpers ----
    void z180_mmu();
    [[nodiscard]] auto mmu_remap(offs_t addr) const -> offs_t;

    u8  RM(offs_t addr);
    void WM(offs_t addr, u8 value);
    void RM16(offs_t addr, PAIR* r);
    void WM16(offs_t addr, PAIR* r);
    u8  ROP();
    u8  ARG();
    u32 ARG16();
    u8  IN(u16 port);
    void OUT(u16 port, u8 value);

    u8  INC(u8 value);
    u8  DEC(u8 value);
    u8  RLC(u8 value);
    u8  RRC(u8 value);
    u8  RL(u8 value);
    u8  RR(u8 value);
    u8  SLA(u8 value);
    u8  SRA(u8 value);
    u8  SLL(u8 value);
    u8  SRL(u8 value);
    u8  RES(u8 bit, u8 value);
    u8  SET(u8 bit, u8 value);

    u8  z180_readcontrol(offs_t port);
    void z180_writecontrol(offs_t port, u8 data);
    u8  z180_internal_port_read(u8 port);
    void z180_internal_port_write(u8 port, u8 data);
    void clock_timers();
    int  check_interrupts();
    int  take_interrupt(int source);
    void handle_io_timers(int cycles);

    [[nodiscard]] auto is_internal_io_address(u16 port) const -> bool;
    [[nodiscard]] auto memory_wait_states() const -> int;
    [[nodiscard]] auto io_wait_states() const -> int;

    void push_word(u16 value);
    [[nodiscard]] auto pop_word() -> u16;

    static void initialize_tables();

    // ---- Execution dispatchers (called by opcode dispatch table) ----
    int exec_op(u8 opcode);
    int exec_cb(u8 opcode);
    int exec_dd(u8 opcode);
    int exec_ed(u8 opcode);
    int exec_fd(u8 opcode);
    int exec_xycb(u8 opcode);

    // ---- Opcode method declarations (768 total, generated by .hxx files) ----
    // The TABLE() macro in z180_core.cpp maps opcodes to these methods.
    // They are defined by the #included MAME .hxx files via OP() macro.
    #define DECLARE_PREFIX_OPS(p) \
        void p##_00(); void p##_01(); void p##_02(); void p##_03(); \
        void p##_04(); void p##_05(); void p##_06(); void p##_07(); \
        void p##_08(); void p##_09(); void p##_0a(); void p##_0b(); \
        void p##_0c(); void p##_0d(); void p##_0e(); void p##_0f(); \
        void p##_10(); void p##_11(); void p##_12(); void p##_13(); \
        void p##_14(); void p##_15(); void p##_16(); void p##_17(); \
        void p##_18(); void p##_19(); void p##_1a(); void p##_1b(); \
        void p##_1c(); void p##_1d(); void p##_1e(); void p##_1f(); \
        void p##_20(); void p##_21(); void p##_22(); void p##_23(); \
        void p##_24(); void p##_25(); void p##_26(); void p##_27(); \
        void p##_28(); void p##_29(); void p##_2a(); void p##_2b(); \
        void p##_2c(); void p##_2d(); void p##_2e(); void p##_2f(); \
        void p##_30(); void p##_31(); void p##_32(); void p##_33(); \
        void p##_34(); void p##_35(); void p##_36(); void p##_37(); \
        void p##_38(); void p##_39(); void p##_3a(); void p##_3b(); \
        void p##_3c(); void p##_3d(); void p##_3e(); void p##_3f(); \
        void p##_40(); void p##_41(); void p##_42(); void p##_43(); \
        void p##_44(); void p##_45(); void p##_46(); void p##_47(); \
        void p##_48(); void p##_49(); void p##_4a(); void p##_4b(); \
        void p##_4c(); void p##_4d(); void p##_4e(); void p##_4f(); \
        void p##_50(); void p##_51(); void p##_52(); void p##_53(); \
        void p##_54(); void p##_55(); void p##_56(); void p##_57(); \
        void p##_58(); void p##_59(); void p##_5a(); void p##_5b(); \
        void p##_5c(); void p##_5d(); void p##_5e(); void p##_5f(); \
        void p##_60(); void p##_61(); void p##_62(); void p##_63(); \
        void p##_64(); void p##_65(); void p##_66(); void p##_67(); \
        void p##_68(); void p##_69(); void p##_6a(); void p##_6b(); \
        void p##_6c(); void p##_6d(); void p##_6e(); void p##_6f(); \
        void p##_70(); void p##_71(); void p##_72(); void p##_73(); \
        void p##_74(); void p##_75(); void p##_76(); void p##_77(); \
        void p##_78(); void p##_79(); void p##_7a(); void p##_7b(); \
        void p##_7c(); void p##_7d(); void p##_7e(); void p##_7f(); \
        void p##_80(); void p##_81(); void p##_82(); void p##_83(); \
        void p##_84(); void p##_85(); void p##_86(); void p##_87(); \
        void p##_88(); void p##_89(); void p##_8a(); void p##_8b(); \
        void p##_8c(); void p##_8d(); void p##_8e(); void p##_8f(); \
        void p##_90(); void p##_91(); void p##_92(); void p##_93(); \
        void p##_94(); void p##_95(); void p##_96(); void p##_97(); \
        void p##_98(); void p##_99(); void p##_9a(); void p##_9b(); \
        void p##_9c(); void p##_9d(); void p##_9e(); void p##_9f(); \
        void p##_a0(); void p##_a1(); void p##_a2(); void p##_a3(); \
        void p##_a4(); void p##_a5(); void p##_a6(); void p##_a7(); \
        void p##_a8(); void p##_a9(); void p##_aa(); void p##_ab(); \
        void p##_ac(); void p##_ad(); void p##_ae(); void p##_af(); \
        void p##_b0(); void p##_b1(); void p##_b2(); void p##_b3(); \
        void p##_b4(); void p##_b5(); void p##_b6(); void p##_b7(); \
        void p##_b8(); void p##_b9(); void p##_ba(); void p##_bb(); \
        void p##_bc(); void p##_bd(); void p##_be(); void p##_bf(); \
        void p##_c0(); void p##_c1(); void p##_c2(); void p##_c3(); \
        void p##_c4(); void p##_c5(); void p##_c6(); void p##_c7(); \
        void p##_c8(); void p##_c9(); void p##_ca(); void p##_cb(); \
        void p##_cc(); void p##_cd(); void p##_ce(); void p##_cf(); \
        void p##_d0(); void p##_d1(); void p##_d2(); void p##_d3(); \
        void p##_d4(); void p##_d5(); void p##_d6(); void p##_d7(); \
        void p##_d8(); void p##_d9(); void p##_da(); void p##_db(); \
        void p##_dc(); void p##_dd(); void p##_de(); void p##_df(); \
        void p##_e0(); void p##_e1(); void p##_e2(); void p##_e3(); \
        void p##_e4(); void p##_e5(); void p##_e6(); void p##_e7(); \
        void p##_e8(); void p##_e9(); void p##_ea(); void p##_eb(); \
        void p##_ec(); void p##_ed(); void p##_ee(); void p##_ef(); \
        void p##_f0(); void p##_f1(); void p##_f2(); void p##_f3(); \
        void p##_f4(); void p##_f5(); void p##_f6(); void p##_f7(); \
        void p##_f8(); void p##_f9(); void p##_fa(); void p##_fb(); \
        void p##_fc(); void p##_fd(); void p##_fe(); void p##_ff();

    DECLARE_PREFIX_OPS(op)
    DECLARE_PREFIX_OPS(cb)
    DECLARE_PREFIX_OPS(dd)
    DECLARE_PREFIX_OPS(ed)
    DECLARE_PREFIX_OPS(fd)
    DECLARE_PREFIX_OPS(xycb)
    #undef DECLARE_PREFIX_OPS

    void illegal_1();
    void illegal_2();

    // ---- Vanguard 8 spec helpers (kept from prior core) ----
    [[nodiscard]] auto vectored_handler_address(std::uint8_t fixed_code) -> std::uint16_t;
    void service_vectored_interrupt(InterruptSource source, std::uint8_t fixed_code);
    void wake_from_halt_for_interrupt();
    void maybe_warn_illegal_bbr();
};

}  // namespace vanguard8::third_party::z180

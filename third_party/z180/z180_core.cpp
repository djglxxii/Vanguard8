// license:BSD-3-Clause
// copyright-holders:Juergen Buchmueller
//
// Vanguard 8 Z180 CPU core — wraps the MAME HD64180/Z180 execution engine.
// Imported from mamedev/mame src/devices/cpu/z180/ at commit
// c331217dffc1f8efde2e5f0e162049e39dd8717d.
//
// Full Z80 + HD64180 instruction set, on-chip MMU, PRT timers, and interrupt
// response.  Opcode dispatch and cycle timing are supplied by the imported
// core; no per-opcode tables exist outside this translation unit.

#include "third_party/z180/z180_core.hpp"

#include <algorithm>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <utility>

namespace vanguard8::third_party::z180 {

// =========================================================================
//  Flag lookup tables — pre-computed once, shared by all Core instances.
//  Identical to MAME's z180_device::device_start() table initialization.
// =========================================================================

namespace {

std::unique_ptr<u8[]> g_SZHVC_add;
std::unique_ptr<u8[]> g_SZHVC_sub;

bool g_tables_initialized = false;

void init_flag_tables() {
    if (g_tables_initialized) return;
    g_tables_initialized = true;

    g_SZHVC_add = std::make_unique<u8[]>(2 * 256 * 256);
    g_SZHVC_sub = std::make_unique<u8[]>(2 * 256 * 256);

    u8* padd = &g_SZHVC_add[0 * 256];
    u8* padc = &g_SZHVC_add[256 * 256];
    u8* psub = &g_SZHVC_sub[0 * 256];
    u8* psbc = &g_SZHVC_sub[256 * 256];

    for (int oldval = 0; oldval < 256; oldval++) {
        for (int newval = 0; newval < 256; newval++) {
            // add or adc w/o carry set
            int val = newval - oldval;
            *padd = (newval) ? ((newval & 0x80) ? SF : 0) : ZF;
            *padd |= (newval & (YF | XF));
            if ((newval & 0x0f) < (oldval & 0x0f)) *padd |= HF;
            if (newval < oldval) *padd |= CF;
            if ((val ^ oldval ^ 0x80) & (val ^ newval) & 0x80) *padd |= VF;
            padd++;

            // adc with carry set
            val = newval - oldval - 1;
            *padc = (newval) ? ((newval & 0x80) ? SF : 0) : ZF;
            *padc |= (newval & (YF | XF));
            if ((newval & 0x0f) <= (oldval & 0x0f)) *padc |= HF;
            if (newval <= oldval) *padc |= CF;
            if ((val ^ oldval ^ 0x80) & (val ^ newval) & 0x80) *padc |= VF;
            padc++;

            // cp, sub or sbc w/o carry set
            val = oldval - newval;
            *psub = NF | ((newval) ? ((newval & 0x80) ? SF : 0) : ZF);
            *psub |= (newval & (YF | XF));
            if ((newval & 0x0f) > (oldval & 0x0f)) *psub |= HF;
            if (newval > oldval) *psub |= CF;
            if ((val ^ oldval) & (oldval ^ newval) & 0x80) *psub |= VF;
            psub++;

            // sbc with carry set
            val = oldval - newval - 1;
            *psbc = NF | ((newval) ? ((newval & 0x80) ? SF : 0) : ZF);
            *psbc |= (newval & (YF | XF));
            if ((newval & 0x0f) >= (oldval & 0x0f)) *psbc |= HF;
            if (newval >= oldval) *psbc |= CF;
            if ((val ^ oldval) & (oldval ^ newval) & 0x80) *psbc |= VF;
            psbc++;
        }
    }

    for (int i = 0; i < 256; i++) {
        int p = 0;
        if (i & 0x01) ++p;
        if (i & 0x02) ++p;
        if (i & 0x04) ++p;
        if (i & 0x08) ++p;
        if (i & 0x10) ++p;
        if (i & 0x20) ++p;
        if (i & 0x40) ++p;
        if (i & 0x80) ++p;
        SZ[i] = i ? i & SF : ZF;
        SZ[i] |= (i & (YF | XF));
        SZ_BIT[i] = i ? i & SF : ZF | PF;
        SZ_BIT[i] |= (i & (YF | XF));
        SZP[i] = SZ[i] | ((p & 1) ? 0 : PF);
        SZHV_inc[i] = SZ[i];
        if (i == 0x80) SZHV_inc[i] |= VF;
        if ((i & 0x0f) == 0x00) SZHV_inc[i] |= HF;
        SZHV_dec[i] = SZ[i] | NF;
        if (i == 0x7f) SZHV_dec[i] |= VF;
        if ((i & 0x0f) == 0x0f) SZHV_dec[i] |= HF;
    }

    // Point the extern-referenced arrays at our allocated storage
    SZHVC_add = g_SZHVC_add.get();
    SZHVC_sub = g_SZHVC_sub.get();
}

}  // anonymous namespace

// Extern definitions for the tables declared in z180_compat.hpp
u8 SZ[256];
u8 SZ_BIT[256];
u8 SZP[256];
u8 SZHV_inc[256];
u8 SZHV_dec[256];
u8* SZHVC_add = nullptr;
u8* SZHVC_sub = nullptr;

// Cycle-count tables from MAME z180tbl.h
const u8 cc_op[0x100] = {
     3, 9, 7, 4, 4, 4, 6, 3, 4, 7, 6, 4, 4, 4, 6, 3,
     7, 9, 7, 4, 4, 4, 6, 3, 8, 7, 6, 4, 4, 4, 6, 3,
     6, 9,16, 4, 4, 4, 6, 4, 6, 7,15, 4, 4, 4, 6, 3,
     6, 9,13, 4,10,10, 9, 3, 6, 7,12, 4, 4, 4, 6, 3,
     4, 4, 4, 4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 6, 4,
     4, 4, 4, 4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 6, 4,
     4, 4, 4, 4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 6, 4,
     7, 7, 7, 7, 7, 7, 3, 7, 4, 4, 4, 4, 4, 4, 6, 4,
     4, 4, 4, 4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 6, 4,
     4, 4, 4, 4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 6, 4,
     4, 4, 4, 4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 6, 4,
     4, 4, 4, 4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 6, 4,
     5, 9, 6, 9, 6,11, 6,11, 5, 9, 6, 0, 6,16, 6,11,
     5, 9, 6,10, 6,11, 6,11, 5, 3, 6, 9, 6, 0, 6,11,
     5, 9, 6,16, 6,11, 6,11, 5, 3, 6, 3, 6, 0, 6,11,
     5, 9, 6, 3, 6,11, 6,11, 5, 4, 6, 3, 6, 0, 6,11
};

const u8 cc_cb[0x100] = {
     7, 7, 7, 7, 7, 7,13, 7, 7, 7, 7, 7, 7, 7,13, 7,
     7, 7, 7, 7, 7, 7,13, 7, 7, 7, 7, 7, 7, 7,13, 7,
     7, 7, 7, 7, 7, 7,13, 7, 7, 7, 7, 7, 7, 7,13, 7,
     7, 7, 7, 7, 7, 7,13, 7, 7, 7, 7, 7, 7, 7,13, 7,
     6, 6, 6, 6, 6, 6, 9, 6, 6, 6, 6, 6, 6, 6, 9, 6,
     6, 6, 6, 6, 6, 6, 9, 6, 6, 6, 6, 6, 6, 6, 9, 6,
     6, 6, 6, 6, 6, 6, 9, 6, 6, 6, 6, 6, 6, 6, 9, 6,
     6, 6, 6, 6, 6, 6, 9, 6, 6, 6, 6, 6, 6, 6, 9, 6,
     7, 7, 7, 7, 7, 7,13, 7, 7, 7, 7, 7, 7, 7,13, 7,
     7, 7, 7, 7, 7, 7,13, 7, 7, 7, 7, 7, 7, 7,13, 7,
     7, 7, 7, 7, 7, 7,13, 7, 7, 7, 7, 7, 7, 7,13, 7,
     7, 7, 7, 7, 7, 7,13, 7, 7, 7, 7, 7, 7, 7,13, 7,
     7, 7, 7, 7, 7, 7,13, 7, 7, 7, 7, 7, 7, 7,13, 7,
     7, 7, 7, 7, 7, 7,13, 7, 7, 7, 7, 7, 7, 7,13, 7,
     7, 7, 7, 7, 7, 7,13, 7, 7, 7, 7, 7, 7, 7,13, 7,
     7, 7, 7, 7, 7, 7,13, 7, 7, 7, 7, 7, 7, 7,13, 7
};

const u8 cc_ed[0x100] = {
    12,13, 6, 6, 9, 6, 6, 6,12,13, 6, 6, 9, 6, 6, 6,
    12,13, 6, 6, 9, 6, 6, 6,12,13, 6, 6, 9, 6, 6, 6,
    12,13, 6, 6, 9, 6, 6, 6,12,13, 6, 6,10, 6, 6, 6,
    12,13, 6, 6, 9, 6, 6, 6,12,13, 6, 6, 9, 6, 6, 6,
     9,10,10,19, 6,12, 6, 6, 9,10,10,18,17,12, 6, 6,
     9,10,10,19, 6,12, 6, 6, 9,10,10,18,17,12, 6, 6,
     9,10,10,19, 6,12, 6,16, 9,10,10,18,17,12, 6,16,
     9,10,10,19,12,12, 8, 6, 9,10,10,18,17,12, 6, 6,
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    12,12,12,12, 6, 6, 6, 6,12,12,12,12, 6, 6, 6, 6,
    12,12,12,12, 6, 6, 6, 6,12,12,12,12, 6, 6, 6, 6,
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6
};

const u8 cc_xy[0x100] = {
     4, 4, 4, 4, 4, 4, 4, 4, 4,10, 4, 4, 4, 4, 4, 4,
     4, 4, 4, 4, 4, 4, 4, 4, 4,10, 4, 4, 4, 4, 4, 4,
     4,12,19, 7, 9, 9,15, 4, 4,10,18, 7, 9, 9, 9, 4,
     4, 4, 4, 4,18,18,15, 4, 4,10, 4, 4, 4, 4, 4, 4,
     4, 4, 4, 4, 9, 9,14, 4, 4, 4, 4, 4, 9, 9,14, 4,
     4, 4, 4, 4, 9, 9,14, 4, 4, 4, 4, 4, 9, 9,14, 4,
     9, 9, 9, 9, 9, 9,14, 9, 9, 9, 9, 9, 9, 9,14, 9,
    15,15,15,15,15,15, 4,15, 4, 4, 4, 4, 9, 9,14, 4,
     4, 4, 4, 4, 9, 9,14, 4, 4, 4, 4, 4, 9, 9,14, 4,
     4, 4, 4, 4, 9, 9,14, 4, 4, 4, 4, 4, 9, 9,14, 4,
     4, 4, 4, 4, 9, 9,14, 4, 4, 4, 4, 4, 9, 9,14, 4,
     4, 4, 4, 4, 9, 9,14, 4, 4, 4, 4, 4, 9, 9,14, 4,
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 4, 4, 4, 4,
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     4,12, 4,19, 4,14, 4, 4, 4, 6, 4, 4, 4, 4, 4, 4,
     4, 4, 4, 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4
};

const u8 cc_xycb[0x100] = {
    19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
    19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
    19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
    19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
    15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
    19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
    19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
    19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
    19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
    19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
    19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
    19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
    19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19
};

const u8 cc_ex[0x100] = {
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0,
     2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,10, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,10, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,10, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,10, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     4, 4, 4, 4, 0, 0, 0, 0, 4, 4, 4, 4, 0, 0, 0, 0,
     5, 0, 3, 0,10, 0, 0, 2, 5, 0, 3, 0,10, 0, 0, 2,
     5, 0, 3, 0,10, 0, 0, 2, 5, 0, 3, 0,10, 0, 0, 2,
     5, 0, 3, 0,10, 0, 0, 2, 5, 0, 3, 0,10, 0, 0, 2,
     5, 0, 3, 0,10, 0, 0, 2, 5, 0, 3, 0,10, 0, 0, 2
};

namespace {

// irep_tmp1 / drep_tmp1 / breg_tmp2 tables used by INI/OUTI/IND/OUTD opcodes
const u8 irep_tmp1[4][4] = {
    {0,0,1,0},{0,1,0,1},{1,0,1,1},{0,1,1,0}
};
const u8 drep_tmp1[4][4] = {
    {0,1,0,0},{1,0,0,1},{0,0,1,0},{0,1,0,1}
};
const u8 breg_tmp2[256] = {
    0,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,
    0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,
    1,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,
    1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,
    0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,
    1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,
    0,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,
    0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,
    1,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,
    1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,
    0,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,
    0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,
    1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,
    0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,
    1,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,
    1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1
};

}  // anonymous namespace

// =========================================================================
//  MAME-compatible register access macros
//  These expand to member accesses on `this` inside Core member functions.
//  Must be defined BEFORE including the .hxx opcode files.
// =========================================================================

// Prevent conflicts with libc _B/_C/_L
#undef _B
#undef _C
#undef _L

#define _PPC    m_PREPC.d
#define _PCD    m_PC.d
#define _PC     m_PC.w.l

#define _SPD    m_SP.d
#define _SP     m_SP.w.l

#define _AFD    m_AF.d
#define _AF     m_AF.w.l
#define _A      m_AF.b.h
#define _F      m_AF.b.l

#define _BCD    m_BC.d
#define _BC     m_BC.w.l
#define _B      m_BC.b.h
#define _C      m_BC.b.l

#define _DED    m_DE.d
#define _DE     m_DE.w.l
#define _D      m_DE.b.h
#define _E      m_DE.b.l

#define _HLD    m_HL.d
#define _HL     m_HL.w.l
#define _H      m_HL.b.h
#define _L      m_HL.b.l

#define _IXD    m_IX.d
#define _IX     m_IX.w.l
#define _HX     m_IX.b.h
#define _LX     m_IX.b.l

#define _IYD    m_IY.d
#define _IY     m_IY.w.l
#define _HY     m_IY.b.h
#define _LY     m_IY.b.l

// ---- Memory / I/O primitive macros (adapted from MAME z180ops.h) ----

// Determine if a port address targets internal I/O (ports 0x00-0x3F when IOCR=0)
inline bool Core::is_internal_io_address(u16 port) const {
    // HD64180 (non-extended): internal when (port ^ m_iocr) & 0xffc0 == 0
    // With m_iocr reset to 0x00, internal range is 0x00-0x3f.
    return ((port ^ m_iocr) & 0xffc0) == 0;
}

inline int Core::memory_wait_states() const {
    return (m_dcntl & 0xc0) >> 6;
}

inline int Core::io_wait_states() const {
    return (m_dcntl & 0x30) == 0 ? 0 : ((m_dcntl & 0x30) >> 4) + 1;
}

u8 Core::RM(offs_t addr) {
    m_extra_cycles += memory_wait_states();
    u8 value = callbacks_.read_memory(mmu_remap(addr));
    if (callbacks_.observe_logical_memory_read)
        callbacks_.observe_logical_memory_read(addr, value);
    return value;
}

void Core::WM(offs_t addr, u8 value) {
    m_extra_cycles += memory_wait_states();
    callbacks_.write_memory(mmu_remap(addr), value);
    if (callbacks_.observe_logical_memory_write)
        callbacks_.observe_logical_memory_write(addr, value);
}

void Core::RM16(offs_t addr, PAIR* r) {
    r->b.l = RM(addr);
    r->b.h = RM(addr + 1);
}

void Core::WM16(offs_t addr, PAIR* r) {
    WM(addr, r->b.l);
    WM(addr + 1, r->b.h);
}

u8 Core::ROP() {
    offs_t addr = _PCD;
    _PC++;
    m_extra_cycles += memory_wait_states();
    return callbacks_.read_memory(mmu_remap(addr));
}

u8 Core::ARG() {
    offs_t addr = _PCD;
    _PC++;
    m_extra_cycles += memory_wait_states();
    return callbacks_.read_memory(mmu_remap(addr));
}

u32 Core::ARG16() {
    offs_t addr = _PCD;
    _PC += 2;
    m_extra_cycles += memory_wait_states() * 2;
    return callbacks_.read_memory(mmu_remap(addr)) |
           (callbacks_.read_memory(mmu_remap(addr + 1)) << 8);
}

u8 Core::IN(u16 port) {
    // Vanguard 8 external-bus precedence carve-out: when the host provides
    // an `external_port_override` hook that claims this port, the cycle is
    // serviced by the external bus glue regardless of the HD64180
    // internal-I/O comparator. See docs/spec/04-io.md "Coexistence with
    // HD64180 Internal I/O" and docs/spec/01-cpu.md "Internal I/O Address
    // Comparator".
    if (callbacks_.external_port_override && callbacks_.external_port_override(port)) {
        m_extra_cycles += io_wait_states();
        return callbacks_.read_port(port);
    }
    if (is_internal_io_address(port))
        return z180_readcontrol(port);
    m_extra_cycles += io_wait_states();
    return callbacks_.read_port(port);
}

void Core::OUT(u16 port, u8 value) {
    if (callbacks_.external_port_override && callbacks_.external_port_override(port)) {
        m_extra_cycles += io_wait_states();
        callbacks_.write_port(port, value);
        return;
    }
    if (is_internal_io_address(port))
        z180_writecontrol(port, value);
    else {
        m_extra_cycles += io_wait_states();
        callbacks_.write_port(port, value);
    }
}

// ---- Opcode helper macros from MAME z180ops.h ----

#define ENTER_HALT() { \
    _PC--;               \
    m_HALT = 1;          \
}

#define LEAVE_HALT() {                                          \
    if (m_HALT) {                                                \
        if (m_HALT == 2) { _PC += 2; }                            \
        else { _PC++; }                                            \
        m_HALT = 0;                                                \
    }                                                           \
}

// Effective address computation for IX+d / IY+d
#define EAX() m_ea = (u32)(u16)(_IX + (int8_t)ARG())
#define EAY() m_ea = (u32)(u16)(_IY + (int8_t)ARG())

#define MMU_REMAP_ADDR(addr) (m_mmu[((addr) >> 12) & 15] | ((addr) & 4095))

#define POP(DR) { RM16(_SPD, &m_##DR); _SP += 2; }
#define PUSH(SR) { _SP -= 2; WM16(_SPD, &m_##SR); }

#define JP { _PCD = ARG16(); }

#define JP_COND(cond)           \
    if (cond) {                  \
        _PCD = ARG16();          \
    } else {                     \
        (void)ARG(); _PC++;      \
    }

#define JR() {                             \
    int8_t arg = (int8_t)ARG();              \
    _PC += arg;                              \
}

#define JR_COND(cond, opcode) { \
    int8_t arg = (int8_t)ARG();  \
    if (cond) {                   \
        _PC += arg;                \
        m_extra_cycles += cc_ex[opcode]; \
    }                              \
}

#define CALL() {             \
    m_ea = ARG16();            \
    PUSH(PC);                 \
    _PCD = m_ea;               \
}

#define CALL_COND(cond, opcode) { \
    if (cond) {                     \
        m_ea = ARG16();              \
        PUSH(PC);                   \
        _PCD = m_ea;                 \
        m_extra_cycles += cc_ex[opcode]; \
    } else {                        \
        (void)ARG(); _PC++;          \
    }                                \
}

#define RET_COND(cond, opcode) { \
    if (cond) {                     \
        POP(PC);                     \
        m_extra_cycles += cc_ex[opcode]; \
    }                                \
}

#define RETN { POP(PC); m_IFF1 = m_IFF2; }

#define RETI { POP(PC); /* daisy_call_reti_device() is NOP in Vanguard8 */ }

#define LD_R_A { m_R = _A; m_R2 = _A & 0x80; }

#define LD_A_R { \
    _A = (m_R & 0x7f) | m_R2; \
    _F = (_F & CF) | SZ[_A] | (m_IFF2 << 2); \
}

#define LD_I_A { m_I = _A; }

#define LD_A_I { \
    _A = m_I; \
    _F = (_F & CF) | SZ[_A] | (m_IFF2 << 2); \
}

#define RST(addr) { PUSH(PC); _PCD = addr; }

// ALU macros
#define ADD(value) { \
    u32 ah = _AFD & 0xff00; \
    u32 res = (u8)((ah >> 8) + value); \
    _F = SZHVC_add[ah | res]; \
    _A = res; \
}

#define ADC(value) { \
    u32 ah = _AFD & 0xff00, c = _AFD & 1; \
    u32 res = (u8)((ah >> 8) + value + c); \
    _F = SZHVC_add[(c << 16) | ah | res]; \
    _A = res; \
}

#define SUB(value) { \
    u32 ah = _AFD & 0xff00; \
    u32 res = (u8)((ah >> 8) - value); \
    _F = SZHVC_sub[ah | res]; \
    _A = res; \
}

#define SBC(value) { \
    u32 ah = _AFD & 0xff00, c = _AFD & 1; \
    u32 res = (u8)((ah >> 8) - value - c); \
    _F = SZHVC_sub[(c << 16) | ah | res]; \
    _A = res; \
}

#define NEG { u8 value = _A; _A = 0; SUB(value); }

#define AND(value) _A &= value; _F = SZP[_A] | HF

#define OR(value)  _A |= value; _F = SZP[_A]

#define XOR(value) _A ^= value; _F = SZP[_A]

#define CP(value) { \
    u32 ah = _AFD & 0xff00; \
    u32 res = (u8)((ah >> 8) - value); \
    _F = SZHVC_sub[ah | res]; \
}

#define DAA { \
    u8 r = _A; \
    if (_F & NF) { \
        if ((_F & HF) | ((_A & 0xf) > 9)) r -= 6; \
        if ((_F & CF) | (_A > 0x99)) r -= 0x60; \
    } else { \
        if ((_F & HF) | ((_A & 0xf) > 9)) r += 6; \
        if ((_F & CF) | (_A > 0x99)) r += 0x60; \
    } \
    _F = (_F & 3) | (_A > 0x99) | ((_A ^ r) & HF) | SZP[r]; \
    _A = r; \
}

// Rotate/shift macros
#define RLCA \
    _A = (_A << 1) | (_A >> 7); \
    _F = (_F & (SF | ZF | PF)) | (_A & (YF | XF | CF))

#define RRCA \
    _F = (_F & (SF | ZF | PF)) | (_A & (YF | XF | CF)); \
    _A = (_A >> 1) | (_A << 7)

#define RLA { \
    u8 res = (_A << 1) | (_F & CF); \
    u8 c = (_A & 0x80) ? CF : 0; \
    _F = (_F & (SF | ZF | PF)) | c | (res & (YF | XF)); \
    _A = res; \
}

#define RRA { \
    u8 res = (_A >> 1) | (_F << 7); \
    u8 c = (_A & 0x01) ? CF : 0; \
    _F = (_F & (SF | ZF | PF)) | c | (res & (YF | XF)); \
    _A = res; \
}

#define RRD { \
    u8 n = RM(_HL); \
    WM(_HL, (n >> 4) | (_A << 4)); \
    _A = (_A & 0xf0) | (n & 0x0f); \
    _F = (_F & CF) | SZP[_A]; \
}

#define RLD { \
    u8 n = RM(_HL); \
    WM(_HL, (n << 4) | (_A & 0x0f)); \
    _A = (_A & 0xf0) | (n >> 4); \
    _F = (_F & CF) | SZP[_A]; \
}

// Exchange macros
#define EX_AF { PAIR tmp; tmp = m_AF; m_AF = m_AF2; m_AF2 = tmp; }
#define EX_DE_HL { PAIR tmp; tmp = m_DE; m_DE = m_HL; m_HL = tmp; }

#define EXX { \
    PAIR tmp; \
    tmp = m_BC; m_BC = m_BC2; m_BC2 = tmp; \
    tmp = m_DE; m_DE = m_DE2; m_DE2 = tmp; \
    tmp = m_HL; m_HL = m_HL2; m_HL2 = tmp; \
}

#define EXSP(DR) { \
    PAIR tmp = {}; \
    RM16(_SPD, &tmp); \
    WM16(_SPD, &m_##DR); \
    m_##DR = tmp; \
}

// 16-bit ALU
#define ADD16(DR, SR) { \
    u32 res = m_##DR.d + m_##SR.d; \
    _F = (_F & (SF | ZF | VF)) | \
         (((m_##DR.d ^ res ^ m_##SR.d) >> 8) & HF) | \
         ((res >> 16) & CF); \
    m_##DR.w.l = (u16)res; \
}

#define ADC16(DR) { \
    u32 res = _HLD + m_##DR.d + (_F & CF); \
    _F = (((_HLD ^ res ^ m_##DR.d) >> 8) & HF) | \
         ((res >> 16) & CF) | \
         ((res >> 8) & SF) | \
         ((res & 0xffff) ? 0 : ZF) | \
         (((m_##DR.d ^ _HLD ^ 0x8000) & (m_##DR.d ^ res) & 0x8000) >> 13); \
    _HL = (u16)res; \
}

#define SBC16(DR) { \
    u32 res = _HLD - m_##DR.d - (_F & CF); \
    _F = (((_HLD ^ res ^ m_##DR.d) >> 8) & HF) | NF | \
         ((res >> 16) & CF) | \
         ((res >> 8) & SF) | \
         ((res & 0xffff) ? 0 : ZF) | \
         (((m_##DR.d ^ _HLD) & (_HLD ^ res) & 0x8000) >> 13); \
    _HL = (u16)res; \
}

// Block opcodes
#define LDI { \
    u8 io = RM(_HL); \
    WM(_DE, io); \
    _F &= SF | ZF | CF; \
    if ((_A + io) & 0x02) _F |= YF; \
    if ((_A + io) & 0x08) _F |= XF; \
    _HL++; _DE++; _BC--; \
    if (_BC) _F |= VF; \
}

#define CPI { \
    u8 val = RM(_HL); \
    u8 res = _A - val; \
    _HL++; _BC--; \
    _F = (_F & CF) | (SZ[res] & ~(YF | XF)) | ((_A ^ val ^ res) & HF) | NF; \
    if (_F & HF) res -= 1; \
    if (res & 0x02) _F |= YF; \
    if (res & 0x08) _F |= XF; \
    if (_BC) _F |= VF; \
}

#define INI { \
    u8 io = IN(_BC); \
    _B--; \
    WM(_HL, io); \
    _HL++; \
    _F = SZ[_B]; \
    if (io & SF) _F |= NF; \
    if ((_C + io + 1) & 0x100) _F |= HF | CF; \
    if ((irep_tmp1[_C & 3][io & 3] ^ breg_tmp2[_B] ^ (_C >> 2) ^ (io >> 2)) & 1) _F |= PF; \
}

#define OUTI { \
    u8 io = RM(_HL); \
    _B--; \
    OUT(_BC, io); \
    _HL++; \
    _F = SZ[_B]; \
    if (io & SF) _F |= NF; \
    if ((_C + io + 1) & 0x100) _F |= HF | CF; \
    if ((irep_tmp1[_C & 3][io & 3] ^ breg_tmp2[_B] ^ (_C >> 2) ^ (io >> 2)) & 1) _F |= PF; \
}

#define LDD { \
    u8 io = RM(_HL); \
    WM(_DE, io); \
    _F &= SF | ZF | CF; \
    if ((_A + io) & 0x02) _F |= YF; \
    if ((_A + io) & 0x08) _F |= XF; \
    _HL--; _DE--; _BC--; \
    if (_BC) _F |= VF; \
}

#define CPD { \
    u8 val = RM(_HL); \
    u8 res = _A - val; \
    _HL--; _BC--; \
    _F = (_F & CF) | (SZ[res] & ~(YF | XF)) | ((_A ^ val ^ res) & HF) | NF; \
    if (_F & HF) res -= 1; \
    if (res & 0x02) _F |= YF; \
    if (res & 0x08) _F |= XF; \
    if (_BC) _F |= VF; \
}

#define IND { \
    u8 io = IN(_BC); \
    _B--; \
    WM(_HL, io); \
    _HL--; \
    _F = SZ[_B]; \
    if (io & SF) _F |= NF; \
    if ((_C + io - 1) & 0x100) _F |= HF | CF; \
    if ((drep_tmp1[_C & 3][io & 3] ^ breg_tmp2[_B] ^ (_C >> 2) ^ (io >> 2)) & 1) _F |= PF; \
}

#define OUTD { \
    u8 io = RM(_HL); \
    _B--; \
    OUT(_BC, io); \
    _HL--; \
    _F = SZ[_B]; \
    if (io & SF) _F |= NF; \
    if ((_C + io - 1) & 0x100) _F |= HF | CF; \
    if ((drep_tmp1[_C & 3][io & 3] ^ breg_tmp2[_B] ^ (_C >> 2) ^ (io >> 2)) & 1) _F |= PF; \
}

// Repeat block opcodes
#define LDIR { LDI; if (_BC) { _PC -= 2; m_extra_cycles += cc_ex[0xb0]; } }
#define CPIR { CPI; if (_BC && !(_F & ZF)) { _PC -= 2; m_extra_cycles += cc_ex[0xb1]; } }
#define INIR { INI; if (_B) { _PC -= 2; m_extra_cycles += cc_ex[0xb2]; } }
#define OTIR { OUTI; if (_B) { _PC -= 2; m_extra_cycles += cc_ex[0xb3]; } }
#define LDDR { LDD; if (_BC) { _PC -= 2; m_extra_cycles += cc_ex[0xb8]; } }
#define CPDR { CPD; if (_BC && !(_F & ZF)) { _PC -= 2; m_extra_cycles += cc_ex[0xb9]; } }
#define INDR { IND; if (_B) { _PC -= 2; m_extra_cycles += cc_ex[0xba]; } }
#define OTDR { OUTD; if (_B) { _PC -= 2; m_extra_cycles += cc_ex[0xbb]; } }

// EI / DI
#define EI { m_IFF1 = m_IFF2 = 1; m_after_EI = 1; }

// HD64180-specific: TST, MLT, OTIM, OTDM, SLP
#define TST(value) _F = SZP[_A & value] | HF

#define MLT(DR) { m_##DR.w.l = m_##DR.b.l * m_##DR.b.h; }

#define OTIM { \
    _B--; \
    OUT(_C, RM(_HL)); \
    _HL++; \
    _C++; \
    _F = (_B) ? NF : NF | ZF; \
}

#define OTDM { \
    _B--; \
    OUT(_C, RM(_HL)); \
    _HL--; \
    _C--; \
    _F = (_B) ? NF : NF | ZF; \
}

#define OTIMR { OTIM; if (_B) { _PC -= 2; m_extra_cycles += cc_ex[0xb3]; } }
#define OTDMR { OTDM; if (_B) { _PC -= 2; m_extra_cycles += cc_ex[0xb3]; } }
#define SLP { _PC -= 2; m_HALT = 2; }

// BIT / BIT_XY macros
#undef BIT
#define BIT(bit, reg) _F = (_F & CF) | HF | SZ_BIT[reg & (1 << bit)]
#define BIT_XY(bit, reg) \
    _F = (_F & CF) | HF | (SZ_BIT[reg & (1 << bit)] & ~(YF | XF)) | ((m_ea >> 8) & (YF | XF))

// ---- LOG stub (MAME uses LOG for debug tracing) ----
#define LOG(...) ((void)0)

// ---- MAME logerror stub ----
#define logerror(...) ((void)0)

// ---- MAME interrupt source constants ----
// Mapped to MameInterruptSource enum values (TRAP=0,NMI=1,IRQ0=2,IRQ1=3,IRQ2=4,...)
#define Z180_INT_IRQ0 2
#define Z180_INT_IRQ1 3
#define Z180_INT_IRQ2 4
#define Z180_IL_IL    0xe0

// ---- MAME daisy chain / debugger stubs ----
#define daisy_call_reti_device() ((void)0)

// Stub types for MAME interrupt dispatch used in z180op.hxx take_interrupt
namespace {
struct device_z80daisy_interface { int z80daisy_irq_ack() { return 0xff; } };
inline device_z80daisy_interface* daisy_get_irq_device() { return nullptr; }
inline int standard_irq_callback(int, unsigned) { return 0xff; }
}  // anonymous namespace

// z180op.hxx contains z180_device::take_interrupt — remap to Core.
// This replaces our manual take_interrupt with the canonical MAME version.
#define z180_device Core

// =========================================================================
//  Opcode method definition macro.
//  In MAME this expands to `void z180_device::prefix_opcode()`.
//  We override it to `void Core::prefix_opcode()` before including .hxx files.
// =========================================================================

#undef OP
#define OP(prefix, opcode) void Core::prefix##_##opcode()

// CC macro — adds extra cycles for the instruction
#undef CC
#define CC(prefix, opcode) m_extra_cycles += m_cc[TABLE_##prefix][opcode]

// ---- PREFIX/TABLE constant aliases matching z180_compat.hpp ----
// (These are also used by the CC/TABLE/EXEC macros)
#define Z180_PREFIX_op    PREFIX_op
#define Z180_PREFIX_cb    PREFIX_cb
#define Z180_PREFIX_dd    PREFIX_dd
#define Z180_PREFIX_ed    PREFIX_ed
#define Z180_PREFIX_fd    PREFIX_fd
#define Z180_PREFIX_xycb  PREFIX_xycb

#define Z180_TABLE_op     TABLE_op
#define Z180_TABLE_cb     TABLE_cb
#define Z180_TABLE_ed     TABLE_ed
#define Z180_TABLE_xy     TABLE_xy
#define Z180_TABLE_xycb   TABLE_xycb
#define Z180_TABLE_ex     TABLE_ex

#define Z180_TABLE_dd     Z180_TABLE_xy
#define Z180_TABLE_fd     Z180_TABLE_xy

// =========================================================================
//  Include MAME opcode source files.
//  These use the OP() macro (defined above) to produce Core::op_00() etc.
// =========================================================================

#include "mame/z180cb.hxx"
#include "mame/z180xy.hxx"
#include "mame/z180dd.hxx"
#include "mame/z180fd.hxx"
#include "mame/z180ed.hxx"
#include "mame/z180op.hxx"

#undef z180_device
//  Opcode dispatch table (static, shared by all Core instances)
//  Uses the TABLE() macro to generate Core::opcode_func entries.
// =========================================================================

#undef TABLE
#define TABLE(prefix) { \
    &Core::prefix##_00,&Core::prefix##_01,&Core::prefix##_02,&Core::prefix##_03,&Core::prefix##_04,&Core::prefix##_05,&Core::prefix##_06,&Core::prefix##_07, \
    &Core::prefix##_08,&Core::prefix##_09,&Core::prefix##_0a,&Core::prefix##_0b,&Core::prefix##_0c,&Core::prefix##_0d,&Core::prefix##_0e,&Core::prefix##_0f, \
    &Core::prefix##_10,&Core::prefix##_11,&Core::prefix##_12,&Core::prefix##_13,&Core::prefix##_14,&Core::prefix##_15,&Core::prefix##_16,&Core::prefix##_17, \
    &Core::prefix##_18,&Core::prefix##_19,&Core::prefix##_1a,&Core::prefix##_1b,&Core::prefix##_1c,&Core::prefix##_1d,&Core::prefix##_1e,&Core::prefix##_1f, \
    &Core::prefix##_20,&Core::prefix##_21,&Core::prefix##_22,&Core::prefix##_23,&Core::prefix##_24,&Core::prefix##_25,&Core::prefix##_26,&Core::prefix##_27, \
    &Core::prefix##_28,&Core::prefix##_29,&Core::prefix##_2a,&Core::prefix##_2b,&Core::prefix##_2c,&Core::prefix##_2d,&Core::prefix##_2e,&Core::prefix##_2f, \
    &Core::prefix##_30,&Core::prefix##_31,&Core::prefix##_32,&Core::prefix##_33,&Core::prefix##_34,&Core::prefix##_35,&Core::prefix##_36,&Core::prefix##_37, \
    &Core::prefix##_38,&Core::prefix##_39,&Core::prefix##_3a,&Core::prefix##_3b,&Core::prefix##_3c,&Core::prefix##_3d,&Core::prefix##_3e,&Core::prefix##_3f, \
    &Core::prefix##_40,&Core::prefix##_41,&Core::prefix##_42,&Core::prefix##_43,&Core::prefix##_44,&Core::prefix##_45,&Core::prefix##_46,&Core::prefix##_47, \
    &Core::prefix##_48,&Core::prefix##_49,&Core::prefix##_4a,&Core::prefix##_4b,&Core::prefix##_4c,&Core::prefix##_4d,&Core::prefix##_4e,&Core::prefix##_4f, \
    &Core::prefix##_50,&Core::prefix##_51,&Core::prefix##_52,&Core::prefix##_53,&Core::prefix##_54,&Core::prefix##_55,&Core::prefix##_56,&Core::prefix##_57, \
    &Core::prefix##_58,&Core::prefix##_59,&Core::prefix##_5a,&Core::prefix##_5b,&Core::prefix##_5c,&Core::prefix##_5d,&Core::prefix##_5e,&Core::prefix##_5f, \
    &Core::prefix##_60,&Core::prefix##_61,&Core::prefix##_62,&Core::prefix##_63,&Core::prefix##_64,&Core::prefix##_65,&Core::prefix##_66,&Core::prefix##_67, \
    &Core::prefix##_68,&Core::prefix##_69,&Core::prefix##_6a,&Core::prefix##_6b,&Core::prefix##_6c,&Core::prefix##_6d,&Core::prefix##_6e,&Core::prefix##_6f, \
    &Core::prefix##_70,&Core::prefix##_71,&Core::prefix##_72,&Core::prefix##_73,&Core::prefix##_74,&Core::prefix##_75,&Core::prefix##_76,&Core::prefix##_77, \
    &Core::prefix##_78,&Core::prefix##_79,&Core::prefix##_7a,&Core::prefix##_7b,&Core::prefix##_7c,&Core::prefix##_7d,&Core::prefix##_7e,&Core::prefix##_7f, \
    &Core::prefix##_80,&Core::prefix##_81,&Core::prefix##_82,&Core::prefix##_83,&Core::prefix##_84,&Core::prefix##_85,&Core::prefix##_86,&Core::prefix##_87, \
    &Core::prefix##_88,&Core::prefix##_89,&Core::prefix##_8a,&Core::prefix##_8b,&Core::prefix##_8c,&Core::prefix##_8d,&Core::prefix##_8e,&Core::prefix##_8f, \
    &Core::prefix##_90,&Core::prefix##_91,&Core::prefix##_92,&Core::prefix##_93,&Core::prefix##_94,&Core::prefix##_95,&Core::prefix##_96,&Core::prefix##_97, \
    &Core::prefix##_98,&Core::prefix##_99,&Core::prefix##_9a,&Core::prefix##_9b,&Core::prefix##_9c,&Core::prefix##_9d,&Core::prefix##_9e,&Core::prefix##_9f, \
    &Core::prefix##_a0,&Core::prefix##_a1,&Core::prefix##_a2,&Core::prefix##_a3,&Core::prefix##_a4,&Core::prefix##_a5,&Core::prefix##_a6,&Core::prefix##_a7, \
    &Core::prefix##_a8,&Core::prefix##_a9,&Core::prefix##_aa,&Core::prefix##_ab,&Core::prefix##_ac,&Core::prefix##_ad,&Core::prefix##_ae,&Core::prefix##_af, \
    &Core::prefix##_b0,&Core::prefix##_b1,&Core::prefix##_b2,&Core::prefix##_b3,&Core::prefix##_b4,&Core::prefix##_b5,&Core::prefix##_b6,&Core::prefix##_b7, \
    &Core::prefix##_b8,&Core::prefix##_b9,&Core::prefix##_ba,&Core::prefix##_bb,&Core::prefix##_bc,&Core::prefix##_bd,&Core::prefix##_be,&Core::prefix##_bf, \
    &Core::prefix##_c0,&Core::prefix##_c1,&Core::prefix##_c2,&Core::prefix##_c3,&Core::prefix##_c4,&Core::prefix##_c5,&Core::prefix##_c6,&Core::prefix##_c7, \
    &Core::prefix##_c8,&Core::prefix##_c9,&Core::prefix##_ca,&Core::prefix##_cb,&Core::prefix##_cc,&Core::prefix##_cd,&Core::prefix##_ce,&Core::prefix##_cf, \
    &Core::prefix##_d0,&Core::prefix##_d1,&Core::prefix##_d2,&Core::prefix##_d3,&Core::prefix##_d4,&Core::prefix##_d5,&Core::prefix##_d6,&Core::prefix##_d7, \
    &Core::prefix##_d8,&Core::prefix##_d9,&Core::prefix##_da,&Core::prefix##_db,&Core::prefix##_dc,&Core::prefix##_dd,&Core::prefix##_de,&Core::prefix##_df, \
    &Core::prefix##_e0,&Core::prefix##_e1,&Core::prefix##_e2,&Core::prefix##_e3,&Core::prefix##_e4,&Core::prefix##_e5,&Core::prefix##_e6,&Core::prefix##_e7, \
    &Core::prefix##_e8,&Core::prefix##_e9,&Core::prefix##_ea,&Core::prefix##_eb,&Core::prefix##_ec,&Core::prefix##_ed,&Core::prefix##_ee,&Core::prefix##_ef, \
    &Core::prefix##_f0,&Core::prefix##_f1,&Core::prefix##_f2,&Core::prefix##_f3,&Core::prefix##_f4,&Core::prefix##_f5,&Core::prefix##_f6,&Core::prefix##_f7, \
    &Core::prefix##_f8,&Core::prefix##_f9,&Core::prefix##_fa,&Core::prefix##_fb,&Core::prefix##_fc,&Core::prefix##_fd,&Core::prefix##_fe,&Core::prefix##_ff  \
}

const Core::opcode_func Core::s_ops[6][0x100] = {
    TABLE(op),
    TABLE(cb),
    TABLE(dd),
    TABLE(ed),
    TABLE(fd),
    TABLE(xycb)
};

// =========================================================================
//  Execution dispatchers — called by Core to execute an opcode.
//  These match MAME's EXEC_PROTOTYPE pattern.
// =========================================================================

int Core::exec_op(u8 opcode) {
    (this->*s_ops[PREFIX_op][opcode])();
    return m_cc[TABLE_op][opcode];
}

int Core::exec_cb(u8 opcode) {
    (this->*s_ops[PREFIX_cb][opcode])();
    return m_cc[TABLE_cb][opcode];
}

int Core::exec_dd(u8 opcode) {
    (this->*s_ops[PREFIX_dd][opcode])();
    return m_cc[TABLE_xy][opcode];
}

int Core::exec_ed(u8 opcode) {
    (this->*s_ops[PREFIX_ed][opcode])();
    return m_cc[TABLE_ed][opcode];
}

int Core::exec_fd(u8 opcode) {
    (this->*s_ops[PREFIX_fd][opcode])();
    return m_cc[TABLE_xy][opcode];
}

int Core::exec_xycb(u8 opcode) {
    (this->*s_ops[PREFIX_xycb][opcode])();
    return m_cc[TABLE_xycb][opcode];
}

// =========================================================================
//  Core public API
// =========================================================================

Core::Core(Callbacks callbacks) : callbacks_(std::move(callbacks)) {
    init_flag_tables();
    reset();
}

void Core::reset() {
    _PPC = 0;
    _PCD = 0;
    _SPD = 0;
    _AFD = 0;
    _BCD = 0;
    _DED = 0;
    _HLD = 0;
    _IXD = 0;
    _IYD = 0;
    m_AF2.d = 0;
    m_BC2.d = 0;
    m_DE2.d = 0;
    m_HL2.d = 0;
    m_R = 0;
    m_R2 = 0;
    m_IFF1 = 0;
    m_IFF2 = 0;
    m_HALT = 0;
    m_IM = 0;
    m_I = 0;
    m_tmdr_latch = 0;
    m_read_tcr_tmdr[0] = 0;
    m_read_tcr_tmdr[1] = 0;
    m_iol = 0;
    std::memset(m_mmu, 0, sizeof(m_mmu));
    m_tmdrh[0] = 0;
    m_tmdrh[1] = 0;
    m_nmi_state = CLEAR_LINE;
    m_nmi_pending = 0;
    m_irq_state[0] = CLEAR_LINE;
    m_irq_state[1] = CLEAR_LINE;
    m_irq_state[2] = CLEAR_LINE;
    m_after_EI = 0;
    m_ea = 0;
    m_extra_cycles = 0;
    last_opcode_ = 0;

    // Point m_cc array at the cycle-count tables
    m_cc[TABLE_op]   = const_cast<u8*>(cc_op);
    m_cc[TABLE_cb]   = const_cast<u8*>(cc_cb);
    m_cc[TABLE_ed]   = const_cast<u8*>(cc_ed);
    m_cc[TABLE_xy]   = const_cast<u8*>(cc_xy);
    m_cc[TABLE_xycb] = const_cast<u8*>(cc_xycb);
    m_cc[TABLE_ex]   = const_cast<u8*>(cc_ex);

    _IX = _IY = 0xffff;  // IX and IY are FFFF after reset
    _F = ZF;              // Zero flag is set

    for (int i = 0; i <= static_cast<int>(MameInterruptSource::MAX); i++) {
        m_int_pending[i] = 0;
    }

    m_frc = 0xff;
    m_frc_prescale = 0;

    // Reset I/O registers to MAME defaults, then override for Vanguard8 spec
    m_tcr = 0x00;
    m_dma_iar1.b.h2 = 0x00;
    m_dstat = DSTAT_DWE1 | DSTAT_DWE0;
    m_dmode = 0x00;
    m_dcntl = 0xf0;
    m_il = 0x00;
    m_itc = ITC_ITE0;
    m_rcr = RCR_REFE | RCR_REFW;
    m_mmu_cbr = 0x00;
    m_mmu_bbr = 0x00;
    m_mmu_cbar = 0xF0;   // MAME default (Vanguard8 spec wants 0xFF — override below)
    m_omcr = OMCR_M1E | OMCR_M1TE | OMCR_IOC;
    m_iocr = 0x00;

    // Vanguard8 spec overrides
    m_mmu_cbar = 0xFF;   // CBAR=0xFF: flat passthrough (no MMU translation)
    m_mmu_cbr = 0x00;
    m_mmu_bbr = 0x00;
    m_il = 0x00;
    m_itc = ITC_ITE0;

    m_tmdr_value[0] = 0xFFFF;
    m_tmdr_value[1] = 0xFFFF;
    m_tmdr[0].w = 0;
    m_tmdr[1].w = 0;
    m_rldr[0].w = 0xFFFF;
    m_rldr[1].w = 0xFFFF;

    m_dma_sar0.d = 0;
    m_dma_dar0.d = 0;
    m_dma_mar1.d = 0;
    m_dma_iar1.d = 0;
    m_dma_bcr[0].w = 0;
    m_dma_bcr[1].w = 0;

    z180_mmu();
}

// ---- Register accessors ----

auto Core::pc() const -> u16 { return _PC; }
auto Core::accumulator() const -> u8 { return _A; }
auto Core::register_i() const -> u8 { return m_I; }
auto Core::register_il() const -> u8 { return m_il; }
auto Core::register_itc() const -> u8 { return m_itc; }
auto Core::cbar() const -> u8 { return m_mmu_cbar; }
auto Core::cbr() const -> u8 { return m_mmu_cbr; }
auto Core::bbr() const -> u8 { return m_mmu_bbr; }
auto Core::interrupt_mode() const -> u8 { return m_IM; }
auto Core::halted() const -> bool { return m_HALT != 0; }

void Core::set_register_i(u8 value) { m_I = value; }
void Core::set_interrupt_mode(u8 mode) { m_IM = static_cast<u8>(mode & 0x03); }
void Core::set_iff1(bool enabled) { m_IFF1 = enabled ? 1 : 0; }
void Core::set_iff2(bool enabled) { m_IFF2 = enabled ? 1 : 0; }

void Core::advance_tstates(u64 tstates) {
    handle_io_timers(static_cast<int>(tstates));
}

// ---- Memory translation (Vanguard8 API) ----

auto Core::translate_logical_address(u16 logical_address) const -> u32 {
    if (m_mmu_cbar == 0xFF) return logical_address;
    const auto ca0_end = static_cast<u16>((m_mmu_cbar >> 4) << 12);
    const auto ca1_start = static_cast<u16>((m_mmu_cbar & 0x0F) << 12);
    if (logical_address < ca0_end) return logical_address;
    if (logical_address >= ca1_start)
        return (static_cast<u32>(m_mmu_cbr) << 12) + (logical_address - ca1_start);
    return (static_cast<u32>(m_mmu_bbr) << 12) + (logical_address - ca0_end);
}

auto Core::read_logical(u16 logical_address) -> u8 {
    u8 value = callbacks_.read_memory(translate_logical_address(logical_address));
    if (callbacks_.observe_logical_memory_read)
        callbacks_.observe_logical_memory_read(logical_address, value);
    return value;
}

void Core::write_logical(u16 logical_address, u8 value) {
    callbacks_.write_memory(translate_logical_address(logical_address), value);
    if (callbacks_.observe_logical_memory_write)
        callbacks_.observe_logical_memory_write(logical_address, value);
}

// ---- MMU ----

void Core::z180_mmu() {
    // Vanguard8 CBAR nibble interpretation (swapped vs MAME/HD64180 datasheet):
    //   Upper nibble (bits 7-4) = CA0 end page (bank area start)
    //   Lower nibble (bits 3-0) = CA1 start page
    // Physical address formula (Vanguard8 spec):
    //   CA0: identity (logical == physical)
    //   Bank: (BBR << 12) + (logical - CA0_end)
    //   CA1:  (CBR << 12) + (logical - CA1_start)
    offs_t ca0_end   = (static_cast<offs_t>(m_mmu_cbar) >> 4) << 12;
    offs_t ca1_start = (static_cast<offs_t>(m_mmu_cbar) & 15) << 12;
    offs_t cbr_base  = static_cast<offs_t>(m_mmu_cbr) << 12;
    offs_t bbr_base  = static_cast<offs_t>(m_mmu_bbr) << 12;

    for (int page = 0; page < 16; page++) {
        offs_t addr = static_cast<offs_t>(page) << 12;
        if (addr >= ca1_start)
            addr = cbr_base + (addr - ca1_start);
        else if (addr >= ca0_end)
            addr = bbr_base + (addr - ca0_end);
        // else CA0: identity
        m_mmu[page] = (addr & 0xfffff);
    }
}

auto Core::mmu_remap(offs_t addr) const -> offs_t {
    if (m_mmu_cbar == 0xFF) return addr;
    return MMU_REMAP_ADDR(addr);
}

// ---- Rotate / shift / bit primitives ----

u8 Core::INC(u8 value) {
    u8 res = value + 1;
    _F = (_F & CF) | SZHV_inc[res];
    return res;
}

u8 Core::DEC(u8 value) {
    u8 res = value - 1;
    _F = (_F & CF) | SZHV_dec[res];
    return res;
}

u8 Core::RLC(u8 value) {
    unsigned res = value;
    unsigned c = (res & 0x80) ? CF : 0;
    res = ((res << 1) | (res >> 7)) & 0xff;
    _F = SZP[res] | c;
    return res;
}

u8 Core::RRC(u8 value) {
    unsigned res = value;
    unsigned c = (res & 0x01) ? CF : 0;
    res = ((res >> 1) | (res << 7)) & 0xff;
    _F = SZP[res] | c;
    return res;
}

u8 Core::RL(u8 value) {
    unsigned res = value;
    unsigned c = (res & 0x80) ? CF : 0;
    res = ((res << 1) | (_F & CF)) & 0xff;
    _F = SZP[res] | c;
    return res;
}

u8 Core::RR(u8 value) {
    unsigned res = value;
    unsigned c = (res & 0x01) ? CF : 0;
    res = ((res >> 1) | (_F << 7)) & 0xff;
    _F = SZP[res] | c;
    return res;
}

u8 Core::SLA(u8 value) {
    unsigned res = value;
    unsigned c = (res & 0x80) ? CF : 0;
    res = (res << 1) & 0xff;
    _F = SZP[res] | c;
    return res;
}

u8 Core::SRA(u8 value) {
    unsigned res = value;
    unsigned c = (res & 0x01) ? CF : 0;
    res = ((res >> 1) | (res & 0x80)) & 0xff;
    _F = SZP[res] | c;
    return res;
}

u8 Core::SLL(u8 value) {
    unsigned res = value;
    unsigned c = (res & 0x80) ? CF : 0;
    res = ((res << 1) | 0x01) & 0xff;
    _F = SZP[res] | c;
    return res;
}

u8 Core::SRL(u8 value) {
    unsigned res = value;
    unsigned c = (res & 0x01) ? CF : 0;
    res = (res >> 1) & 0xff;
    _F = SZP[res] | c;
    return res;
}

u8 Core::RES(u8 bit, u8 value) {
    return value & ~(1 << bit);
}

u8 Core::SET(u8 bit, u8 value) {
    return value | (1 << bit);
}

// ---- Internal I/O (adapted from MAME z180_internal_port_read/write) ----

u8 Core::z180_readcontrol(offs_t port) {
    return z180_internal_port_read(port & 0x3f);
}

u8 Core::z180_internal_port_read(u8 port) {
    u8 data = 0xff;

    switch (port) {
    case 0x00: case 0x01: case 0x02: case 0x03:
    case 0x04: case 0x05: case 0x06: case 0x07:
    case 0x08: case 0x09: case 0x0a: case 0x0b:
        // ASCI/CSIO — stubbed (Vanguard8 doesn't use serial peripherals)
        break;

    case 0x0c:  // TMDR0L
        data = m_tmdr_value[0] & 0x00ff;
        if ((m_tcr & TCR_TDE0) == 0) {
            m_tmdr_latch |= 1;
            m_tmdrh[0] = (m_tmdr_value[0] & 0xff00) >> 8;
        }
        if (m_read_tcr_tmdr[0]) {
            m_tcr &= ~TCR_TIF0;
            m_read_tcr_tmdr[0] = 0;
        } else {
            m_read_tcr_tmdr[0] = 1;
        }
        break;

    case 0x0d:  // TMDR0H
        if (m_tmdr_latch & 1) {
            m_tmdr_latch &= ~1;
            data = m_tmdrh[0];
        } else {
            data = (m_tmdr_value[0] & 0xff00) >> 8;
        }
        if (m_read_tcr_tmdr[0]) {
            m_tcr &= ~TCR_TIF0;
            m_read_tcr_tmdr[0] = 0;
        } else {
            m_read_tcr_tmdr[0] = 1;
        }
        break;

    case 0x0e: data = m_rldr[0].b.l; break;
    case 0x0f: data = m_rldr[0].b.h; break;

    case 0x10:  // TCR
        data = m_tcr;
        if (m_read_tcr_tmdr[0]) { m_tcr &= ~TCR_TIF0; m_read_tcr_tmdr[0] = 0; }
        else { m_read_tcr_tmdr[0] = 1; }
        if (m_read_tcr_tmdr[1]) { m_tcr &= ~TCR_TIF1; m_read_tcr_tmdr[1] = 0; }
        else { m_read_tcr_tmdr[1] = 1; }
        break;

    case 0x14:  // TMDR1L
        data = m_tmdr_value[1];
        if ((m_tcr & TCR_TDE1) == 0) {
            m_tmdr_latch |= 2;
            m_tmdrh[1] = (m_tmdr_value[1] & 0xff00) >> 8;
        }
        if (m_read_tcr_tmdr[1]) { m_tcr &= ~TCR_TIF1; m_read_tcr_tmdr[1] = 0; }
        else { m_read_tcr_tmdr[1] = 1; }
        break;

    case 0x15:  // TMDR1H
        if (m_tmdr_latch & 2) { m_tmdr_latch &= ~2; data = m_tmdrh[1]; }
        else { data = (m_tmdr_value[1] & 0xff00) >> 8; }
        if (m_read_tcr_tmdr[1]) { m_tcr &= ~TCR_TIF1; m_read_tcr_tmdr[1] = 0; }
        else { m_read_tcr_tmdr[1] = 1; }
        break;

    case 0x16: data = m_rldr[1].b.l; break;
    case 0x17: data = m_rldr[1].b.h; break;
    case 0x18: data = m_frc; break;
    case 0x19: data = 0xff; break;

    // DMA registers (0x20-0x32) — read from core state
    case 0x20: data = m_dma_sar0.b.l; break;
    case 0x21: data = m_dma_sar0.b.h; break;
    case 0x22: data = m_dma_sar0.b.h2 & 0x0f; break;
    case 0x23: data = m_dma_dar0.b.l; break;
    case 0x24: data = m_dma_dar0.b.h; break;
    case 0x25: data = m_dma_dar0.b.h2 & 0x0f; break;
    case 0x26: data = m_dma_bcr[0].b.l; break;
    case 0x27: data = m_dma_bcr[0].b.h; break;
    case 0x28: data = m_dma_mar1.b.l; break;
    case 0x29: data = m_dma_mar1.b.h; break;
    case 0x2a: data = m_dma_mar1.b.h2 & 0x0f; break;
    case 0x2b: data = m_dma_iar1.b.l; break;
    case 0x2c: data = m_dma_iar1.b.h; break;
    case 0x2d: data = m_dma_iar1.b.h2 & 0x0f; break;
    case 0x2e: data = m_dma_bcr[1].b.l; break;
    case 0x2f: data = m_dma_bcr[1].b.h; break;
    case 0x30: data = m_dstat | ~DSTAT_MASK; break;
    case 0x31: data = m_dmode | ~DMODE_MASK; break;
    case 0x32: data = m_dcntl; break;

    case 0x33: data = m_il & IL_MASK; break;
    case 0x34: data = m_itc | ~ITC_MASK; break;
    case 0x36: data = m_rcr | ~RCR_MASK; break;
    case 0x38: data = m_mmu_cbr; break;
    case 0x39: data = m_mmu_bbr; break;
    case 0x3a: data = m_mmu_cbar; break;
    case 0x3e: data = m_omcr | OMCR_M1TE | ~OMCR_MASK; break;
    case 0x3f: data = m_iocr | ~0xe0; break;

    default: data = 0xff; break;
    }

    if (callbacks_.observe_internal_io_read)
        callbacks_.observe_internal_io_read(static_cast<u16>(port), data);
    return data;
}

void Core::z180_writecontrol(offs_t port, u8 data) {
    z180_internal_port_write(port & 0x3f, data);
}

void Core::z180_internal_port_write(u8 port, u8 data) {
    switch (port) {
    case 0x00: case 0x01: case 0x02: case 0x03:
    case 0x04: case 0x05: case 0x06: case 0x07:
    case 0x08: case 0x09: case 0x0a: case 0x0b:
        // ASCI/CSIO — stubbed
        break;

    case 0x0c:  // TMDR0L
        m_tmdr[0].b.l = data;
        m_tmdr_value[0] = (m_tmdr_value[0] & 0xff00) | m_tmdr[0].b.l;
        break;
    case 0x0d:  // TMDR0H
        m_tmdr[0].b.h = data;
        m_tmdr_value[0] = (m_tmdr_value[0] & 0x00ff) | (m_tmdr[0].b.h << 8);
        break;
    case 0x0e: m_rldr[0].b.l = data; break;
    case 0x0f: m_rldr[0].b.h = data; break;

    case 0x10: {  // TCR
        u16 old = m_tcr;
        m_tcr = (m_tcr & (TCR_TIF1 | TCR_TIF0)) | (data & ~(TCR_TIF1 | TCR_TIF0));
        if (!(old & TCR_TDE0) && (m_tcr & TCR_TDE0)) m_tmdr_value[0] = 0;
        if (!(old & TCR_TDE1) && (m_tcr & TCR_TDE1)) m_tmdr_value[1] = 0;
        break;
    }

    case 0x14:  // TMDR1L
        m_tmdr[1].b.l = data;
        m_tmdr_value[1] = (m_tmdr_value[1] & 0xff00) | m_tmdr[1].b.l;
        break;
    case 0x15:  // TMDR1H
        m_tmdr[1].b.h = data;
        m_tmdr_value[1] = (m_tmdr_value[1] & 0x00ff) | (m_tmdr[1].b.h << 8);
        break;
    case 0x16: m_rldr[1].b.l = data; break;
    case 0x17: m_rldr[1].b.h = data; break;
    case 0x18: /* FRC read-only */ break;

    // DMA registers (0x20-0x32)
    case 0x20: m_dma_sar0.b.l = data; break;
    case 0x21: m_dma_sar0.b.h = data; break;
    case 0x22: m_dma_sar0.b.h2 = data & 0x0f; break;
    case 0x23: m_dma_dar0.b.l = data; break;
    case 0x24: m_dma_dar0.b.h = data; break;
    case 0x25: m_dma_dar0.b.h2 = data & 0x0f; break;
    case 0x26: m_dma_bcr[0].b.l = data; break;
    case 0x27: m_dma_bcr[0].b.h = data; break;
    case 0x28: m_dma_mar1.b.l = data; break;
    case 0x29: m_dma_mar1.b.h = data; break;
    case 0x2a: m_dma_mar1.b.h2 = data & 0x0f; break;
    case 0x2b: m_dma_iar1.b.l = data; break;
    case 0x2c: m_dma_iar1.b.h = data; break;
    case 0x2d: m_dma_iar1.b.h2 = data & 0x0f; break;
    case 0x2e: m_dma_bcr[1].b.l = data; break;
    case 0x2f: m_dma_bcr[1].b.h = data; break;

    case 0x30:  // DSTAT
        m_dstat = (m_dstat & DSTAT_DME) | (data & DSTAT_MASK & ~DSTAT_DME);
        if ((data & (DSTAT_DE1 | DSTAT_DWE1)) == DSTAT_DE1) m_dstat |= DSTAT_DME;
        if ((data & (DSTAT_DE0 | DSTAT_DWE0)) == DSTAT_DE0) m_dstat |= DSTAT_DME;
        break;
    case 0x31: m_dmode = data & DMODE_MASK; break;
    case 0x32: m_dcntl = data; break;
    case 0x33: m_il = data & IL_MASK; break;
    case 0x34: m_itc = (m_itc & ITC_UFO) | (data & ITC_MASK & ~ITC_UFO); break;
    case 0x36: m_rcr = data & RCR_MASK; break;
    case 0x38: m_mmu_cbr = data; z180_mmu(); break;
    case 0x39: m_mmu_bbr = data; z180_mmu(); maybe_warn_illegal_bbr(); break;
    case 0x3a: m_mmu_cbar = data; z180_mmu(); break;
    case 0x3e: m_omcr = data & OMCR_MASK; break;
    case 0x3f: m_iocr = data & 0xe0; break;
    default: break;
    }

    if (callbacks_.observe_internal_io_write)
        callbacks_.observe_internal_io_write(static_cast<u16>(port), data);
}

// ---- PRT timers ----

void Core::clock_timers() {
    if (m_tcr & TCR_TDE0) {
        if (m_tmdr_value[0] == 0) {
            m_tmdr_value[0] = m_rldr[0].w;
            m_tcr |= TCR_TIF0;
        } else {
            m_tmdr_value[0]--;
        }
    }
    if (m_tcr & TCR_TDE1) {
        if (m_tmdr_value[1] == 0) {
            m_tmdr_value[1] = m_rldr[1].w;
            m_tcr |= TCR_TIF1;
        } else {
            m_tmdr_value[1]--;
        }
    }
    if ((m_tcr & TCR_TIE0) && (m_tcr & TCR_TIF0)) {
        if (m_IFF1 && !m_after_EI)
            m_int_pending[static_cast<int>(MameInterruptSource::PRT0)] = 1;
    }
    if ((m_tcr & TCR_TIE1) && (m_tcr & TCR_TIF1)) {
        if (m_IFF1 && !m_after_EI)
            m_int_pending[static_cast<int>(MameInterruptSource::PRT1)] = 1;
    }
}

int Core::check_interrupts() {
    if (m_IFF1 && !m_after_EI) {
        if (m_irq_state[0] != CLEAR_LINE && (m_itc & ITC_ITE0) == ITC_ITE0)
            m_int_pending[static_cast<int>(MameInterruptSource::IRQ0)] = 1;
        if (m_irq_state[1] != CLEAR_LINE && (m_itc & ITC_ITE1) == ITC_ITE1)
            m_int_pending[static_cast<int>(MameInterruptSource::IRQ1)] = 1;
        if (m_irq_state[2] != CLEAR_LINE && (m_itc & ITC_ITE2) == ITC_ITE2)
            m_int_pending[static_cast<int>(MameInterruptSource::IRQ2)] = 1;
        // ASCI/CSIO interrupt checks stubbed — not used in Vanguard8
    }

    int cycles = 0;
    for (int i = 0; i <= static_cast<int>(MameInterruptSource::MAX); i++) {
        if (m_int_pending[i]) {
            cycles += take_interrupt(i);
            m_int_pending[i] = 0;
            break;
        }
    }
    return cycles;
}

void Core::handle_io_timers(int cycles) {
    while (cycles-- > 0) {
        m_frc_prescale++;
        if (m_frc_prescale >= 10) {
            m_frc_prescale = 0;
            m_frc--;
            if ((m_frc & 1) == 0) clock_timers();
        }
    }
}

// ---- Interrupt handling ----

auto Core::service_pending_interrupt(bool int0_asserted, bool int1_asserted)
    -> std::optional<InterruptService> {
    if (!m_IFF1 || m_after_EI) return std::nullopt;

    // Update IRQ line state
    m_irq_state[0] = int0_asserted ? ASSERT_LINE : CLEAR_LINE;
    m_irq_state[1] = int1_asserted ? ASSERT_LINE : CLEAR_LINE;

    // Record pending state and PC before check_interrupts dispatches
    bool had_int0 = int0_asserted && (m_itc & ITC_ITE0);
    bool had_int1 = int1_asserted && (m_itc & ITC_ITE1);
    bool had_prt0 = m_int_pending[static_cast<int>(MameInterruptSource::PRT0)] != 0;
    bool had_prt1 = m_int_pending[static_cast<int>(MameInterruptSource::PRT1)] != 0;
    auto old_pc = _PC;

    // check_interrupts calls take_interrupt (MAME version) which handles
    // IRQ0 via IM0/1/2, and non-IRQ0 via the IL vectored scheme.
    check_interrupts();

    // Determine which interrupt was serviced by checking what changed
    if (_PC != old_pc) {
        // INT0 (IRQ0) → IM1 handler is always 0x0038
        if (had_int0 && !m_int_pending[static_cast<int>(MameInterruptSource::IRQ0)]) {
            if (callbacks_.acknowledge_int1) callbacks_.acknowledge_int1();
            return InterruptService{.source = InterruptSource::int0, .handler_address = 0x0038};
        }
        // INT1 → vectored via IL
        if (had_int1 && !m_int_pending[static_cast<int>(MameInterruptSource::IRQ1)]) {
            if (callbacks_.acknowledge_int1) callbacks_.acknowledge_int1();
            return InterruptService{.source = InterruptSource::int1,
                                    .handler_address = vectored_handler_address(0x00)};
        }
        // PRT0 → vectored via IL with offset 0x04 (handled by take_interrupt)
        if (had_prt0 && !m_int_pending[static_cast<int>(MameInterruptSource::PRT0)]) {
            m_tcr &= ~TCR_TIF0;
            return InterruptService{.source = InterruptSource::prt0,
                                    .handler_address = vectored_handler_address(0x04)};
        }
        // PRT1 → vectored via IL with offset 0x06 (handled by take_interrupt)
        if (had_prt1 && !m_int_pending[static_cast<int>(MameInterruptSource::PRT1)]) {
            m_tcr &= ~TCR_TIF1;
            return InterruptService{.source = InterruptSource::prt1,
                                    .handler_address = vectored_handler_address(0x06)};
        }
    }

    return std::nullopt;
}

// ---- Execution ----

void Core::step_one() {
    m_R++;
    m_extra_cycles = 0;
    // EI defers interrupt acceptance for one instruction: after_EI is
    // decremented here (before the opcode) so that EI sets it to 1 and
    // the next instruction's entry decrements it back to 0.
    if (m_after_EI) m_after_EI--;
    exec_op(ROP());
}

void Core::run_until_halt(std::size_t max_instructions) {
    std::size_t executed = 0;
    while (!m_HALT) {
        if (executed >= max_instructions)
            throw std::runtime_error("Test ROM did not halt within the instruction budget.");
        step_one();
        ++executed;
    }
}

auto Core::next_instruction_tstates() const -> u64 {
    // Peek the next opcode without executing it
    u32 addr = mmu_remap(_PCD);
    u8 opcode = callbacks_.read_memory(addr);

    if (opcode == 0xdd || opcode == 0xfd) {
        // DD/FD prefix — peek next byte for the real opcode
        u32 next_addr = mmu_remap(static_cast<u16>(_PCD + 1));
        u8 next = callbacks_.read_memory(next_addr);
        if (next == 0xcb) return cc_xycb[0];  // XY+CB prefix
        return cc_xy[next];
    }
    if (opcode == 0xcb) return cc_cb[0];
    if (opcode == 0xed) return cc_ed[0];
    return cc_op[opcode];
}

auto Core::peek_next_opcode() const -> u8 {
    u32 addr = mmu_remap(_PCD);
    return callbacks_.read_memory(addr);
}

// ---- Register snapshot ----

auto Core::register_snapshot() const -> RegisterSnapshot {
    return RegisterSnapshot{
        .af = m_AF.w.l,
        .bc = m_BC.w.l,
        .de = m_DE.w.l,
        .hl = m_HL.w.l,
        .sp = m_SP.w.l,
        .pc = m_PC.w.l,
        .af_alt = m_AF2.w.l,
        .bc_alt = m_BC2.w.l,
        .de_alt = m_DE2.w.l,
        .hl_alt = m_HL2.w.l,
        .ix = m_IX.w.l,
        .iy = m_IY.w.l,
        .r = m_R,
        .i = m_I,
        .il = m_il,
        .itc = m_itc,
        .cbar = m_mmu_cbar,
        .cbr = m_mmu_cbr,
        .bbr = m_mmu_bbr,
        .interrupt_mode = m_IM,
        .iff1 = m_IFF1 != 0,
        .iff2 = m_IFF2 != 0,
        .ei_delay = static_cast<u8>(m_after_EI),
        .halted = m_HALT != 0,
        .tcr = m_tcr,
        .tmdr0 = m_tmdr_value[0],
        .rldr0 = m_rldr[0].w,
        .tmdr1 = m_tmdr_value[1],
        .rldr1 = m_rldr[1].w,
    };
}

void Core::load_register_snapshot(const RegisterSnapshot& state) {
    m_AF.w.l = state.af;
    m_BC.w.l = state.bc;
    m_DE.w.l = state.de;
    m_HL.w.l = state.hl;
    m_SP.w.l = state.sp;
    m_PC.w.l = state.pc;
    m_AF2.w.l = state.af_alt;
    m_BC2.w.l = state.bc_alt;
    m_DE2.w.l = state.de_alt;
    m_HL2.w.l = state.hl_alt;
    m_IX.w.l = state.ix;
    m_IY.w.l = state.iy;
    m_R = state.r;
    m_I = state.i;
    m_il = state.il;
    m_itc = state.itc;
    m_mmu_cbar = state.cbar;
    m_mmu_cbr = state.cbr;
    m_mmu_bbr = state.bbr;
    m_IM = state.interrupt_mode;
    m_IFF1 = state.iff1 ? 1 : 0;
    m_IFF2 = state.iff2 ? 1 : 0;
    m_after_EI = state.ei_delay;
    m_HALT = state.halted ? 1 : 0;
    m_tcr = state.tcr;
    m_tmdr_value[0] = state.tmdr0;
    m_rldr[0].w = state.rldr0;
    m_tmdr_value[1] = state.tmdr1;
    m_rldr[1].w = state.rldr1;
    z180_mmu();
}

// ---- Vanguard8 spec helpers ----

auto Core::vectored_handler_address(u8 fixed_code) -> u16 {
    u16 vector_pointer =
        static_cast<u16>((static_cast<u16>(m_I) << 8) | ((m_il & IL_MASK) | fixed_code));
    return static_cast<u16>(
        RM(vector_pointer) |
        (RM(static_cast<u16>(vector_pointer + 1)) << 8)
    );
}

void Core::service_vectored_interrupt(InterruptSource /*source*/, u8 fixed_code) {
    wake_from_halt_for_interrupt();
    m_IFF1 = m_IFF2 = 0;
    u16 handler_address = vectored_handler_address(fixed_code);
    PUSH(PC);
    _PCD = handler_address;
}

void Core::wake_from_halt_for_interrupt() {
    if (m_HALT) {
        m_HALT = 0;
        _PC++;
    }
}

void Core::maybe_warn_illegal_bbr() {
    if (m_mmu_bbr >= 0xF0 && callbacks_.record_warning) {
        callbacks_.record_warning(
            "Illegal BBR write: " + std::to_string(m_mmu_bbr) +
            " (>= 0xF0 maps into SRAM space)"
        );
    }
}

// ---- Internal I/O pass-through (used by adapter) ----

auto Core::in0(u8 port) -> u8 {
    return z180_internal_port_read(port & 0x3f);
}

void Core::out0(u8 port, u8 value) {
    z180_internal_port_write(port & 0x3f, value);
}

// ---- is_internal_io_address (needed by macro) ----
// Already defined inline above, but we need the out-of-line declaration too.
// The inline definition in the macro section above handles actual calls.

// =========================================================================
//  Push/pop helpers for interrupt service (Vanguard8 API compatibility)
// =========================================================================

void Core::push_word(u16 value) {
    _SP -= 2;
    WM(_SPD, static_cast<u8>(value & 0xff));
    WM(static_cast<u16>(_SPD + 1), static_cast<u8>((value >> 8) & 0xff));
}

auto Core::pop_word() -> u16 {
    u8 lo = RM(_SPD);
    u8 hi = RM(static_cast<u16>(_SPD + 1));
    _SP += 2;
    return static_cast<u16>(lo | (hi << 8));
}

}  // namespace vanguard8::third_party::z180

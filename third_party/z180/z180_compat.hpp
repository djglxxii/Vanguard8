// license:BSD-3-Clause
// Adapted from MAME z180.h for standalone use in Vanguard8
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <stdexcept>
#include <string>

namespace vanguard8::third_party::z180 {

// MAME type aliases
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using offs_t = std::uint32_t;

// MAME's PAIR type (used heavily in opcodes)
// Layout (little-endian): d = uint32_t, w.l = bits 0-15, w.h = bits 16-31,
// b.l = bits 0-7, b.h = bits 8-15, b.h2 = bits 16-23, b.h3 = bits 24-31.
struct PAIR {
    union {
        std::uint32_t d;
        struct {
            std::uint16_t l;
            std::uint16_t h;
        } w;
        struct {
            std::uint8_t l;
            std::uint8_t h;
            std::uint8_t h2;
            std::uint8_t h3;
        } b;
    };
};

// MAME's PAIR16 type
struct PAIR16 {
    union {
        std::uint16_t w;
        struct {
            std::uint8_t l;
            std::uint8_t h;
        } b;
    };
};

// Interrupt source enum matching MAME's internal ordering
enum class MameInterruptSource {
    TRAP = 0,
    NMI = 1,
    IRQ0 = 2,
    IRQ1 = 3,
    IRQ2 = 4,
    PRT0 = 5,
    PRT1 = 6,
    DMA0 = 7,
    DMA1 = 8,
    CSIO = 9,
    ASCI0 = 10,
    ASCI1 = 11,
    MAX = ASCI1,
};

// The Callbacks struct — kept from the existing API
struct Callbacks {
    std::function<std::uint8_t(std::uint32_t)> read_memory;
    std::function<void(std::uint32_t, std::uint8_t)> write_memory;
    std::function<std::uint8_t(std::uint16_t)> read_port;
    std::function<void(std::uint16_t, std::uint8_t)> write_port;
    std::function<void(std::uint16_t, std::uint8_t)> observe_logical_memory_read;
    std::function<void(std::uint16_t, std::uint8_t)> observe_logical_memory_write;
    std::function<void(std::uint16_t, std::uint8_t)> observe_internal_io_read;
    std::function<void(std::uint16_t, std::uint8_t)> observe_internal_io_write;
    std::function<void(std::string)> record_warning;
    std::function<void()> acknowledge_int1;
};

// Register snapshot — kept from the existing API
struct RegisterSnapshot {
    std::uint16_t af = 0;
    std::uint16_t bc = 0;
    std::uint16_t de = 0;
    std::uint16_t hl = 0;
    std::uint16_t sp = 0;
    std::uint16_t pc = 0;
    std::uint16_t af_alt = 0;
    std::uint16_t bc_alt = 0;
    std::uint16_t de_alt = 0;
    std::uint16_t hl_alt = 0;
    std::uint16_t ix = 0;
    std::uint16_t iy = 0;
    std::uint8_t r = 0;
    std::uint8_t i = 0;
    std::uint8_t il = 0;
    std::uint8_t itc = 0;
    std::uint8_t cbar = 0;
    std::uint8_t cbr = 0;
    std::uint8_t bbr = 0;
    std::uint8_t interrupt_mode = 0;
    bool iff1 = false;
    bool iff2 = false;
    std::uint8_t ei_delay = 0;
    bool halted = false;
    std::uint8_t tcr = 0;
    std::uint16_t tmdr0 = 0;
    std::uint16_t rldr0 = 0;
    std::uint16_t tmdr1 = 0;
    std::uint16_t rldr1 = 0;
};

enum class InterruptSource {
    int0,
    int1,
    prt0,
    prt1,
};

struct InterruptService {
    InterruptSource source;
    std::uint16_t handler_address = 0;
};

// Flag constants (matching MAME's z180.cpp definitions)
constexpr u8 CF = 0x01;
constexpr u8 NF = 0x02;
constexpr u8 PF = 0x04;
constexpr u8 VF = PF;
constexpr u8 XF = 0x08;
constexpr u8 HF = 0x10;
constexpr u8 YF = 0x20;
constexpr u8 ZF = 0x40;
constexpr u8 SF = 0x80;

// MAME-like SZ/SZP/SZHV_inc/SZHV_dec tables
extern u8 SZ[256];
extern u8 SZ_BIT[256];
extern u8 SZP[256];
extern u8 SZHV_inc[256];
extern u8 SZHV_dec[256];
extern u8* SZHVC_add;
extern u8* SZHVC_sub;

// MAME cycle count tables
extern const u8 cc_op[0x100];
extern const u8 cc_cb[0x100];
extern const u8 cc_ed[0x100];
extern const u8 cc_xy[0x100];
extern const u8 cc_xycb[0x100];
extern const u8 cc_ex[0x100];

// DMA status register bits
constexpr u8 DSTAT_DE1 = 0x80;
constexpr u8 DSTAT_DE0 = 0x40;
constexpr u8 DSTAT_DWE1 = 0x20;
constexpr u8 DSTAT_DWE0 = 0x10;
constexpr u8 DSTAT_DIE1 = 0x08;
constexpr u8 DSTAT_DIE0 = 0x04;
constexpr u8 DSTAT_DME = 0x01;
constexpr u8 DSTAT_MASK = 0xfd;

// DMA mode register bits
constexpr u8 DMODE_DM = 0x30;
constexpr u8 DMODE_SM = 0x0c;
constexpr u8 DMODE_MMOD = 0x02;
constexpr u8 DMODE_MASK = 0x3e;

// DMA/WAIT control register bits
constexpr u8 DCNTL_MWI1 = 0x80;
constexpr u8 DCNTL_MWI0 = 0x40;
constexpr u8 DCNTL_IWI1 = 0x20;
constexpr u8 DCNTL_IWI0 = 0x10;
constexpr u8 DCNTL_DMS1 = 0x08;
constexpr u8 DCNTL_DMS0 = 0x04;
constexpr u8 DCNTL_DIM1 = 0x02;
constexpr u8 DCNTL_DIM0 = 0x01;

// TCR bits
constexpr u8 TCR_TIF1 = 0x80;
constexpr u8 TCR_TIF0 = 0x40;
constexpr u8 TCR_TIE1 = 0x20;
constexpr u8 TCR_TIE0 = 0x10;
constexpr u8 TCR_TDE1 = 0x02;
constexpr u8 TCR_TDE0 = 0x01;

// ITC bits
constexpr u8 ITC_TRAP = 0x80;
constexpr u8 ITC_UFO = 0x40;
constexpr u8 ITC_ITE2 = 0x04;
constexpr u8 ITC_ITE1 = 0x02;
constexpr u8 ITC_ITE0 = 0x01;
constexpr u8 ITC_MASK = 0xc7;

// IL mask
constexpr u8 IL_MASK = 0xe0;

// CBAR bits
constexpr u8 CBAR_CA = 0xf0;
constexpr u8 CBAR_BA = 0x0f;

// OMCR bits
constexpr u8 OMCR_M1E = 0x80;
constexpr u8 OMCR_M1TE = 0x40;
constexpr u8 OMCR_IOC = 0x20;
constexpr u8 OMCR_MASK = 0xe0;

// RCR bits
constexpr u8 RCR_REFE = 0x80;
constexpr u8 RCR_REFW = 0x40;
constexpr u8 RCR_MASK = 0xc3;

// I/O line status flags
constexpr u32 IOL_CKA0 = 0x00000001;
constexpr u32 IOL_CKA1 = 0x00000002;
constexpr u32 IOL_CKS = 0x00000004;
constexpr u32 IOL_DREQ0 = 0x00000800;
constexpr u32 IOL_DREQ1 = 0x00001000;
constexpr u32 IOL_TEND0 = 0x00020000;
constexpr u32 IOL_TEND1 = 0x00040000;

// MAME prefix constants
constexpr int PREFIX_op = 0;
constexpr int PREFIX_cb = 1;
constexpr int PREFIX_dd = 2;
constexpr int PREFIX_ed = 3;
constexpr int PREFIX_fd = 4;
constexpr int PREFIX_xycb = 5;
constexpr int PREFIX_COUNT = PREFIX_xycb + 1;

constexpr int TABLE_op = 0;
constexpr int TABLE_cb = 1;
constexpr int TABLE_ed = 2;
constexpr int TABLE_xy = 3;
constexpr int TABLE_xycb = 4;
constexpr int TABLE_ex = 5;

// Integer assertion macros that MAME uses
#define ASSERT_LINE 1
#define CLEAR_LINE 0

// MAME's string_format for logging (simplified)
inline std::string string_format(const char* fmt, ...) {
    (void)fmt;
    return {};
}

}  // namespace vanguard8::third_party::z180

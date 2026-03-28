# Vanguard 8 — CPU Reference

## Hitachi HD64180

The HD64180 is a Z80-compatible CMOS microprocessor with an integrated MMU,
dual-channel DMA controller, two serial ports, two 16-bit timer/counters, and
a DRAM refresh circuit. It is fully binary-compatible with the Z80 at the
instruction level, with additional instructions and on-chip peripherals.

- **Clock:** 7.15909 MHz (master crystal 14.31818 MHz ÷ 2)
- **Process:** CMOS (low power vs. NMOS Z80)
- **Data bus:** 8-bit
- **Address bus:** 16-bit logical, 20-bit physical (via MMU)
- **Pipeline:** 2-stage — fetch overlaps with execute (~10–20% throughput gain over Z80 at same clock)

---

## Register Set

### Main Registers (Z80-compatible)

```
AF   — Accumulator + Flags
BC   — General purpose / counter
DE   — General purpose / pointer
HL   — General purpose / pointer

AF'  — Alternate accumulator + flags  (exchanged via EX AF,AF')
BC'  — Alternate BC                   (exchanged via EXX)
DE'  — Alternate DE                   (exchanged via EXX)
HL'  — Alternate HL                   (exchanged via EXX)

IX   — Index register X
IY   — Index register Y
SP   — Stack pointer
PC   — Program counter

I    — Interrupt vector base
R    — DRAM refresh counter
```

### Flags Register (F)

```
Bit 7   S    Sign
Bit 6   Z    Zero
Bit 5   —    (unused)
Bit 4   H    Half-carry
Bit 3   —    (unused)
Bit 2   P/V  Parity / Overflow
Bit 1   N    Add/Subtract
Bit 0   C    Carry
```

---

## HD64180 Additional Instructions

Beyond the standard Z80 instruction set, the HD64180 adds:

| Mnemonic          | Operation                                      | Notes                        |
|-------------------|------------------------------------------------|------------------------------|
| `MLT rr`          | HL ← H × L (unsigned 8×8 → 16-bit multiply)  | BC, DE, HL, SP supported     |
| `IN0 r,(n)`       | r ← internal I/O port n                       | Accesses HD64180 internal regs |
| `OUT0 (n),r`      | internal I/O port n ← r                       | Accesses HD64180 internal regs |
| `TST r`           | Flags ← A AND r (non-destructive AND test)    |                              |
| `TST n`           | Flags ← A AND n (immediate)                   |                              |
| `TSTIO n`         | Flags ← (C) AND n                             |                              |
| `OTIM`            | OUTI; if B≠0 then decrement HL, repeat        | Block output with increment  |
| `OTDM`            | OUTD; if B≠0 then decrement HL, repeat        | Block output with decrement  |
| `OTIMR`           | Repeat OTIM until B=0                         |                              |
| `OTDMR`           | Repeat OTDM until B=0                         |                              |
| `SLP`             | CPU halts until next interrupt                | Power-saving sleep mode      |

`TST` is a non-destructive AND test: it leaves `A` unchanged, derives `S`,
`Z`, and `P/V` from the masked result, sets `H`, and resets `N` and `C`.

---

## Memory Management Unit (MMU)

The HD64180 MMU divides the 64 KB logical address space into three regions and
maps each to a position in the 20-bit (1 MB) physical address space.

### MMU Regions

```
Region          Logical Range       Physical Base       Size
────────────────────────────────────────────────────────────────────
Common Area 0   0x0000–(CBAR.CA0)  Always 0x00000      Fixed
Bank Area       (CBAR.CA0)–(CBAR.CA1) (BBR << 12)     Switchable
Common Area 1   (CBAR.CA1)–0xFFFF  (CBR << 12)         Fixed
```

### Internal MMU Registers (accessed via OUT0/IN0)

| Register | Internal Addr | Reset | Description                                    |
|----------|---------------|-------|------------------------------------------------|
| CBAR     | 0x3A          | 0xFF  | CA0/CA1 boundary byte (upper=CA1, lower=CA0)  |
| CBR      | 0x38          | 0x00  | Common Area 1 physical base (bits 19:12)       |
| BBR      | 0x39          | 0x00  | Bank Area physical base (bits 19:12)           |

### Vanguard 8 MMU Configuration

```
CBAR = 0x48   → CA0 covers 0x0000–0x3FFF; CA1 covers 0x8000–0xFFFF
CBR  = 0xF0   → CA1 physical base = 0xF0000 (System SRAM)
BBR  = 0x04   → Bank Area physical base = 0x04000 at boot (ROM bank 0)
```

### ROM Bank Switching

```
Physical address of bank window = (BBR << 12) + (logical_addr - 0x4000)

Bank N maps to: BBR = (N + 1) × 4

Bank 0:  BBR = 0x04  → ROM bytes 0x04000–0x07FFF
Bank 1:  BBR = 0x08  → ROM bytes 0x08000–0x0BFFF
Bank 2:  BBR = 0x0C  → ROM bytes 0x0C000–0x0FFFF
...
Bank 58: BBR = 0xEC  → ROM bytes 0xEC000–0xEFFFF
```

BBR values 0xF0 and above are illegal — they map the bank window into the
physical SRAM region (0xF0000–0xF7FFF). The maximum legal BBR is 0xEC.

Maximum ROM: 59 switchable banks × 16 KB + 16 KB fixed = **960 KB**

---

## DMA Controller

The HD64180 includes two independent DMA channels. Each channel can transfer
data between memory and I/O, or memory to memory.

### DMA Channel Registers (Internal I/O)

Channel 0 is memory-to-memory; channel 1 is memory-to-I/O.

| Register | Addr        | Description                                  |
|----------|-------------|----------------------------------------------|
| SAR0L    | 0x20        | Ch0 source address, bits 7:0                 |
| SAR0H    | 0x21        | Ch0 source address, bits 15:8                |
| SAR0B    | 0x22        | Ch0 source address, bits 19:16               |
| DAR0L    | 0x23        | Ch0 destination address, bits 7:0            |
| DAR0H    | 0x24        | Ch0 destination address, bits 15:8           |
| DAR0B    | 0x25        | Ch0 destination address, bits 19:16          |
| BCR0L    | 0x26        | Ch0 byte count, bits 7:0                     |
| BCR0H    | 0x27        | Ch0 byte count, bits 15:8                    |
| MAR1L    | 0x28        | Ch1 memory address, bits 7:0                 |
| MAR1H    | 0x29        | Ch1 memory address, bits 15:8                |
| MAR1B    | 0x2A        | Ch1 memory address, bits 19:16               |
| IAR1L    | 0x2B        | Ch1 I/O address, bits 7:0                    |
| IAR1H    | 0x2C        | Ch1 I/O address, bit 8                       |
| (reserved) | 0x2D      | —                                            |
| BCR1L    | 0x2E        | Ch1 byte count, bits 7:0                     |
| BCR1H    | 0x2F        | Ch1 byte count, bits 15:8                    |
| DSTAT    | 0x30        | DMA status (DE1/DE0 enable; DME global en.)  |
| DMODE    | 0x31        | DMA mode (transfer type, address step)       |
| DCNTL    | 0x32        | DMA/wait control                             |

### DMA Modes

- **Memory → Memory:** Copy between two RAM regions
- **Memory → I/O:** Used for audio chip data feeds (not used for VRAM; VDP has its own port interface)
- **I/O → Memory:** Receive data from cartridge / serial

Note: VRAM access on the V9938 is handled through the VDP's own port-based VRAM
access mechanism, not through the HD64180 DMA. The HD64180 DMA is used for
main RAM operations and peripheral transfers.

---

## Timer/Counters

The HD64180 programmable reload timer (PRT) block contains two independent
16-bit timer channels, **PRT0** and **PRT1**. Each channel consists of:

- a 16-bit **Timer Data Register** (`TMDRn`) that counts down
- a 16-bit **Reload Register** (`RLDRn`) that is copied into `TMDRn` on timeout

The timer input clock is the **system clock divided by 20**. On Vanguard 8,
that is:

```
CPU clock (phi): 7.15909 MHz
PRT input clock: 357,954.5 Hz   (phi / 20)
PRT tick period: ~2.7937 us
```

### Timer Registers (Internal I/O)

| Register | Addr | Description |
|----------|------|-------------|
| TMDR0L   | 0x0C | PRT0 down counter, bits 7:0 |
| TMDR0H   | 0x0D | PRT0 down counter, bits 15:8 |
| RLDR0L   | 0x0E | PRT0 reload value, bits 7:0 |
| RLDR0H   | 0x0F | PRT0 reload value, bits 15:8 |
| TCR      | 0x10 | Timer control / interrupt status |
| TMDR1L   | 0x14 | PRT1 down counter, bits 7:0 |
| TMDR1H   | 0x15 | PRT1 down counter, bits 15:8 |
| RLDR1L   | 0x16 | PRT1 reload value, bits 7:0 |
| RLDR1H   | 0x17 | PRT1 reload value, bits 15:8 |

During reset, `TMDR0`, `TMDR1`, `RLDR0`, and `RLDR1` all initialize to
`0xFFFF`.

### Timer Control Register (TCR)

`TCR` is laid out as:

```
Bit 7   TIF1   Timer 1 interrupt flag (read only)
Bit 6   TIF0   Timer 0 interrupt flag (read only)
Bit 5   TIE1   Timer 1 interrupt enable
Bit 4   TIE0   Timer 0 interrupt enable
Bit 3   TOC1   Timer 1 output control
Bit 2   TOC0   Timer 1 output control
Bit 1   TDE1   Timer 1 down-count enable
Bit 0   TDE0   Timer 0 down-count enable
```

Semantics:

- `TDEn = 1` starts down counting for channel `n`.
- `TDEn = 0` stops down counting, allowing `TMDRn` to be read or written
  freely.
- When `TMDRn` decrements to `0x0000`, `TIFn` becomes `1`.
- If `TIEn = 1`, `TIFn = 1` requests that channel's internal timer interrupt.
- After reaching `0x0000`, `TMDRn` automatically reloads from `RLDRn`.
- `TIFn` is cleared only when `TCR` is read and then either byte of the
  corresponding `TMDRn` is read.

### Timer Read/Write Rules

- `TMDRn` decrements continuously while `TDEn = 1`.
- To obtain a coherent 16-bit snapshot without stopping the timer, software
  must read `TMDRnL` first, then `TMDRnH`.
- Writing `TMDRn` is only defined while `TDEn = 0`.
- `RLDRn` may be updated by software, but software must avoid changing it
  across an active timeout boundary; the safe path is to stop the channel
  before rewriting timer state.

### Vanguard 8 Usage

| Channel | Function on Vanguard 8                                 |
|---------|--------------------------------------------------------|
| Timer 0 | Available for game use (e.g., raster effect timing)    |
| Timer 1 | Audio sequencer tick (supplements YM2151 IRQ)          |

The `A18/TOUT` waveform-output feature of PRT1 is part of the HD64180 itself,
but no Vanguard 8 subsystem depends on it. The software-visible contract for
the emulator is the timer register behavior and interrupt generation.

---

## Interrupt System

| Line  | Description                                                       |
|-------|-------------------------------------------------------------------|
| INT0  | VDP-A /INT (V-blank and H-blank); YM2151 timer IRQ               |
| INT1  | MSM5205 /VCLK (ADPCM sample clock — dedicated to audio feeding)  |
| INT2  | Available for cartridge expansion                                 |

There is no NMI source. VDP-A's single /INT pin connects to CPU /INT0.

### INT0 — Follows CPU Interrupt Mode

Vanguard 8 firmware runs in **IM1**. In IM1, INT0 calls address **0x0038**.
The handler reads VDP-A status registers to distinguish V-blank from H-blank:
- **S#0 bit 7** (VB): V-blank flag, cleared by reading S#0
- **S#1 bit 0** (FH): H-blank flag, cleared by reading S#1 (select via R#15)

### INT1/INT2 — Always Vectored, Mode-Independent

**INT1 and INT2 do not follow the IM0/1/2 setting.** The HD64180 uses a dedicated
vectored response for these lines regardless of the current interrupt mode.
The same vectored response mechanism is also used by on-chip peripheral
interrupts such as the programmable reload timers.

The vector pointer is formed from two HD64180 internal registers:

| Register | Internal Addr | Description                                          |
|----------|---------------|------------------------------------------------------|
| IL       | 0x33          | Interrupt Vector Low register; bits 7:5 select 32-byte subtable |
| ITC      | 0x34          | Interrupt/Trap Control; bits ITE2/ITE1/ITE0 enable INT2/1/0 |

The low vector byte is not taken directly from `IL`. Instead:

```
vector_low = (IL & 0xE0) | fixed_code
```

Where `fixed_code` is chosen by the interrupt source:

| Source | Fixed low-byte code |
|--------|---------------------|
| INT1   | `0x00` |
| INT2   | `0x02` |
| PRT0   | `0x04` |
| PRT1   | `0x06` |

The processor reads a 2-byte handler address (little-endian) from the computed
vector pointer. Startup code must configure `I`, `IL`, `ITC`, and populate the
vector table before enabling INT1 or on-chip timer interrupts.

Example:

```
I  = 0x80
IL = 0xE0

INT1 vector word = 0x80E0–0x80E1
INT2 vector word = 0x80E2–0x80E3
PRT0 vector word = 0x80E4–0x80E5
PRT1 vector word = 0x80E6–0x80E7
```

### Internal Timer Interrupts

PRT0 and PRT1 are **internal vectored interrupts**, not external `INT0`
sources. Their requests:

- are globally masked by the CPU interrupt enable state (`IEF1`)
- are individually controlled by `TIE0` and `TIE1` in `TCR`
- use the same `I`/`IL` vector-table mechanism as `INT1` and `INT2`
- are independent of the VDP-A / YM2151 wire-OR on external `INT0`

On Vanguard 8:

- PRT0 is available as a software timer interrupt source.
- PRT1 is typically used as an additional audio/game sequencer tick, but this
  is a software convention rather than a separate hardware wiring path.

---

## Electrical / Timing

- Supply voltage: 5 V (CMOS)
- All inputs TTL-compatible
- Bus cycle: 3 CPU clocks minimum (wait states insertable)
- DRAM refresh: On-chip, configurable; refreshes system DRAM automatically

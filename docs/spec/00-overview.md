# Vanguard 8 — System Overview

## Design Philosophy

The Vanguard 8 uses no custom silicon. Every component was a commercially
available off-the-shelf part by 1985–1986. This eliminates ASIC non-recurring
engineering costs ($1M–$5M for a custom VDP) and allows any contract electronics
manufacturer with access to standard parts to build the board.

The system is positioned as a premium home console: clearly superior to the NES
and Sega Master System, but not attempting to replicate the exotic custom hardware
of arcade boards (no sprite scaling/rotation, no 3+ background layers).

---

## Complete Component List

| Subsystem         | Part              | Manufacturer | Available | Qty |
|-------------------|-------------------|--------------|-----------|-----|
| CPU               | HD64180           | Hitachi      | 1985      | 1   |
| Main RAM          | 62256 (32KB SRAM) | Various      | 1984      | 1   |
| VRAM              | 4164 (64Kbit DRAM)| Various      | 1980      | 16  |
| VDP-A             | V9938             | Yamaha       | 1985      | 1   |
| VDP-B             | V9938             | Yamaha       | 1985      | 1   |
| Video compositing | 74LS257           | Various      | Standard  | 4   |
| FM audio          | YM2151            | Yamaha       | 1983      | 1   |
| PSG audio         | AY-3-8910         | General Inst.| 1978      | 1   |
| ADPCM audio       | MSM5205           | OKI          | 1984      | 1   |
| Clock/glue        | 74LS-series TTL   | Various      | Standard  | —   |
| Master oscillator | 14.31818 MHz XTAL | Various      | Standard  | 1   |

---

## Clock Architecture

Master crystal: **14.31818 MHz** (2× NTSC color subcarrier frequency)

| Signal           | Derivation          | Frequency     |
|------------------|---------------------|---------------|
| CPU clock        | Master ÷ 2          | 7.15909 MHz   |
| NTSC subcarrier  | Master ÷ 4          | 3.57954 MHz   |
| H-sync           | Derived from pixel clock | ~15.734 kHz |
| V-sync (NTSC)    | H-sync ÷ 262 (interlaced) | 59.94 Hz  |

Both V9938 chips are driven from the same master clock, ensuring they remain
frame-perfectly synchronized without any external sync logic.

---

## Memory Map

The HD64180 uses an internal MMU that translates 16-bit logical addresses to
20-bit physical addresses, enabling access to up to 1 MB of physical memory from
the 64 KB logical address space.

### Logical Address Space (CPU viewpoint)

```
0x0000 – 0x3FFF   ROM Common Area 0   16 KB   Fixed, always maps to first 16 KB of cartridge
0x4000 – 0x7FFF   ROM Bank Window     16 KB   Switchable via HD64180 BBR register
0x8000 – 0xFFFF   System SRAM         32 KB   Fixed (HD64180 Common Area 1)
```

### Physical Address Space

```
0x00000 – 0x03FFF   Cartridge ROM, fixed region (16 KB)
0x04000 – 0xEFFFF   Cartridge ROM, banked region (944 KB, 59 × 16 KB pages)
0xF0000 – 0xF7FFF   System SRAM (32 KB)
```

### HD64180 MMU Register Setup (at boot)

| Register | Address | Value | Effect                                      |
|----------|---------|-------|---------------------------------------------|
| CBAR     | 0x3A    | 0x48  | CA0 ends at 0x3FFF; CA1 begins at 0x8000   |
| CBR      | 0x38    | 0xF0  | CA1 physical base = 0xF0000 (SRAM)         |
| BBR      | 0x39    | 0x04  | Bank window physical base = 0x04000 (boot) |

To switch to ROM bank N (0-based), write `(N + 1) × 4` to BBR using `OUT0`.
Bank 0 → BBR = 0x04, Bank 1 → BBR = 0x08, … Bank 58 → BBR = 0xEC.
59 switchable banks × 16 KB + 16 KB fixed = **960 KB maximum cartridge ROM**.
BBR values 0xF0 and above are illegal (map into SRAM region).

---

## I/O Port Map

All peripheral access is via Z80-style I/O instructions (`IN`/`OUT`). The
HD64180's extended I/O space (accessed via `IN0`/`OUT0`) is reserved for internal
CPU registers.

```
Port    Dir     Device          Function
────────────────────────────────────────────────────────
0x00    R       Controller 1    Button state (active low)
0x01    R       Controller 2    Button state (active low)

0x40    W       YM2151          Address register
0x40    R       YM2151          Status register (busy/IRQ flags)
0x41    W       YM2151          Data register

0x50    W       AY-3-8910       Address latch
0x51    W       AY-3-8910       Data write
0x51    R       AY-3-8910       Data read

0x60    W       MSM5205         Control (sample rate select, reset)
0x61    W       MSM5205         Data (4-bit ADPCM nibble)

0x80    R/W     VDP-A           VRAM data read/write
0x81    W       VDP-A           VRAM address / register command
0x81    R       VDP-A           Status register
0x82    W       VDP-A           Palette data
0x83    W       VDP-A           Register indirect write

0x84    R/W     VDP-B           VRAM data read/write
0x85    W       VDP-B           VRAM address / register command
0x85    R       VDP-B           Status register
0x86    W       VDP-B           Palette data
0x87    W       VDP-B           Register indirect write
```

---

## Interrupt Map

VDP-A's single /INT pin connects directly to the CPU /INT0 line. There is no
NMI source; all VDP interrupts are maskable via INT0. VDP-B's /INT is not
connected to the CPU.

| Source            | Line | Vector          | Typical Use                                      |
|-------------------|------|-----------------|--------------------------------------------------|
| VDP-A V-blank     | INT0 | 0x0038 (IM1)    | Sprite/palette updates, game logic tick          |
| VDP-A H-blank     | INT0 | 0x0038 (IM1)    | Per-line scroll update (H-scroll: see note)      |
| YM2151 Timer IRQ  | INT0 | 0x0038 (IM1)    | Audio sequencer tick                             |
| MSM5205 /VCLK     | INT1 | Vectored (I/IL) | Feed next ADPCM nibble (see INT1 note)           |
| HD64180 PRT0      | Internal | Vectored (I/IL + PRT0 code) | General software timer                     |
| HD64180 PRT1      | Internal | Vectored (I/IL + PRT1 code) | Optional audio / game sequencer tick       |

**INT0:** INT0 follows the CPU interrupt mode. Vanguard 8 firmware runs in **IM1**;
INT0 always calls **0x0038**.

**INT1/INT2/internal HD64180 peripheral interrupts — always vectored,
mode-independent:** The HD64180 handles INT1, INT2, and on-chip peripheral
interrupts such as `PRT0` and `PRT1` using a dedicated vectored response that
is **independent of the IM0/1/2 setting**. This is not IM2 — it is a separate
mechanism specific to these sources. The processor forms the vector pointer as:

```
Vector pointer high byte = I register
Vector pointer low byte  = (IL & 0xE0) | fixed_code
```

Fixed low-byte codes used by Vanguard 8-visible sources:

```
INT1  = 0x00
INT2  = 0x02
PRT0  = 0x04
PRT1  = 0x06
```

The processor reads 2 bytes (little-endian) from that pointer address to get the
actual handler address. INT1 must also be enabled via the **ITC** register
(internal addr `0x34`), bit 1 (`ITE1`). `PRT0` and `PRT1` are enabled
independently through `TCR.TIE0` and `TCR.TIE1`.

Vanguard 8 startup configures INT1 as follows (see `docs/spec/04-io.md` for
register setup code):

```
I   = 0x80   → vector table in SRAM
IL  = 0xE0   → INT1/INT2/internal vectors occupy SRAM 0x80E0–0x80FF
ITC = 0x02   → ITE1 = 1 (INT1 enabled)
```

With that setup:

```
INT1 vector word = 0x80E0–0x80E1
INT2 vector word = 0x80E2–0x80E3
PRT0 vector word = 0x80E4–0x80E5
PRT1 vector word = 0x80E6–0x80E7
```

The 2-byte ADPCM handler address is stored at SRAM `0x80E0–0x80E1`.

**H-blank scroll note:** The H-blank interrupt mechanism (VDP-A S#1 FH flag) is
well-specified. However, R#26/R#27 horizontal scroll register behavior in Graphic 4
mode requires verification against the V9938 Technical Data Book before relying on
per-line horizontal parallax. Vertical scroll via R#23 is fully specified and reliable.

The INT0 handler identifies its source by reading VDP-A status registers (S#0
for V-blank, S#1 for H-blank) and the YM2151 status port. The MSM5205 /VCLK
signal connects to the CPU /INT1 line, giving ADPCM sample feeding a dedicated
interrupt with no sharing. The HD64180 PRT timers are separate internal
vectored interrupt sources and do not share the external `INT0` wire-OR line.

---

## Cartridge

- Connector: 60-pin edge connector
- ROM: Up to 960 KB (mask ROM or EEPROM)
- Optional battery-backed SRAM: 8–32 KB, mapped into the bank window
- ROM and SRAM select decoded from upper address lines + /CE signals on cartridge PCB
- No mapper chip required — the HD64180 MMU handles all banking

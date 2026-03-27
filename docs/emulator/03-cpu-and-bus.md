# Vanguard 8 Emulator — CPU, MMU, and Bus

## HD64180 / Z180 Core

The CPU is currently emulated using a **standalone milestone-2 extraction**
derived from MAME's Z180 core in `src/devices/cpu/z180/`. The extracted code
is vendored in `third_party/z180/` and is intentionally limited to the reset,
MMU, interrupt, and boot-test opcode surface needed by milestone 2.

The Z180 and HD64180 are widely described as the same silicon (Zilog
distributed the HD64180 design under the Z180 name), but this has not been
independently audited for this project — see the compatibility note below
before treating it as settled.

### Z180 / HD64180 Compatibility

The claim that "Z180 and HD64180 are the same silicon" is a widely-cited
industry fact (Zilog licensed the design from Hitachi and distributed it under
the Z180 name), but it has not been independently audited against the MAME
core's implementation for this project. This is an **implementation assumption,
not a verified claim**.

Milestone-11 compatibility notes are tracked in
`docs/emulator/08-compatibility-audit.md`.

Before relying on any HD64180-specific behavior (especially `IN0`/`OUT0`
internal register access, the INT1/INT2 vectored mechanism, and `MLT`), the
MAME Z180 core's behavior for those features should be cross-checked against
the Hitachi HD64180 Product Specification and the Zilog Z180 Product
Specification. Discrepancies, if any, should be logged as spec conflicts and
resolved before that feature is tested against real game code.

### What the extracted core provides today

- Reset behavior pinned to the repo's documented HD64180 boot defaults
- Internal MMU (`CBAR`, `CBR`, `BBR`) with Vanguard 8 physical-address
  translation
- `OUT0`/`IN0` access for the internal registers required by milestone 2
- INT0 IM1 service and INT1 vectored service via `I` and `IL`
- The opcode subset needed by the milestone-2 boot/MMU/interrupt tests

### What is still deferred

- Full Z80/Z180 opcode coverage
- DMA execution
- Timer/counter execution
- ASCI/CSIO integration
- Full MAME device-framework integration

### Adapter layer

`third_party/z180/z180_core.cpp` exposes a standalone callback surface and
`src/core/cpu/z180_adapter.cpp` binds it to the emulator `Bus`:

```cpp
// Memory read/write — called by the extracted Z180 core
uint8_t z180_read(uint32_t physical_addr);
void    z180_write(uint32_t physical_addr, uint8_t value);

// I/O read/write — called for IN/OUT instructions
uint8_t z180_io_read(uint16_t port);
void    z180_io_write(uint16_t port, uint8_t value);

// Warning and interrupt acknowledge hooks
void    z180_warn(std::string message);
void    z180_int1_ack();
```

The extraction is **adapted** rather than unmodified. The repository spec
remains authoritative, so reset-time MMU behavior and the INT1 vector-pointer
rule follow `docs/spec/01-cpu.md` even where MAME internals differ.

---

## MMU — Physical Address Translation

The HD64180 MMU translates every 16-bit logical address to a 20-bit physical
address before the memory access reaches the bus. The adapter exposes the
post-translation physical address to the bus, not the logical address. This
means the bus never needs to know about the MMU.

### Translation rules (from `docs/spec/01-cpu.md`)

```
CBAR = 0x48  → CA0: 0x0000–0x3FFF; Bank Area: 0x4000–0x7FFF; CA1: 0x8000–0xFFFF

Logical 0x0000–0x3FFF (CA0):
    Physical = logical                         // always maps to 0x00000–0x03FFF

Logical 0x4000–0x7FFF (Bank Area):
    Physical = (BBR << 12) + (logical - 0x4000)

Logical 0x8000–0xFFFF (CA1):
    Physical = (CBR << 12) + (logical - 0x8000)
               = 0xF0000 + (logical - 0x8000)   // at boot
```

### Illegal BBR detection

The bus checks every BBR write (via `OUT0` to internal address 0x39). If the
written value is 0xF0 or above, the emulator logs a warning:

```
[WARN] Illegal BBR write: 0xF2 at PC=0x1234 — maps into SRAM region
```

The write is still applied (the hardware does not prevent it), but the log
flag records the violation. Tests verify that BBR ≥ 0xF0 does in fact alias
into the SRAM region as the spec requires.

---

## Bus — Memory and I/O Routing

The `Bus` class is the sole routing layer. All components register with the
bus at construction time. The CPU core calls the bus (via the adapter) for
every memory access and I/O operation.

### Memory routing (physical address)

```
Physical 0x00000–0x03FFF → CartridgeSlot (fixed region, first 16 KB)
Physical 0x04000–0xEFFFF → CartridgeSlot (banked region, 944 KB)
Physical 0xF0000–0xF7FFF → SRAM (32 KB)
Physical 0xF8000–0xFFFFF → Open bus (unmapped; returns 0xFF, logs warning)
```

### I/O routing (port number)

```
0x00        → ControllerPort[0] read
0x01        → ControllerPort[1] read
0x40        → YM2151 address write / status read
0x41        → YM2151 data write
0x50        → AY-3-8910 address latch write
0x51        → AY-3-8910 data read/write
0x60        → MSM5205 control write
0x61        → MSM5205 data write
0x80        → VDP-A VRAM data read/write
0x81 (W)    → VDP-A address/register command
0x81 (R)    → VDP-A status register
0x82        → VDP-A palette write
0x83        → VDP-A register indirect write
0x84        → VDP-B VRAM data read/write
0x85 (W)    → VDP-B address/register command
0x85 (R)    → VDP-B status register
0x86        → VDP-B palette write
0x87        → VDP-B register indirect write
(other)     → Open bus (returns 0xFF, logs warning)
```

The `IN0`/`OUT0` instructions access the HD64180 internal register space
(addresses `0x00–0x3F` in the internal space). These are handled entirely by
the extracted Z180 core and never reach the external bus.

---

## Interrupt Wiring

The bus owns the interrupt state that connects components to the CPU.

### INT0 (open-collector wire-OR)

```cpp
// Any asserting source drives INT0 low. Bus holds the union.
struct Int0State {
    bool vdp_a_vblank = false;
    bool vdp_a_hblank = false;
    bool ym2151_timer = false;
};

bool int0_asserted() const {
    return vdp_a_vblank || vdp_a_hblank || ym2151_timer;
}
```

The CPU core polls `int0_asserted()` before each instruction fetch.
Each source is cleared by the game's INT0 handler reading the appropriate
status register:
- VDP-A S#0 read (port 0x81) clears `vdp_a_vblank`
- VDP-A S#1 read clears `vdp_a_hblank`
- YM2151 status read (port 0x40) clears `ym2151_timer`

### INT1 (MSM5205 /VCLK)

```cpp
bool int1_asserted = false;
```

Set by `msm5205.on_vclk()`. Cleared by the CPU core automatically when the
INT1 vectored response is taken (the acknowledge cycle clears the pending
flag). The CPU only responds to INT1 if ITE1 is set in the ITC register
(enforced by the Z180 core).

### No NMI

There is no NMI source on the Vanguard 8. The Z180 core's NMI input is held
inactive unconditionally.

---

## DMA Controller

The HD64180 has two internal DMA channels in hardware. The current
milestone-2 extraction does **not** execute DMA transfers yet; DMA remains a
later milestone. The callback shapes below are the planned integration model
once the broader extraction is expanded.

### Planned channel 0 — Memory to Memory

Channel 0 reads from a 20-bit physical source address and writes to a 20-bit
physical destination address. Both sides go through the adapter's memory
callbacks:

```cpp
uint8_t z180_read(uint32_t physical_addr);    // source read
void    z180_write(uint32_t physical_addr, uint8_t value);  // dest write
```

The bus routes these physical addresses to CartridgeSlot or SRAM exactly as
it would for CPU-initiated memory accesses.

### Planned channel 1 — Memory to I/O

Channel 1 reads from a 20-bit physical memory address and writes to an I/O
port specified by the `IAR1L`/`IAR1H` registers (`docs/spec/01-cpu.md`,
internal addresses 0x2B–0x2C). The memory side uses `z180_read`; the I/O
side uses the adapter's I/O write callback, which routes through the bus port
dispatch:

```cpp
// DMA channel 1 I/O destination — same path as a CPU OUT instruction
void z180_io_write(uint16_t port, uint8_t value);
```

This means a DMA channel 1 transfer to port 0x41 (YM2151 data) is handled
identically to the CPU executing `OUT (0x41), A`. The bus's I/O routing table
is the single dispatch point for both CPU-initiated and DMA-initiated port
writes.

**VRAM note:** VDP VRAM is not accessible via DMA. VDP VRAM access goes
through the VDP's port interface (ports 0x80–0x87). The HD64180 DMA channel 1
could in principle target these ports, but the Vanguard 8 hardware spec does
not describe any use of DMA for VDP access — the VDP's own command engine
(HMMM, HMMC, etc.) is the intended mechanism for bulk VRAM transfers.

---

## Timer/Counters

The HD64180 has two 16-bit programmable reload timers (PRT0 and PRT1). Each
channel has:

- `TMDRn` — 16-bit down counter
- `RLDRn` — 16-bit reload value
- `TCR` bits for enable, interrupt enable, and interrupt flag state

Relevant internal I/O addresses:

```text
TMDR0L 0x0C   TMDR0H 0x0D   RLDR0L 0x0E   RLDR0H 0x0F
TCR    0x10
TMDR1L 0x14   TMDR1H 0x15   RLDR1L 0x16   RLDR1H 0x17
```

The input clock for both channels is the CPU/system clock divided by 20.
At Vanguard 8's 7.15909 MHz CPU clock, the PRT tick rate is 357,954.5 Hz.

Behavioral rules locked by `docs/spec/01-cpu.md`:

- `TDEn = 1` starts channel `n` down counting
- `TDEn = 0` stops channel `n`, allowing free `TMDRn` writes
- `TMDRn` decrements once every 20 CPU clocks
- when `TMDRn` reaches `0x0000`, `TIFn` is set and `TMDRn` reloads from `RLDRn`
- `TIFn` is cleared only after reading `TCR` and then reading either byte of
  the corresponding `TMDRn`
- coherent live reads use `TMDRnL` followed by `TMDRnH`

### Timer Interrupt Routing

PRT0 and PRT1 are **internal vectored interrupts**, not external `INT0`
sources. They do not use the VDP-A/YM2151 open-collector wire-OR line.

Their vector table entries use the HD64180 internal interrupt scheme:

```text
vector_low = (IL & 0xE0) | fixed_code

INT1 = 0x00
INT2 = 0x02
PRT0 = 0x04
PRT1 = 0x06
```

So with `I = 0x80` and `IL = 0xE0`, the relevant vector words are:

```text
INT1 -> 0x80E0
INT2 -> 0x80E2
PRT0 -> 0x80E4
PRT1 -> 0x80E6
```

This matters for the adapter shape: timer interrupt requests are generated
inside the CPU/peripheral model, not on the `Bus::int0_asserted()` or
`Bus::int1_asserted()` paths. The bus remains responsible only for external
interrupt contributors such as VDP-A, YM2151, and MSM5205.

The `docs/spec/01-cpu.md` note that Timer 1 is conventionally used as an audio
sequencer tick supplementing the YM2151 IRQ is a software convention, not a
hardware routing constraint.

---

## Cartridge SRAM (Battery-Backed)

`docs/spec/00-overview.md` mentions optional battery-backed SRAM (8–32 KB)
mapped into the cartridge bank window. The banking and arbitration rules for
this feature are not yet fully specified in the repo spec.

**v1 decision: battery-backed cartridge SRAM is not supported.** The
`CartridgeSlot` class treats the entire bank window as read-only ROM. Any
write to the physical address range 0x04000–0xEFFFF is logged as a warning
and discarded. This will be revisited when the spec defines the SRAM mapping
in sufficient detail.

---

## Boot Sequence Emulation

On reset, the extracted core initializes registers to the Vanguard 8 documented
boot values used by the milestone-2 tests:

```
PC   = 0x0000
CBAR = 0xFF   (CA0 covers full 64 KB — flat map before boot ROM runs)
CBR  = 0x00
BBR  = 0x00
```

The cartridge's fixed ROM at physical 0x00000 immediately runs the Vanguard 8
startup sequence (from `docs/spec/04-io.md`), which reconfigures:

```
CBAR = 0x48, CBR = 0xF0, BBR = 0x04
I    = 0x80, IL = 0xE0, ITC = 0x02
```

The emulator does not pre-configure these registers on the CPU's behalf. They
are set by the ROM code executing through the CPU core, exactly as hardware.
This means any ROM that skips the startup sequence will run in the wrong MMU
configuration — correct behavior.

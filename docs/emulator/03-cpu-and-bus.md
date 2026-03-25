# Vanguard 8 Emulator — CPU, MMU, and Bus

## HD64180 / Z180 Core

The CPU is emulated using the **MAME Z180 core** extracted from
`src/devices/cpu/z180/`. The Z180 and HD64180 are widely described as the same
silicon (Zilog distributed the HD64180 design under the Z180 name), but this
has not been independently audited for this project — see the compatibility
note below before treating it as settled. The core covers the complete Z80
instruction set plus HD64180 extensions and internal peripherals.

### Z180 / HD64180 Compatibility

The claim that "Z180 and HD64180 are the same silicon" is a widely-cited
industry fact (Zilog licensed the design from Hitachi and distributed it under
the Z180 name), but it has not been independently audited against the MAME
core's implementation for this project. This is an **implementation assumption,
not a verified claim**.

Before relying on any HD64180-specific behavior (especially `IN0`/`OUT0`
internal register access, the INT1/INT2 vectored mechanism, and `MLT`), the
MAME Z180 core's behavior for those features should be cross-checked against
the Hitachi HD64180 Product Specification and the Zilog Z180 Product
Specification. Discrepancies, if any, should be logged as spec conflicts and
resolved before that feature is tested against real game code.

### What the extracted core provides

- Full Z80 instruction set with correct T-state counts
- HD64180 additional instructions: `MLT`, `IN0`, `OUT0`, `TST`, `TSTIO`,
  `OTIM`, `OTDM`, `OTIMR`, `OTDMR`, `SLP`
- Internal MMU (CBAR, CBR, BBR) with 16→20-bit physical address translation
- Dual-channel DMA controller (channels 0 and 1)
- Two 16-bit timer/counters with interrupt output
- INT0 response (IM1: call 0x0038)
- INT1 / INT2 mode-independent vectored response (I/IL registers)
- DRAM refresh counter (R register)

### Adapter layer

MAME's core uses its own device framework for memory and I/O callbacks.
`src/core/cpu/z180_adapter.cpp` replaces these with thin wrappers that
delegate to the emulator's `Bus`:

```cpp
// Memory read/write — called by the Z180 core
uint8_t z180_read(uint32_t physical_addr);
void    z180_write(uint32_t physical_addr, uint8_t value);

// I/O read/write — called for IN/OUT instructions
uint8_t z180_io_read(uint16_t port);
void    z180_io_write(uint16_t port, uint8_t value);

// Interrupt acknowledge — called when CPU services INT1/INT2
uint8_t z180_int1_ack();
uint8_t z180_int2_ack();
```

The adapter does **not** modify the Z180 core files. The core is included
unmodified; only the callback bindings change.

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
(addresses 0x00–0x3F in the internal space). These are handled entirely by
the Z180 core and never reach the external bus.

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

The HD64180 has two internal DMA channels managed by the Z180 core. The two
channels have different transfer types and use different adapter callbacks.

### Channel 0 — Memory to Memory

Channel 0 reads from a 20-bit physical source address and writes to a 20-bit
physical destination address. Both sides go through the adapter's memory
callbacks:

```cpp
uint8_t z180_read(uint32_t physical_addr);    // source read
void    z180_write(uint32_t physical_addr, uint8_t value);  // dest write
```

The bus routes these physical addresses to CartridgeSlot or SRAM exactly as
it would for CPU-initiated memory accesses.

### Channel 1 — Memory to I/O

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

The HD64180 has two 16-bit programmable reload timers (PRT0 and PRT1), both
managed entirely inside the Z180 core. Their overflow interrupts are internal
to the HD64180 interrupt system — they do not surface on the external INT0,
INT1, or INT2 pins. The Z180 core routes them through its own internal
interrupt arbitration and the emulator does not need to wire them to the bus
externally.

From the emulator's perspective: the Z180 core handles timer ticks, overflow
detection, and interrupt dispatch without any external wiring. The adapter
does not expose timer state to the bus.

The `docs/spec/01-cpu.md` notes Timer 1 is conventionally used as an audio
sequencer tick supplementing the YM2151 IRQ. This is a game-level convention,
not a hardware routing constraint. The timer fires its internal interrupt; the
game's handler (called at 0x0038 in IM1 like all maskable interrupts) does the
audio sequencing work.

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

On reset, the Z180 core initializes registers to hardware reset values:

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
I    = 0x80, IL = 0xF8, ITC = 0x02
```

The emulator does not pre-configure these registers on the CPU's behalf. They
are set by the ROM code executing through the CPU core, exactly as hardware.
This means any ROM that skips the startup sequence will run in the wrong MMU
configuration — correct behavior.

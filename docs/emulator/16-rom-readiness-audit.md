# Vanguard 8 Emulator — ROM-Readiness Audit

Audit date: 2026-04-26
Scope: gaps in the emulator's hardware-feature coverage that would block
a game ROM (handwritten asm or agentically generated) from running
correctly on the Vanguard 8 emulator. GUI / desktop polish is explicitly
out of scope.

This is a fresh audit, not a rerun of `08-compatibility-audit.md` or
`15-second-pass-audit.md`. Findings that those notes already opened
remain open unless this audit confirms otherwise; this document focuses
on what an agentically authored ROM is most likely to trip over today.

---

## TL;DR

The emulator is solid in the areas the spec already pins down hard:
boot MMU, ROM banking, V-blank/H-blank wiring, INT0 dispatch, INT1 ADPCM
feed, save-state determinism, and the Graphic 4 + Mode 2 sprite render
path. Compositing (VDP-A over VDP-B with index-0 transparency) is wired
correctly, both VDPs run in lockstep, and replay regression coverage is
real.

The single biggest blocker for ROM authoring is the **CPU
instruction-set surface**. The timed Z180 dispatch only covers the
opcodes that have already been demanded by one specific ROM (PacManV8).
A standard Z80/HD64180 toolchain — SDCC, z88dk, hand-written `RST n`
trampolines, anything that uses `IX/IY`, block transfers, conditional
`CALL`, or `LDIR` — will hit `Unsupported timed Z180 opcode …` within a
few hundred instructions. This is the single most important gap to
close before agentic ROM authoring becomes practical.

The video subsystem covers Graphic 1–4 and Graphic 6 backgrounds, both
sprite modes, and the Graphic 4 command engine. Text 1/2, Graphic 5,
Graphic 7, and the command engine pixel-packing for any mode other than
Graphic 4 are still missing. R#26/R#27 horizontal scroll is still
deferred.

The audio subsystem is in surprisingly good shape: YM2151, AY-3-8910,
and MSM5205 are all implemented against vendored cores and properly
clocked, the INT0 wire-OR with YM2151 is wired, and INT1 drives the
ADPCM nibble feed. There are no spec-level gaps here that would block
a ROM.

The I/O subsystem (controllers) is complete for the 2-controller
specified surface. ROM banking via `OUT0 (0x39),A` is correct.

---

## Severity Key

| Level    | Meaning                                                           |
|----------|-------------------------------------------------------------------|
| Blocker  | A ROM written to spec will not run, or will hit a hard error      |
| High     | A ROM written to spec will run but produce wrong output           |
| Medium   | Spec-defined feature absent; only software that uses it is hit    |
| Low      | Documented limitation, niche or already deferred by the spec      |

---

## 1. CPU / HD64180 Instruction Coverage — **Blocker**

### Where the gap lives

The timed CPU path lives in two places:

- `third_party/z180/z180_core.cpp`, primary opcode table starting at
  `initialize_tables()` (~line 403). Hand-populated dispatch — every
  opcode that exists is explicitly assigned to a method.
- `src/core/cpu/z180_adapter.cpp::current_instruction_tstates()`,
  `cb_instruction_tstates()`, and `ed_instruction_tstates()` (~line 335
  onward). These return T-state counts; if an opcode is not listed they
  throw `std::runtime_error("Unsupported timed Z180 opcode 0x… at PC
  0x…")`.

Anything not in **both** the dispatch table and the timing table either
silently runs as `op_unimplemented` (primary table gap) or crashes the
emulator (timing table gap).

### What is implemented today

Approximate coverage from `initialize_tables()` and the timing
dispatcher:

- `LD r,r'` register-to-register block (`0x40–0x7F` minus `HALT`)
- ALU on `A` with register/`(HL)`/immediate for `ADD`, `ADC`, `SUB`,
  `SBC`, `AND`, `XOR`, `OR`, `CP` — only the base register column,
  `0x80–0xBF` and matching immediates
- 16-bit pair loads `LD rr,nn`, `INC rr`, `DEC rr`, `ADD HL,ss` for the
  base pairs (no IX/IY)
- 8-bit `INC r` / `DEC r` and the standard immediate `LD r,n`
- `JR e` / `JR cc,e`, `JP nn` / `JP cc,nn` for `Z`, `NZ`
- `CALL nn` (unconditional only), `RET`, `RET cc` for `Z`, `NZ`, `C`,
  `NC`
- `PUSH/POP` for `BC`, `DE`, `HL`, `AF`
- `IN A,(n)`, `OUT (n),A`
- `EX DE,HL`, `CPL`, `SCF`, `DI`, `EI`, `NOP`, `HALT`, `DJNZ`,
  `RRCA` (`0x0F`)
- `LD (nn),HL` (`0x22`), `LD HL,(nn)` (`0x2A`), `LD (nn),A` (`0x32`),
  `LD A,(nn)` (`0x3A`), `LD SP,nn` (`0x31`)
- CB prefix: 7 sub-opcodes — `RR L` (`CB 1D`), `SRL H` (`CB 3C`),
  `SRL A` (`CB 3F`), `BIT 4..7,A` (`CB 67/6F/77/7F`)
- ED prefix: a small mix — `OUT0 (n),A`, `LD I,A`, `RETI`, `IM 1`,
  `MLT rr` family, `IN0 r,(n)`, `OUT0 (n),r`, `TST r`/`TST n`. A few
  bitfield fall-throughs in `ed_instruction_tstates()` give plausible
  timing for adjacent ED opcodes the dispatch table doesn't actually
  implement, which is its own correctness risk.

### What is missing — concrete shortlist a ROM will hit

1. **All IX/IY (`DD` / `FD`) prefixes.** Any non-trivial subroutine
   prologue that uses `IX` as a frame pointer will fail. This is the
   default for every C compiler that targets Z80, including SDCC.
   Without IX/IY the ROM author is restricted to register-only or
   global-pointer code.
2. **Block transfer / search instructions.** `LDI` / `LDIR` / `LDD` /
   `LDDR` / `CPI` / `CPIR` / `CPD` / `CPDR`. These are the standard
   primitives for VRAM uploads, sprite table copies, and string moves.
   No memcpy-equivalent works without them.
3. **`RST n` (8 vectors).** `RST 0x38` is only reachable as the IM1
   interrupt — software that uses `RST 0x08`–`RST 0x30` as fast call
   trampolines will not run. Many size-optimized handlers and
   Z80-monitor-style firmwares depend on these.
4. **Conditional `CALL`** (`0xC4`, `0xCC`, `0xD4`, `0xDC`, `0xE4`,
   `0xEC`, `0xF4`, `0xFC`). Half the ALU/conditional flow surface
   compilers and asm authors expect.
5. **Conditional `JP cc,nn`** for `C`, `NC`, `PO`, `PE`, `P`, `M`.
   Only `JP Z` and `JP NZ` are wired today.
6. **`JP (HL)`** (`0xE9`) — used by every jump-table dispatch. Without
   this, switch-style state machines fall back to long if/else chains.
7. **Indirect loads `LD A,(BC)`** / `LD A,(DE)` / `LD (BC),A` /
   `LD (DE),A` (`0x02`, `0x0A`, `0x12`, `0x1A`). Standard for buffer
   walks where `HL` is reserved.
8. **`EX (SP),HL`** (`0xE3`), **`EX AF,AF'`** (`0x08`), **`EXX`**
   (`0xD9`), **`LD SP,HL`** (`0xF9`).
9. **Rotate/shift accumulator forms** `RLCA`, `RLA`, `RRA` (the
   register form `RRCA` is implemented; the others are not).
10. **`DAA`** (`0x27`), **`CCF`** (`0x3F`).
11. **`LD (HL),n`** (`0x36`).
12. **CB-prefix bit ops (almost all of them).** `BIT n,r`, `RES n,r`,
    `SET n,r`, all rotates/shifts on `B/C/D/E/L/(HL)`. Of 256 CB
    sub-opcodes, **7** are implemented. Any ROM that uses bit flags
    (controller polling, sprite state) will trap immediately.
13. **ED-prefix block I/O and most arith.** `NEG`, `RETN`, `IM 0`,
    `IM 2`, `IN r,(C)` / `OUT (C),r`, `RLD` / `RRD`, `LDIR` and
    siblings, `INI` / `OTIR`, etc.
14. **`IM 2`** specifically. The spec describes INT1/INT2 and PRT0/PRT1
    as **mode-independent** vectored interrupts, but a ROM might still
    legally execute `IM 2` for the external `INT0` line; the timed
    dispatcher will refuse it.

### Why this matters for agentic ROM authoring

Every milestone since M19 has been "one more opcode the harness
asked for." That model has worked because the only ROM exercising the
emulator is PacManV8 and it is being written cooperatively. An
agentically generated ROM has no such cooperative loop — the agent's
output will reflect whatever instruction mix its toolchain or hand
emit, and the emulator will reject the first unsupported one.

There are two viable paths forward; we should pick one before more
opcode-by-opcode milestones are spent:

- **(A) Bring the timed core up to a published HD64180 instruction
  set baseline in one pass.** This is a much larger milestone than
  recent ones, but it converges the gap once instead of polling for
  the next missing opcode.
- **(B) Document a strict authoring subset.** Publish, in the repo,
  the exact list of opcodes that work today (primary, CB, ED), and
  require any agentic toolchain to emit only that subset. The
  authoring agent can target SDCC with `--no-std-crt0` and a custom
  stripped runtime, or hand-write asm against the documented subset.

Path (B) is realistic in the short term but fragile; path (A) is
where the emulator should land before the project claims "agentic ROM
authoring works."

### Status of `IX`/`IY` register state

The register snapshot exposes `ix_` / `iy_`, but they exist purely as
state slots — no instruction reads or writes them, and the prefix
bytes `0xDD` / `0xFD` are not in either table. Removing the storage
without coverage would not break anything; adding the coverage is
where the real work is.

---

## 2. Video — VDP Coverage Gaps

### 2.1 Display modes — Medium

`current_display_mode()` (`v9938.cpp:795`) recognizes Graphic 1, 2, 3,
4, and 6 only. Mode bits for Text 1, Text 2, Graphic 5, and Graphic 7
fall through to `DisplayMode::unsupported`, which fills the line with
backdrop color and disables sprite rendering for that line.

Concretely missing:

- **Text 1 (Screen 0).** 40×24 6-pixel font, 2 colors from R#7. A ROM
  that wants any in-game text without committing the VRAM cost of
  Graphic 4 string blits has no other path. This is also the cheapest
  way to put a HUD on VDP-A while VDP-B runs the game layer.
- **Text 2 (Screen 0W).** 80×24 mode with the 4-color blink mechanism.
  Mostly relevant to homebrew/dev tools, but any ROM that wants dense
  text (debug consoles, dialogue heavy menus) will want it.
- **Graphic 5 (Screen 6).** 512×212 at 2bpp. Spec marks the palette
  mapping as needing V9938-databook verification before implementation.
- **Graphic 7 (Screen 8).** 256×212 at 8bpp direct color. Same spec
  caveat — direct-color encoding needs databook verification.

For a first agentic-ROM target, Graphic 4 alone is enough, but Text 1
is a cheap win and would unlock a whole class of menu/HUD games.
Graphic 5/7 should stay deferred until the spec verification work
happens.

### 2.2 Command engine pixel packing — Medium

The hardware blitter (LMMV, LMMM, HMMV, HMMM, YMMM, LINE, PSET,
POINT, SRCH, LMMC, HMMC) is implemented in `v9938.cpp`, but every
pixel access goes through `graphic4_pset()` / `graphic4_point()` /
`graphic4_command_byte_address()`. Running a command in Graphic 3, 5,
6, or 7 will read/write pixels at the wrong VRAM offsets and at the
wrong bit depth.

Even though Graphic 6 has a *renderer*, the *command engine* still
treats VRAM as 2-pixels-per-byte at 128 bytes/line — Graphic 6 is 2
pixels/byte but at 256 bytes/line, so HMMM blits will land in the
wrong place. A ROM doing tile compositing on Graphic 6 will produce
visibly garbled output even though the screen renders.

This is coupled to 2.1 — fixing modes Graphic 5/6/7 properly means
fixing pixel-packing for both the renderer and the command engine
together.

### 2.3 Graphic 3 LN=1 lines 192–211 — Medium (locked by spec)

`render_graphic3_background_scanline()` returns backdrop for any
`line >= 192`. The repo spec
(`docs/spec/02-video.md`) explicitly forbids inventing the LN=1 fetch
rule until it is verified against the V9938 databook. Locked
intentionally; flag here only because a ROM that selects Graphic 3 +
LN=1 will see a 20-line stripe of backdrop at the bottom of the
visible frame and may be confused by it.

### 2.4 Horizontal scroll R#26/R#27 — Medium (locked by spec)

`vertical_scroll()` reads R#23 and is correctly applied in
`graphic4_byte_address()` and `graphic6_byte_address()`. R#26/R#27 are
read into the register file but **never consulted** by any rendering
path. Per the spec, this is locked intentionally pending databook
verification. Per-line horizontal parallax effects therefore cannot
be implemented by a ROM.

### 2.5 Sprite overflow status reporting — Medium

When `visible_sprite_count > 4` (Mode 1) or `> 8` (Mode 2),
`v9938.cpp` sets `status[0] |= 0x40` (5th-sprite/9th-sprite flag) but
does not write the *sprite number* of the overflowing sprite into
`S#0` bits 4:0 (5S/C5–C0 fields). A ROM that reads the status
register to identify which sprite overflowed will get stale data.

### 2.6 Scroll table for tile modes — Low

Vertical scroll via R#23 is only applied in Graphic 4 and Graphic 6.
The spec is explicit that R#23 is "available in Graphic 4, 5, 6, and
7"; tile-mode behavior is not committed. So this is correct as-is, but
worth noting because authoring agents may try to scroll tile-mode
backgrounds and silently get nothing.

### 2.7 VDP command timing — Low

`estimate_command_cycles()` returns `1` for every command except HMMM,
which uses a coarse `64 + (NX × NY × 3)` estimate. This means:

- LMMV/LMMM/HMMV/YMMM/LINE/etc. **complete instantaneously** in master
  cycles. The CE flag (S#2 bit 0) clears almost immediately after the
  R#46 write.
- A ROM that polls CE before queuing the next blit will never observe
  it set. If the ROM batches blits across V-blank and assumes "the
  blitter is still busy from frame N-1," it will misbehave.
- Worst case for ROM behavior: a ROM that *intentionally* uses a busy
  blit as a software timer (rare on Z80, but happens) will run too
  fast.

This is a correctness-of-timing issue, not a correctness-of-output
issue. Real software is unlikely to depend on a precise count of
master cycles per blit, but should at least see CE asserted long
enough to be observable across a few CPU instructions.

---

## 3. Audio

### 3.1 General posture — OK

YM2151 (vendored Nuked OPM), AY-3-8910 (Ayumi), and MSM5205 (custom
adapter) are all wired. Master clock derivation matches the spec
(YM2151 from master÷4, AY from master÷8, MSM5205 from a separately
modeled rate). YM2151 IRQ is on the INT0 wire-OR; MSM5205 /VCLK fires
INT1; AY has no interrupt and is correct that way. Stereo mix lands
in `audio_mixer.cpp` with the YM2151 carrying its own L/R pan and AY +
MSM5205 center-panned, matching the spec mixing model.

### 3.2 ADPCM nibble feed — OK but worth a sanity test

`Msm5205Adapter::trigger_vclk()` advances the ADPCM core only when
called by the scheduler at the configured /VCLK rate, and INT1 is
asserted by the bus when the rate is active. The scheduler-driven
timing is correct, but the failure mode (writing nibble too late
because the INT1 handler missed a deadline at 8 kHz) is silent —
nothing logs a missed-deadline event. Authoring agents debugging
ADPCM will not know the difference between "wrong sample data" and
"correct data, fed late." A simple counter of late nibbles surfaced
through the headless inspect would help.

### 3.3 No audio gaps that block a ROM

Closing milestone-15-era audio bring-up plus M33-T01's
instruction-granular timing fix means the audio path is the most
ROM-ready subsystem in the repo. The remaining open items
(WAV record, audio panel) are dev/debug ergonomics rather than
hardware correctness.

---

## 4. Memory / Cartridge

### 4.1 Battery-backed cartridge SRAM — Low (locked by spec)

The spec mentions optional 8–32 KB battery SRAM mapped into the bank
window, but explicitly defers the mapper details. `Cartridge` only
provides ROM banking via BBR. A ROM that wants persistent save data
has nowhere to write it.

For the agentic-ROM goal specifically: the obvious workaround is to
publish a documented "Vanguard 8 save snapshot" mechanism in the
emulator's headless API rather than emulating cartridge SRAM. Treat
this as a deliberate exclusion until the spec defines it.

### 4.2 BBR illegal range — OK

BBR ≥ 0xF0 is rejected per spec. Verified by tests. No gap.

### 4.3 ROM size limit — OK

960 KB cap is enforced and matches spec (16 KB fixed + 59 × 16 KB
banks).

---

## 5. Interrupts

### 5.1 INT0 wire-OR — OK

VDP-A V-blank, VDP-A H-blank, and YM2151 IRQ all feed `int0_state_`
in the bus and the IM1 vector is dispatched correctly. Status
register reads clear the appropriate flags. No gap.

### 5.2 INT1 / vectored peripheral interrupts — OK

INT1 is wired to MSM5205 /VCLK; PRT0/PRT1 use the same vectored
mechanism via `I` and `IL`. Tested at the unit level. No gap.

### 5.3 IM 0 / IM 2 — Medium

`IM 1` is implemented (`op_ed_im_1`). `IM 0` and `IM 2` are not in the
ED dispatch table. The Vanguard 8 firmware runs in IM 1 by spec, so
this is consistent — but a ROM that runs `IM 2` for its own external
INT0 handler design (legal Z80 code) will trap on `Unsupported timed
Z180 ED opcode`. Since INT1/INT2/PRT are mode-independent on HD64180,
the practical impact of missing `IM 2` is small, but the dispatch
should still accept the instruction.

### 5.4 NMI — OK by spec

There is no NMI source on the Vanguard 8. The CPU not implementing
NMI dispatch is correct.

---

## 6. I/O — Controllers

Two controllers, 8 buttons each, active low. `ControllerPorts`
matches the spec bit map exactly. 4-player expansion is deferred by
spec. No gap.

---

## 7. Frontend / Headless

Out of scope per the audit brief, but worth a short note for ROM
workflows specifically:

- The headless binary (`vanguard8_headless`) supports replay capture,
  frame hashing, and inspection report dumps. This is how an authoring
  agent can verify a ROM's behavior frame by frame without spinning
  up SDL.
- `--peek-mem` / `--peek-logical` are useful for asserting state.
  Logical row prefixes were normalized to 4-digit hex in milestone 43,
  so external regex-based parsers work.
- There is no ROM-side debug-print mechanism (port `0xFE` is not
  registered). For agentic authoring this would be useful: a ROM
  could emit `OUT (0xFE),A` byte-by-byte and the emulator could
  capture it as a side-channel log without affecting hardware
  behavior. Spec-wise this would have to be added as an emulator-only
  debug port; a real Vanguard 8 cartridge cannot rely on it.

---

## 8. Recommended Order of Work

Driven by what blocks ROM authoring most directly, not by audit
severity alone.

1. **Pick a CPU strategy and execute it (Section 1).** Either
   bring the timed core up to a documented HD64180 baseline in one
   sweep, or publish a precise authoring-subset list and gate
   agentic ROM tooling against that list. Status quo (one opcode per
   milestone) does not get us to "any ROM can run."
2. **Sprite overflow status fields (2.5).** Cheap fix, prevents a
   plausible class of subtle ROM bugs.
3. **Text 1 background renderer (2.1).** First non-Graphic-4 mode
   that has unambiguous spec, and unlocks HUD/menu authoring.
4. **VDP command timing realism (2.7).** Make CE observable; cap
   "complete in 1 master cycle" with at least a per-pixel cost per
   command. Optional polish, but real ROMs will rely on CE polling.
5. **Optional ROM-side debug port (Section 7).** Only if the team
   wants agent-authored ROMs to be able to log without poking VRAM.
6. **Defer:** Graphic 5/7 (need V9938 databook), R#26/R#27 (same),
   battery SRAM (need spec), 4-player expansion (need spec). These
   are correct deferrals — do not let "ROM compatibility" pressure
   short-circuit the spec verification work.

---

## 9. What is *not* a gap

Filed here so future audits don't reopen them:

- Compositing model: VDP-A index-0 transparency over VDP-B is
  implemented and tested. Both VDPs lockstep on the same scheduler.
- INT0 dispatch and S#0/S#1 clearing.
- INT1 ADPCM feed and vectored response from `I`/`IL`.
- ROM banking via `OUT0 (0x39),A` and the `BBR ≥ 0xF0` rejection.
- Save-state, replay, rewind determinism.
- Controller port bit map and active-low behavior.
- YM2151 / AY / MSM5205 functional output (vendored cores).

These have regression coverage and match the spec; the gap list above
is what prevents an arbitrary ROM from running, not these.

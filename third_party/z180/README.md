# Z180 Core (Imported)

This directory contains the Vanguard 8 CPU core: a faithful import of the
MAME HD64180 / Z180 device sources at a single pinned upstream commit.
After milestone 47 (2026-04-26), full HD64180 opcode coverage is the
shipped state of this core, not a non-goal.

## Source

- Upstream: `mamedev/mame`, files under `src/devices/cpu/z180/`.
- Pinned commit: `c331217dffc1f8efde2e5f0e162049e39dd8717d`.
- License: BSD-3-Clause. The upstream license header is carried verbatim
  at the top of `z180_core.cpp` and `z180_core.hpp`.

## Scope

The imported core supplies dispatch, register state, and cycle timing
for the full instruction set of the Vanguard 8 CPU:

- Full Z80 base set, including `DD` / `FD` (`IX` / `IY`), `CB`, and
  `ED` prefixes.
- Block transfer / search (`LDI` / `LDIR` / `LDD` / `LDDR` / `CPI` /
  `CPIR` / `CPD` / `CPDR`).
- All eight `RST n` vectors, all conditional `CALL cc,nn` and
  `JP cc,nn`, `JP (HL)`, indirect `LD A,(BC)` / `(DE)` and mirror
  forms, `EX (SP),HL` / `EX AF,AF'` / `EXX` / `LD SP,HL`, accumulator
  rotates (`RLCA` / `RLA` / `RRA`), `DAA`, `CCF`, `LD (HL),n`, and
  the 256-entry CB-prefix bit-op surface.
- ED-prefix block I/O and arithmetic, `NEG`, `RETN`, `IM 0` / `IM 2`,
  `IN r,(C)` / `OUT (C),r`, `RLD` / `RRD`, `INI` / `OTIR` and
  siblings.
- HD64180-specific instructions: `MLT`, `IN0`, `OUT0`, `TST`,
  `TSTIO`, `OTIM` / `OTDM` / `OTIMR` / `OTDMR`, `SLP`.
- On-chip MMU registers (`CBAR`, `CBR`, `BBR`) with the Vanguard 8
  spec's logical-to-physical translation rules.
- HD64180 PRT timer block (`TMDR0/1`, `RLDR0/1`, `TCR`) with
  vectored interrupt semantics.
- HD64180 DMA controller surface used by `Z180Adapter::execute_dma`.
- Vectored-interrupt response for `INT1` / `INT2` / `PRT0` / `PRT1`
  driven by `I` and `IL`.

Cycle timing is supplied by the imported core. No per-opcode timing
tables exist in `src/core/cpu/z180_adapter.{hpp,cpp}`.

## Vanguard 8 spec overrides preserved

Where MAME's upstream behavior diverges from `docs/spec/01-cpu.md`,
the Vanguard 8 spec wins and the divergence is implemented as a
clearly-marked override inside the imported core or its in-tree shim.
The preserved overrides are:

- **Reset values for `CBAR`, `CBR`, `BBR`** — `CBAR=0x48`, `CBR=0xF0`,
  `BBR=0x04` per the boot configuration in `docs/spec/01-cpu.md`.
- **Vectored-interrupt low-byte rule** — `vector_low = (IL & 0xE0) |
  fixed_code` for `INT1` / `INT2` / `PRT0` / `PRT1`, matching the
  spec rather than MAME's wider mask.
- **PRT timer reset values** — `TMDR0` / `TMDR1` / `RLDR0` / `RLDR1`
  initialize to `0xFFFF`.
- **HALT-resume semantics fixed in M36** — on interrupt service from
  HALT, `halted_` clears and `pc_` advances past the HALT opcode
  before the return address is pushed.

Any future divergence between MAME upstream and the Vanguard 8 spec
must be recorded here and in `docs/spec/01-cpu.md`, not silently
adopted.

## Adapter surface

`src/core/cpu/z180_adapter.{hpp,cpp}` is bus glue. It carries the
breakpoint, bank-switch logging (`OUT0 (0x39)`), INT1 acknowledgement,
internal-I/O mirroring, save-state snapshot/load, peek/translate, and
HALT-resume observability surfaces. The adapter never holds an opcode
table — every primary, CB, ED, DD, and FD instruction is dispatched by
the imported core.

## Updating the pin

If a new upstream commit is adopted:

1. Replace `z180_core.cpp` and `z180_core.hpp` from the same single
   commit; do not mix versions.
2. Re-apply the spec overrides above. Each override is marked with a
   `// Vanguard 8 spec override` comment in the imported source.
3. Re-run the M47 verification commands in
   `docs/emulator/milestones/47.md` — at minimum `cmake --build
   cmake-build-debug`, full `ctest`, the replay fixture smoke, and
   the canonical PacManV8 T021 harness. Replay-fixture digest shifts
   are acceptable if reproducible across three repeat runs and
   inspection confirms a uniform timing delta rather than a behavior
   change.
4. Update the pinned-commit line at the top of this file.

# M30-T01 — Cover Missing Timed HD64180 PacManV8 VBlank Handler PUSH/POP rr Opcodes

Status: `completed`
Milestone: `30`
Depends on: `M29-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/spec/04-io.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/30.md`
- `docs/emulator/15-second-pass-audit.md`
- `/home/djglxxii/src/PacManV8/docs/field-manual/vanguard8-build-directory-skew-and-timed-opcodes.md`

Scope:
- Add timed opcode coverage for the register-pair stack opcodes exercised
  by the PacManV8 VBlank handler after it was widened to save more
  registers around `audio_update_frame`:
  - `0xC5` (`PUSH BC`) at `PC 0x0039`
  - `0xD5` (`PUSH DE`) at `PC 0x003A`
  - `0xE5` (`PUSH HL`) at `PC 0x003B`
  - `0xE1` (`POP HL`)  at `PC 0x0040`
  - `0xD1` (`POP DE`)  at `PC 0x0041`
  - `0xC1` (`POP BC`)  at `PC 0x0042`
- Implement all six as one pass. They are a single homogeneous family
  triggered by the same PacManV8 handler change, and the PacManV8 field
  manual explicitly recommends closing them together rather than looping
  on one-opcode-at-a-time patches.
- Keep `PUSH AF` (`0xF5`) and `POP AF` (`0xF1`) — already registered —
  unchanged, and add regression coverage that they remain covered.
- Add a narrow integration regression that reproduces the PacManV8
  VBlank handler prologue and epilogue around an `audio_update_frame`
  stand-in and proves timed execution survives past all six PCs.

Blocker reference:
- Binary: `/home/djglxxii/src/Vanguard8/cmake-build-debug/src/vanguard8_headless`
  (canonical; the stale `build/` directory was removed on 2026-04-19 —
  any task or field-manual note still pointing at
  `build/src/vanguard8_headless` is out of date)
- Command:
  ```bash
  /home/djglxxii/src/Vanguard8/cmake-build-debug/src/vanguard8_headless \
    --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
    --frames 180 --hash-audio \
    --expect-audio-hash a8d5a5c921628a88b12a4b95e1294af3ddd79620bd940b0702300098af341483
  ```
- Observed abort (current):
  `Unsupported timed Z180 opcode 0xC5 at PC 0x39`
- Expected subsequent aborts per the field manual once `0xC5` is cleared,
  in order: `0xD5` at `PC 0x3A`, `0xE5` at `PC 0x3B`, `0xE1` at `PC 0x40`,
  `0xD1` at `PC 0x41`, `0xC1` at `PC 0x42`. All six are in scope for this
  task, so none of them should surface as a later abort after this task
  lands.

Done when:
- The evidence-backed PacManV8 T016 regression command no longer throws
  on any of the six designated VBlank handler PCs.
- Tests lock the register-pair stack semantics in the timed extracted
  CPU path:
  - `PUSH rr` (BC/DE/HL): SP <- SP - 2; memory[SP+1] <- high(rr);
    memory[SP] <- low(rr); PC += 1; 11 T-states; flags unchanged.
  - `POP rr` (BC/DE/HL): low(rr) <- memory[SP]; high(rr) <- memory[SP+1];
    SP <- SP + 2; PC += 1; 10 T-states; flags unchanged.
- Tests confirm the already-registered `PUSH AF` / `POP AF` paths still
  execute and time correctly after the new opcodes are added.
- Runtime/integration coverage exercises the full PacManV8-shaped
  prologue/epilogue sequence (`PUSH AF; PUSH BC; PUSH DE; PUSH HL;` …
  `POP HL; POP DE; POP BC; POP AF;`) and proves the frame loop survives
  past all six of the previously failing opcode PCs with correct SP,
  register, and flag state on exit.
- Remaining timed-opcode gaps (beyond these six) stay explicit and
  documented; no broader CPU completion or generic stack-family table
  expansion (IX/IY forms, `EX (SP),HL`, `LD SP,rr`, etc.) is attempted.

Implementation guidance:
- Declare `op_push_bc`, `op_push_de`, `op_push_hl`, `op_pop_bc`,
  `op_pop_de`, `op_pop_hl` on the extracted z180 core in
  `third_party/z180/z180_core.hpp` and implement them in
  `third_party/z180/z180_core.cpp`, mirroring the existing `op_push_af`
  and `op_pop_af` helpers. Register them at `opcodes_[0xC5]`,
  `opcodes_[0xD5]`, `opcodes_[0xE5]`, `opcodes_[0xC1]`, `opcodes_[0xD1]`,
  and `opcodes_[0xE1]` respectively.
- Wire timing in
  `src/core/cpu/z180_adapter.cpp::current_instruction_tstates()`:
  - Add `0xC5`, `0xD5`, `0xE5` to the existing 11 T-state `PUSH AF`
    (`0xF5`) group — do not split the group; all register-pair pushes
    share the same timing on the HD64180.
  - Add `0xC1`, `0xD1`, `0xE1` to the existing 10 T-state `POP AF`
    (`0xF1`) / `RET` (`0xC9`) group for the same reason.
- Extend `tests/test_cpu.cpp` with focused scheduled-CPU coverage for all
  six new opcodes, pinning the SP direction/magnitude, the register-pair
  byte order in memory, flag preservation, PC advance, and T-state cost.
- Extend `tests/test_integration.cpp` with a narrow ROM harness that
  reproduces the PacManV8 VBlank handler prologue at `PC 0x0038..0x003B`
  and epilogue at `PC 0x0040..0x0043`, wrapping a trivial
  `audio_update_frame` stand-in, and demonstrates the frame loop
  survives past all six previously failing opcode PCs.

Verification:
```bash
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure
```

Completion summary:
- Re-read `docs/spec/01-cpu.md`, `docs/spec/04-io.md`,
  `docs/emulator/03-cpu-and-bus.md`, `docs/emulator/07-implementation-plan.md`,
  `docs/emulator/15-second-pass-audit.md`, the milestone-30 contract, and the
  PacManV8 field manual before implementation, then kept the change inside the
  extracted HD64180 runtime, the timed adapter, tests, and workflow docs.
- Extended the extracted z180 core with `op_push_bc`, `op_push_de`,
  `op_push_hl`, `op_pop_bc`, `op_pop_de`, and `op_pop_hl`. Declarations landed
  next to the existing `op_push_af` / `op_pop_af` entries in
  `third_party/z180/z180_core.hpp`; implementations in
  `third_party/z180/z180_core.cpp` mirror the existing `op_push_af` /
  `op_pop_af` helpers one-for-one, delegating to the shared `push_word` /
  `pop_word` routines so the register-pair byte order, SP direction, and
  flag preservation match the HD64180 semantics documented in `docs/spec`.
- Registered the six new ops at `opcodes_[0xC5]` (`PUSH BC`),
  `opcodes_[0xD5]` (`PUSH DE`), `opcodes_[0xE5]` (`PUSH HL`),
  `opcodes_[0xC1]` (`POP BC`), `opcodes_[0xD1]` (`POP DE`), and
  `opcodes_[0xE1]` (`POP HL`) inside `initialize_tables()`, leaving the
  already-registered `PUSH AF` / `POP AF` handlers untouched.
- Wired timing in `src/core/cpu/z180_adapter.cpp::current_instruction_tstates()`
  by adding `0xC5`, `0xD5`, `0xE5` to the existing 11 T-state `PUSH AF`
  (`0xF5`) case group and `0xC1`, `0xD1`, `0xE1` to the existing 10 T-state
  `POP AF` / `RET` (`0xF1` / `0xC9`) case group. The homogeneous register-pair
  stack timing is encoded as shared case-fallthroughs rather than as six
  separate clauses.
- Added three focused scheduled-path CPU `TEST_CASE`s in `tests/test_cpu.cpp`:
  - `scheduled CPU covers PUSH rr used by the PacManV8 VBlank handler prologue`
    exercises `PUSH BC`, `PUSH DE`, `PUSH HL`, and — as a regression — `PUSH AF`,
    pinning 11 T-states, SP predecrement by 2, correct high/low byte order in
    SRAM, unchanged flags, and PC += 1.
  - `scheduled CPU covers POP rr used by the PacManV8 VBlank handler epilogue`
    exercises `POP BC`, `POP DE`, `POP HL`, and — as a regression — `POP AF`,
    pinning 10 T-states, SP postincrement by 2, correct byte ordering out of
    SRAM, unchanged flags, and PC += 1.
  - `scheduled CPU covers the full PacManV8 VBlank prologue/epilogue
    PUSH/POP sequence` runs the exact `PUSH AF; PUSH BC; PUSH DE; PUSH HL;`
    ... `POP HL; POP DE; POP BC; POP AF;` handler-shaped sequence under
    `run_until_halt`, proving AF/BC/DE/HL are restored byte-for-byte and SP
    returns to its starting value.
- Added runtime integration coverage in `tests/test_integration.cpp`
  (`make_pacmanv8_vblank_push_pop_rr_blocker_rom`) that places the prologue
  at `PC 0x0038..0x003B`, four NOPs as the `audio_update_frame` stand-in at
  `PC 0x003C..0x003F`, and the epilogue + HALT at `PC 0x0040..0x0044`. The
  matching `TEST_CASE` stages the boot MMU (`CBAR=0x48`, `CBR=0xF0`,
  `BBR=0x04`), seeds SP into SRAM, and demonstrates the frame loop survives
  past every one of the previously failing opcode PCs with SP, AF, BC, DE,
  and HL all restored correctly on exit.
- Kept the milestone strictly narrow. Only the six register-pair stack
  opcodes named by the PacManV8 field manual were added. No IX/IY PUSH/POP
  forms, no `EX (SP),HL`, no `LD SP,rr`, and no VDP/palette/compositor/audio
  changes rode along. `PUSH AF` and `POP AF` remain unmodified; regression
  assertions in both new `PUSH rr` / `POP rr` test cases confirm they still
  execute and time correctly.
- Verified on the final tree with:
  `cmake --build cmake-build-debug`
  `ctest --test-dir cmake-build-debug --output-on-failure`
  All 145 tests pass (138 prior + 4 from M29 + 3 added by this task: two
  focused CPU `TEST_CASE`s with sections, one CPU full-sequence
  `TEST_CASE`, and one integration `TEST_CASE`). One test remains
  intentionally skipped because the showcase ROM is not built in this
  configuration.
- Ran the PacManV8 T016 regression command against the canonical
  `cmake-build-debug/src/vanguard8_headless` binary with the current
  `/home/djglxxii/src/PacManV8/build/pacman.rom`. The previously observed
  abort at `Unsupported timed Z180 opcode 0xC5 at PC 0x39` is gone. The ROM
  now advances through the VBlank handler prologue and epilogue and aborts
  far later, at `Unsupported timed Z180 opcode 0x7C at PC 0x2954` (`LD A,H`).
  That new abort is out of M30 scope — it is a distinct timed-CPU gap
  surfaced once the VBlank handler became traversable, and belongs to a
  future narrow compatibility milestone. Final PacManV8 T016 audio-hash
  verification remains blocked on that new gap and any further gaps it
  uncovers, but M30's stated exit criterion (the emulator no longer aborts
  on any of the six register-pair stack opcode PCs on the VBlank handler
  path) is met.

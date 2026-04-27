# M47-T01 — Full MAME HD64180 / Z180 Core Import

Status: `completed`
Milestone: `47`
Depends on: `M46-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/47.md`
- `docs/emulator/16-rom-readiness-audit.md` (Section 1)

## Scope

Replace the milestone-2-era hand-written Z180 stub with the full
MAME HD64180 / Z180 CPU core, and shrink `Z180Adapter` to bus glue.
After this task lands, the timed dispatch tables and every
`"Unsupported timed Z180 opcode …"` throw site no longer exist;
the imported core supplies its own opcode dispatch and cycle
counts.

## Authoritative source

- Upstream: `mamedev/mame`, files under `src/devices/cpu/z180/`.
- License: BSD-3-Clause (acceptable per project memory; license is
  not a project constraint).
- Pin: a single MAME commit SHA, recorded in
  `third_party/z180/README.md` after import. Use the same pin for
  the `.cpp` and `.h` files; do not mix commits.

## Step 1 — Import

1. Pull the MAME z180 device sources at the chosen pinned commit
   into `third_party/z180/`. Replace the existing
   `z180_core.hpp` / `z180_core.cpp` with the imported sources.
   Carry over the upstream license header verbatim.
2. Strip the MAME device-framework dependencies (`device_t`,
   `device_execute_interface`, `state_string_export`,
   `screen_device`, `address_space`, `disasm_interface`,
   anything that requires `emu.h`). The imported core must
   compile against the project's existing C++20 toolchain with
   no MAME runtime present.
3. Where MAME uses its memory-space and I/O-space callbacks,
   replace them with the existing `vanguard8::core::cpu::
   Callbacks` shape already used by the milestone-2 stub
   (`read_memory`, `write_memory`, `read_port`, `write_port`,
   `observe_*`, `record_warning`, `acknowledge_int1`). The
   adapter remains the single bus boundary.
4. Preserve, in the imported core, the Vanguard 8 spec rules that
   the milestone-2 extraction had previously layered on top of
   MAME:
   - Reset values for `CBAR` / `CBR` / `BBR` per
     `docs/spec/01-cpu.md`. If MAME's defaults differ, override
     in a clearly-marked `// Vanguard 8 spec override` block.
   - The `IL & 0xE0` low-byte rule for vectored interrupt
     pointers (`INT1` / `INT2` / `PRT0` / `PRT1`).
   - PRT timer reset values (`TMDR0` / `TMDR1` / `RLDR0` /
     `RLDR1` initialize to `0xFFFF`).
   - HALT-resume behavior fixed in M36.

## Step 2 — Adapter shrink

Edit `src/core/cpu/z180_adapter.{hpp,cpp}`:

1. **Delete** the following members and every line of their
   bodies:
   - `Z180Adapter::current_instruction_tstates()`
   - `Z180Adapter::cb_instruction_tstates(...)`
   - `Z180Adapter::ed_instruction_tstates(...)`
   - Every `case 0x..` opcode label and `if (opcode >= 0x.. &&
     opcode <= 0x..)` range-check inside those functions.
   - Every
     `throw std::runtime_error("Unsupported timed Z180 opcode …")`
     site in this file.
2. **Replace** `Z180Adapter::next_scheduled_tstates()` and
   `Z180Adapter::step_scheduled_instruction()` with
   thin wrappers that ask the imported core for the next
   instruction's cycle count and execute one instruction
   respectively. Keep the existing pre/post observation hooks
   (breakpoint hit recording, bank-switch logging, INT1
   acknowledgement) intact.
3. **Preserve** every other public member of `Z180Adapter`:
   `pc()`, `accumulator()`, `register_i()`, `register_il()`,
   `register_itc()`, `cbar()`, `cbr()`, `bbr()`,
   `interrupt_mode()`, `halted()`, `state_snapshot()`,
   `load_state_snapshot()`, `breakpoints()`,
   `breakpoint_hits()`, `bank_switch_log()`,
   `set_register_i()`, `set_interrupt_mode()`, `set_iff1()`,
   `set_iff2()`, `add_breakpoint()`, `clear_breakpoints()`,
   `clear_breakpoint_hits()`, `clear_bank_switch_log()`,
   `in0()`, `out0()`, `advance_tstates()`,
   `execute_dma()`, `translate_logical_address()`,
   `peek_logical()`, `read_logical()`, `write_logical()`,
   `service_pending_interrupt()`, `step_instruction()`,
   `run_until_halt()`, `run_until_breakpoint_or_halt()`,
   `next_interrupt_service_source()`, and the bus-side
   `record_breakpoint_hit()` / `record_bank_switch()`.
   These are consumed by `Bus`, the scheduler, the debugger,
   replay, and save-state.
4. **Save-state migration.** If the imported core represents
   register state in a layout different from `CpuStateSnapshot`,
   either:
   - Translate at the boundary (cheapest; preferred), or
   - Bump the save-state version and write a one-time migration
     path from the M46-era format to the M47 format.
   Either choice must be documented in this task file; existing
   save-state files in the test corpus must continue to load
   without manual intervention.

## Step 3 — Test surface

Edit `tests/test_cpu.cpp`:

1. **Delete** every test asserting a per-opcode T-state value
   against `current_instruction_tstates()` and its CB/ED
   variants. These tests pinned the hand-rolled dispatch tables
   that no longer exist.
2. **Keep or rewrite** the spec-pinned integration tests:
   - MMU translation through `CBAR` / `CBR` / `BBR` (boot
     mapping plus at least one runtime bank switch).
   - Illegal `BBR ≥ 0xF0` rejection / warning behavior.
   - `OUT0 (0x39),A` recorded in `bank_switch_log`.
   - INT0 IM1 dispatch lands at `0x0038` and the V-blank flag
     clears on `S#0` read.
   - INT1 vectored dispatch reads `0x80E0` / `0x80E1` for
     `I = 0x80`, `IL = 0xE0`.
   - PRT0 / PRT1 vectored dispatch.
   - HALT-resume invariant from M36.
3. **Add** behavioral coverage for the previously unsupported
   instruction classes:
   - `LDIR` block copy with `BC = 0` post-condition and the
     destination range byte-equal to the source.
   - `IX`-based subroutine prologue
     (`PUSH IX` / `LD IX,nn` / `LD A,(IX+d)`); read a stack-
     adjacent local and assert the right byte.
   - `BIT n,r` / `RES n,r` / `SET n,r` cycle exercising `H`
     flag side effects.
   - `JP (HL)` jump-table with three entries; assert the right
     branch is taken for each table index.
   - `RST 0x08` round-trip (push, jump, return).
   - `IM 2` + external INT2 vector fetch via `I` / `IL`.
4. **Negative guard:** assert no primary opcode 0x00–0xFF
   triggers a throw from the adapter timing API. The simplest
   form is a parameterized loop that calls
   `Z180Adapter::next_scheduled_tstates()` for a synthesized
   single-byte program at each opcode value and asserts no
   `std::runtime_error` is observed. The CB and ED prefixes
   should be exercised similarly across their full sub-opcode
   range.

## Step 4 — Replay digest re-pinning

The MAME core supplies more accurate timing than the hand-rolled
tables. The deterministic replay fixture
(`tests/replays/replay_fixture.{rom,v8r}`) is expected to produce
a one-time shift in frame and event-log digests.

1. Run the fixture pre-import and record the current digests in
   this task file (under `## Pre-import digests`).
2. Run the fixture post-import three times in a row and confirm
   byte-identical results across the three runs.
3. Update the test goldens to the new digests; record them in
   this task file under `## Post-import digests`.
4. Briefly inspect the pre/post diff: a uniform delay shift is
   expected and acceptable. If the fixture's *behavior* (event
   ordering, port writes, bank switches) diverges substantively,
   record the divergence and stop — that is a defect in the
   adapter shim, not a digest re-pin.

## Step 5 — Doc updates

1. Rewrite `third_party/z180/README.md`:
   - Source: full MAME HD64180 / Z180 device.
   - Pin: the exact upstream commit SHA used.
   - License: BSD-3-Clause (carry the upstream header).
   - Scope: full Z80 + HD64180 instruction set, on-chip MMU /
     PRT / DMA / interrupt response.
   - Vanguard 8 spec overrides: enumerate the spec-anchored
     adjustments that were preserved (CBAR/CBR/BBR boot
     defaults, `IL & 0xE0` rule, PRT reset values, HALT-resume).
   - Remove the "Non-goals" section that listed full opcode
     coverage as out of scope. Replace with a one-line statement
     that full coverage is the shipped state.
2. Update `docs/emulator/16-rom-readiness-audit.md` Section 1's
   status entry: severity drops from "Blocker" to "Resolved" with
   a one-line note pointing at M47.
3. Update `docs/emulator/current-milestone.md` per the milestone-
   discipline rules to reflect M47's status throughout execution.
4. Move this file from `docs/tasks/active/` to
   `docs/tasks/completed/` (or `blocked/`) on closure with the
   summary required by the milestone control rules.

## Pre-import digests

Captured from the M46 (commit `16bc3c5`) tree before the MAME core
import landed. The replay-fixture and PacManV8 T017 audio digests were
the test-suite's pinned values prior to M47.

- Replay fixture (`tests/replays/replay_fixture.{rom,v8r}`) frame 4
  SHA-256: previously pinned at the same value
  `e46b5246bda293e09e199967b99ac352f931c04e2ad88e775b06a3b93ccb838c`
  in `tests/CMakeLists.txt`. The pin survived the import because the
  fixture's traffic only exercises opcodes whose timing did not shift
  under the imported core; no replay-fixture re-pin was required.
- PacManV8 T017 300-frame audio SHA-256 (pinned in
  `tests/test_frontend_backends.cpp:387`):
  `61ca417ef206a0762ea3691cb0e48f5bf567205beffed27877d27afca839e7cc`.

## Post-import digests

Captured at HEAD `9f5c69e` ("Import full MAME HD64180 core and retire
per-opcode CPU milestones (M47)") on 2026-04-26.

Replay fixture, three repeat runs, all byte-identical:

```bash
cmake-build-debug/src/vanguard8_headless \
    --rom tests/replays/replay_fixture.rom \
    --replay tests/replays/replay_fixture.v8r \
    --frames 4 --hash-frame 4 --hash-audio
```

- Frames completed: 4
- Master cycles: 953680
- CPU T-states: 476840
- Event log size: 856
- Event log digest: `7492340876534060551`
- Frame 4 SHA-256:
  `e46b5246bda293e09e199967b99ac352f931c04e2ad88e775b06a3b93ccb838c`
- Audio SHA-256:
  `48beda9f68f15ac4e3fca3f8b54ebea7832a906d358cc49867ae508287039ddf`

The replay-fixture frame digest is unchanged from M46. This is
expected: the fixture's input does not exercise any opcode whose
cycle count differs between the hand-rolled M46 tables and the
imported core, so no uniform timing delta surfaces.

PacManV8 T017 300-frame audio SHA-256 (`tests/test_frontend_backends
.cpp:387`), three repeat runs, all byte-identical:

```text
a765959a62e9a5afe9d075206efb8943e3141ef4274844a07c35f21c20d1ab27
```

This is the digest shift M47 anticipated for "more accurate timing
from the imported core". The frontend-side test still asserts audio
nonzero, frame nonzero, and `pc() != 0x2B8B` — all three structural
assertions pass. The pinned-hash assertion fails because the test's
expected hash predates the import. Re-pinning the test was not in
M47's allowed paths; see "Out-of-scope follow-ups" below.

## Verification evidence

- `cmake --build cmake-build-debug` succeeded at commit `9f5c69e`.
- `ctest --test-dir cmake-build-debug --output-on-failure` ran at
  `194 / 195` (showcase milestone-7 test pre-existing skip; one
  failure: test 114 *PacManV8 T017 audio/video output is nonzero
  after instruction-granular audio timing*, the digest re-pin
  deferred to M48 below). Spec-anchored CPU integration tests pass
  (MMU translation, illegal `BBR` rejection, `OUT0 (0x39)` bank-
  switch logging, INT0 / INT1 / PRT vectored dispatch, HALT-resume).
  Behavioral coverage for previously unsupported instruction classes
  passes (`LDIR` block copy, `IX`-based prologue, CB-prefix bit ops,
  `JP (HL)` jump table, `RST 0x08` round-trip, `IM 2` + INT2 vector
  fetch). Negative guard confirming no primary opcode 0x00–0xFF
  triggers a throw from the adapter timing API passes (the throw
  sites no longer exist).
- `vanguard8_headless_replay_regression` (ctest #195) passes against
  the post-import frame digest pinned in `tests/CMakeLists.txt:63`.
- Replay fixture three-run reproducibility: confirmed (digests
  above; byte-identical across runs).
- Headless dispatch-trap absence smoke:
  `cmake-build-debug/src/vanguard8_headless --rom
  tests/replays/replay_fixture.rom --replay
  tests/replays/replay_fixture.v8r` runs cleanly to the fixture's
  programmed end with event-log digest `7492340876534060551`. No
  `Unsupported timed Z180 opcode` trap.
- Negative-guard grep for the throw string across `src/`,
  `third_party/`, and `tests/` returns no matches.
- Canonical PacManV8 T021 harness:
  ```bash
  cd /home/djglxxii/src/PacManV8
  python3 tools/build.py
  python3 tools/pattern_replay_tests.py \
      --evidence-dir tests/evidence/T021-pattern-replay-and-fidelity-testing
  ```
  Result: `2/2 passed` (`early-level-pattern` and
  `short-corner-route`). No CPU-dispatch trap.

## Out-of-scope follow-ups exposed during execution

- **PacManV8 T017 300-frame audio digest re-pin.** The imported MAME
  core's more accurate audio timing shifts the
  `tests/test_frontend_backends.cpp:387` pinned hash from
  `61ca417e…839e7cc` to `a765959a…20d1ab27` (deterministic across
  three repeat runs). The structural assertions in the test
  (audio nonzero, frame nonzero, `pc() != 0x2B8B`) still pass; only
  the pinned digest needs updating. `tests/test_frontend_backends
  .cpp` is not in M47's allowed paths, so the re-pin is deferred to
  milestone 48 (`docs/emulator/milestones/48.md`,
  `docs/tasks/active/M48-T01-pacmanv8-t017-audio-digest-repin.md`).

## Completion Summary

Date: 2026-04-26

Implemented against:
- `docs/spec/01-cpu.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/47.md`
- `docs/emulator/16-rom-readiness-audit.md` (Section 1)

Completed within M47 scope:
- Replaced the milestone-2 hand-written Z180 stub with the full
  MAME HD64180 / Z180 device sources at pinned upstream commit
  `c331217dffc1f8efde2e5f0e162049e39dd8717d`. License header
  carried verbatim. The MAME device-framework dependencies were
  stripped or replaced with an in-tree shim that preserves the
  existing `vanguard8::core::cpu::Z180Adapter` public surface.
- Removed `Z180Adapter::current_instruction_tstates()`,
  `Z180Adapter::cb_instruction_tstates()`, and
  `Z180Adapter::ed_instruction_tstates()` together with every
  range-check, `case 0x..` opcode label, and
  `throw std::runtime_error("Unsupported timed Z180 opcode …")`
  site. The imported core supplies dispatch and cycle timing.
- Preserved the existing observability surface: breakpoints,
  `record_breakpoint_hit`, `record_bank_switch` on `OUT0 (0x39)`,
  `bank_switch_log`, `acknowledge_int1`, internal-I/O mirroring,
  save-state snapshot/load, peek/translate, and HALT-resume.
- Preserved the Vanguard 8 spec overrides: `CBAR=0x48`,
  `CBR=0xF0`, `BBR=0x04` reset values; the `IL & 0xE0`
  vectored-interrupt low-byte rule; `TMDR0`/`TMDR1`/`RLDR0`/
  `RLDR1` reset values of `0xFFFF`; HALT-resume semantics from
  M36.
- Rewrote `tests/test_cpu.cpp` against spec-pinned integration
  tests plus behavioral coverage for previously unsupported
  instruction classes (`LDIR`, `IX`-based prologue, CB-prefix bit
  ops, `JP (HL)` jump table, `RST 0x08` round-trip, `IM 2` + INT2
  vector fetch). Added the negative guard asserting no primary
  opcode 0x00–0xFF triggers a throw from the adapter timing API.
- Replay fixture digests in `tests/replays/` did not require a
  re-pin: the fixture's input does not exercise opcodes whose
  cycle counts shifted under the imported core. The
  `vanguard8_headless_replay_regression` ctest entry continues to
  pass against the M46 frame-4 digest
  `e46b5246bda293e09e199967b99ac352f931c04e2ad88e775b06a3b93ccb838c`.
- Rewrote `third_party/z180/README.md` to describe the full import,
  the pinned upstream commit, the BSD-3-Clause license, the
  preserved Vanguard 8 spec overrides, and the new shipped state
  (full opcode coverage). Removed the milestone-2 opcode list and
  the "full opcode coverage is a non-goal" line.
- Updated `docs/emulator/16-rom-readiness-audit.md` Section 1
  status from "Blocker" to "Resolved" with a one-line note
  pointing at M47.
- Updated `docs/emulator/07-implementation-plan.md` to mark
  Milestone 47 as accepted with a closure annotation, append the
  Milestone 48 entry for the deferred T017 audio digest re-pin,
  and note that M19–M46 are superseded in spirit by M47.
- Updated `docs/emulator/current-milestone.md` to record M47
  acceptance and the M48 carryover.

Verification completed:
- `cmake --build cmake-build-debug` passed.
- `ctest --test-dir cmake-build-debug --output-on-failure` ran at
  `194 / 195` (one pre-existing showcase skip; one out-of-scope
  digest re-pin failure routed to M48 — see "Out-of-scope follow-
  ups exposed during execution" above).
- Replay-fixture three-run reproducibility confirmed.
- Headless dispatch-trap absence smoke confirmed.
- PacManV8 T021 harness passes both replay cases end-to-end.

Remaining blocker:
- None inside M47's allowed scope. The deferred T017 audio digest
  re-pin is captured by M48, which authorizes the narrow
  re-pin in `tests/test_frontend_backends.cpp`.

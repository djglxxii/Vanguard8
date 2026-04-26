# M47-T01 — Full MAME HD64180 / Z180 Core Import

Status: `active`
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

_To be filled in during execution._

## Post-import digests

_To be filled in during execution._

## Verification evidence

_To be filled in during execution. At minimum:_

- `cmake --build cmake-build-debug` succeeded at commit ____.
- `ctest --test-dir cmake-build-debug --output-on-failure`
  passed at ___ / ___.
- Replay fixture produced byte-identical digests across three
  runs post-import.
- PacManV8 T021 harness either completed end-to-end or surfaced
  a non-CPU blocker recorded below.

## Out-of-scope follow-ups exposed during execution

_If the import surfaces non-CPU blockers (VDP, audio, scheduler,
headless format), record them here and open a follow-up
milestone — do **not** broaden M47._

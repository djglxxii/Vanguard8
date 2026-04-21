# M35-T01 — Add Headless Agent Observability Primitives

Status: `planned`
Milestone: `35`
Depends on: `M34-T01`

Implements against:
- `docs/spec/00-overview.md`
- `docs/spec/01-cpu.md`
- `docs/spec/02-video.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/04-video.md`
- `docs/emulator/06-debugger.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/35.md`
- `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T020-intermission-cutscenes.md`

Scope:
- Add the headless CLI introspection surface authorized by milestone 35 to
  `src/frontend/headless.cpp` / `src/frontend/headless.hpp`:
  - `--peek-mem ADDR[:LEN]` (repeatable, physical address via
    `core::Bus::read_memory`).
  - `--peek-logical ADDR[:LEN]` (repeatable, translated through the current
    MMU; report both logical and resolved physical address).
  - `--dump-vram-a PATH` and `--dump-vram-b PATH` (raw 64 KB VRAM via
    `core::video::V9938::vram` / `state_snapshot`).
  - `--dump-vdp-regs` (all R#, S#, palette, command-state, and
    `current_visible_width` for both VDPs).
  - `--dump-cpu` (full register block, halt state, MMU via
    `cpu::Z180Adapter::state_snapshot` + existing accessors).
  - `--inspect-frame N` (pin introspection to end of frame `N`; default is
    the last completed frame).
  - `--inspect PATH` (write a single deterministic inspection report file
    combining all active introspection flags; stdout fallback when absent).
  - `--run-until-pc HEX[:MAX_FRAMES]` (drive
    `cpu::Z180Adapter::add_breakpoint(BreakpointType::pc, ...)` from the CLI
    and report hit/no-hit, hit frame index, and master-cycle in the final
    summary).
- Extend `--help` to list every new flag with a one-line usage string.
- Add any headless-only formatting helpers needed for the inspection report
  under `src/frontend/` (e.g., a small `headless_inspect.{hpp,cpp}` pair).
  Must consume only existing `const` / snapshot APIs; no new core state.
- Add tests under `tests/` covering:
  - golden-file lock for the inspection report format against a committed
    fixture ROM or equivalent deterministic scenario
  - determinism guard: two invocations produce byte-identical report files
    and byte-identical VRAM dumps
  - non-perturbation guard: adding any subset or all of the new flags (with
    an unreachable `--run-until-pc` target in the combined case) leaves
    frame digest, audio digest, and event-log digest identical to a run
    without them
  - `--peek-mem` / `--peek-logical` correctness against a ROM with known
    initialized bytes (compare against direct `core::Bus::read_memory`)
  - `--dump-vram-a` / `--dump-vram-b` parity with `video::V9938::vram()` and
    with the dump's own stdout digest
  - `--run-until-pc` hit path and not-hit path, including exit-code behavior
- Update `docs/emulator/06-debugger.md` with a new "Headless Observability"
  section describing every new flag, its arguments, its output, and the
  intended agent workflow (including how to use `--run-until-pc` +
  `--peek-mem` to distinguish state-never-reached from
  state-reached-but-crashed).
- Add a short cross-reference note in `docs/emulator/00-overview.md` or
  equivalent so the section is discoverable from the top-level emulator docs.
- Update `docs/emulator/07-implementation-plan.md` with the milestone 35
  entry and any sequencing notes.
- Move this file from `docs/tasks/planned/` into `docs/tasks/active/` when
  work begins, and into `docs/tasks/completed/` (with a completion summary)
  on acceptance.
- Update `docs/emulator/current-milestone.md` when work begins to flip
  `Status:` to `in-progress`, and again on completion to mark the milestone
  `ready-for-review`.

Blocker reference:
- PacManV8 blocked task:
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T020-intermission-cutscenes.md`
- Three indistinguishable failure modes the current headless tool cannot
  differentiate at the T020 repro frames (`1020`, `1770`, `2520`, `2640`):
  - `GAME_FLOW_STATE_INTERMISSION` is never reached at runtime.
  - It is reached but `intermission_start` crashes or infinite-loops before
    drawing.
  - Drawing runs but is not visible in the composed framebuffer (wrong VRAM
    page, overwrite by a later routine, or similar).
- Canonical smoke regression command for this task:
  ```bash
  cmake-build-debug/src/vanguard8_headless \
      --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
      --frames 120 --inspect-frame 120 \
      --inspect /tmp/vanguard8-m35-pacman-inspect.txt \
      --peek-mem 0xF8250:16 --peek-mem 0xF8270:8 \
      --dump-cpu --dump-vdp-regs
  ```
  The peek addresses cover `GAME_FLOW_STATE_BASE = 0x8250` and
  `INTERMISSION_STATE_BASE = 0x8270` translated to physical SRAM via the
  boot-time `CBAR=0x48 / CBR=0xF0` mapping (SRAM base `0xF0000`).

Done when:
- Every flag in milestone 35's "Required CLI surface" is implemented and
  documented in `--help` and in the new "Headless Observability" section of
  `docs/emulator/06-debugger.md`.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes with the
  new headless-CLI tests included.
- The PacManV8 smoke regression command above produces a deterministic,
  non-empty inspection report covering CPU state, both VDP register blocks,
  the two peeked RAM ranges, and no host-specific content outside a clearly
  marked invocation header.
- The committed golden digest (or golden file) for the smoke regression is
  recorded in the task's completion summary.
- All determinism and non-perturbation guards listed under "Required tests"
  pass.

Out of scope:
- Any change to `src/core/`, `third_party/z180/`, or other non-frontend
  runtime behavior. This milestone adds no new tracking buffers, counters,
  or instrumentation hooks in core.
- Live desktop debugger panels, ImGui UI, or any rendering-side change.
- Opcode, VDP, audio, scheduler, cartridge, input, save-state, or replay
  feature work.
- Changes to the semantics of any existing headless flag
  (`--frames`, `--hash-frame`, `--expect-frame-hash`, `--hash-audio`,
  `--expect-audio-hash`, `--dump-frame`, `--dump-fixture`, `--trace`,
  `--trace-instructions`, `--symbols`, `--press-key`,
  `--gamepad1-button`, `--gamepad2-button`, `--replay`, `--recent`,
  `--paused`, `--step-frame`, `--vclk`).
- PacManV8 ROM edits. T020 evidence refresh happens in the PacManV8 repo
  after this task is completed and the PacManV8 task is reactivated.

Verification commands:
```bash
cd /home/djglxxii/src/Vanguard8
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure

cmake-build-debug/src/vanguard8_headless --help

cd /home/djglxxii/src/PacManV8
python3 tools/build.py

cd /home/djglxxii/src/Vanguard8
cmake-build-debug/src/vanguard8_headless \
    --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
    --frames 120 --inspect-frame 120 \
    --inspect /tmp/vanguard8-m35-pacman-inspect.txt \
    --peek-mem 0xF8250:16 --peek-mem 0xF8270:8 \
    --dump-cpu --dump-vdp-regs

cmake-build-debug/src/vanguard8_headless \
    --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
    --frames 120 \
    --run-until-pc 0x0103:120
```

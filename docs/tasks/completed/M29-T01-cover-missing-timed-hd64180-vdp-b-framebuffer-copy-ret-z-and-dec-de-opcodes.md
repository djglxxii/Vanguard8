# M29-T01 — Cover Missing Timed HD64180 VDP-B Framebuffer Copy RET Z and DEC DE Opcodes for PacManV8

Status: `completed`
Milestone: `29`
Depends on: `M28-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/spec/02-video.md`
- `docs/spec/04-io.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/29.md`
- `docs/emulator/15-second-pass-audit.md`

Scope:
- Reproduce the PacManV8 VDP-B framebuffer copy `RET Z` / `DEC DE`
  blockers designated in
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T007-vdp-b-maze-render-and-pellet-display.md`
  after the milestone-28 `OR E` patch.
- Add only the timed opcode coverage needed for the two base opcodes
  designated by that blocked task:
  - `0xC8` (`RET Z`) at `PC 0x0248`
  - `0x1B` (`DEC DE`) at `PC 0x024D`
- Add regressions that pin each instruction's direct behavior and the
  ROM-driven timed path without broadening into general opcode completion.

Blocker reference:
- Binary: `/home/djglxxii/src/Vanguard8/build/src/vanguard8_headless`
- Command:
  ```bash
  python3 tools/build.py
  /home/djglxxii/src/Vanguard8/build/src/vanguard8_headless \
    --rom build/pacman.rom \
    --frames 60 \
    --dump-frame tests/evidence/T007-vdp-b-maze-render/frame_060.ppm \
    --hash-frame 60
  ```
- Observed abort (current, after M28 `OR E` patch):
  `Unsupported timed Z180 opcode 0xC8 at PC 0x248`
- Next predicted abort after `0xC8` is resolved (per T007 static audit):
  `Unsupported timed Z180 opcode 0x1B at PC 0x24D`

Done when:
- The evidence-backed PacManV8 runtime frame-dump path no longer throws on
  the blocked `PC 0x0248` (`RET Z`) or `PC 0x024D` (`DEC DE`).
- Tests lock `RET Z` conditional-return semantics in the timed extracted
  CPU path: taken path (Z=1) pops PC from SP, advances SP by two, and
  costs 11 T-states; not-taken path (Z=0) advances PC by one and costs
  5 T-states; flags are not modified on either path.
- Tests lock `DEC DE` 16-bit decrement semantics in the timed extracted
  CPU path: DE <- DE - 1 with wrap at 0x0000, PC advances by one,
  6 T-states, flags unchanged.
- Runtime/integration cases exercise the previously failing `PC 0x0248`
  and `PC 0x024D` and prove the frame loop survives past them.
- Remaining timed-opcode gaps (beyond these two) stay explicit and
  documented; no broader CPU completion, conditional-return table, or
  generic 16-bit-decrement sweep is attempted.

Implementation guidance:
- Declare `op_ret_z` and `op_dec_de` on the extracted z180 core in
  `third_party/z180/z180_core.hpp` and implement them in
  `third_party/z180/z180_core.cpp`, mirroring the structure of the
  existing conditional-return and 16-bit-arithmetic helpers already in
  the core. Register them at `opcodes_[0xC8]` and `opcodes_[0x1B]`
  respectively.
- Wire timing in
  `src/core/cpu/z180_adapter.cpp::current_instruction_tstates()`:
  - `0x1B` joins the existing 6 T-state 16-bit `DEC rr` / `INC rr` group.
  - `0x0C8` is conditional: follow the adapter's existing pattern for
    conditional returns (if one exists) or add a dedicated branch that
    reports 11 T-states when the current Z flag is set and 5 T-states
    when it is clear. Do not collapse to a single constant cost.
- Extend `tests/test_cpu.cpp` with focused scheduled-CPU coverage for both
  opcodes, pinning the flag-preservation, PC advance, SP behavior, and
  the conditional 11/5 T-state split for `RET Z`.
- Extend `tests/test_integration.cpp` with a narrow ROM harness that
  reproduces the observed PacManV8 framebuffer copy path and demonstrates
  the frame loop survives past both `PC 0x0248` and `PC 0x024D`.

Verification:
```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
ctest --test-dir build --output-on-failure
```

Completion summary:
- Re-read `docs/spec/01-cpu.md`, `docs/spec/02-video.md`, `docs/spec/04-io.md`,
  `docs/emulator/03-cpu-and-bus.md`, `docs/emulator/07-implementation-plan.md`,
  `docs/emulator/15-second-pass-audit.md`, and the milestone-29 contract
  before implementation, then kept the change inside the extracted HD64180
  runtime, tests, and workflow docs.
- Extended the extracted z180 core with `op_dec_de` and `op_ret_z` and
  registered them at `opcodes_[0x1B]` and `opcodes_[0xC8]` respectively
  (declarations in `third_party/z180/z180_core.hpp`, implementations in
  `third_party/z180/z180_core.cpp`). `op_dec_de` mirrors the other
  16-bit-pair decrement pattern with wrapping arithmetic and leaves
  flags untouched; `op_ret_z` pops PC from SP only when the Z flag is
  set and otherwise falls through, matching the documented Z80
  conditional-return semantics.
- Wired `current_instruction_tstates()` in
  `src/core/cpu/z180_adapter.cpp` so `0x1B` joins the existing 6
  T-state group alongside `INC HL` (`0x23`), and `0xC8` reports 11
  T-states on the taken path and 5 T-states on the not-taken path by
  inspecting the current Z flag from the register snapshot — matching
  the pattern used for the existing conditional `JR NZ,e` timing.
- Added focused scheduled-path CPU coverage in `tests/test_cpu.cpp`:
  - `scheduled CPU covers DEC DE used by the PacManV8 VDP-B framebuffer
    copy path` pins the typical 0x1234 -> 0x1233 decrement and the
    0x0000 -> 0xFFFF wrap, both at 6 T-states with all flags preserved
    and PC +1.
  - `scheduled CPU covers RET Z used by the PacManV8 VDP-B framebuffer
    copy path` pins both timing branches: the Z=1 taken path pops PC
    from SP, advances SP by two, and costs 11 T-states (staged with the
    documented boot MMU so SP points into SRAM); the Z=0 not-taken path
    advances PC by one, leaves SP untouched, and costs 5 T-states.
    Flags are preserved on both paths.
- Added runtime integration coverage in `tests/test_integration.cpp`
  (`make_pacmanv8_vdp_b_framebuffer_copy_ret_z_and_dec_de_blocker_rom`)
  that reproduces the VDP-B framebuffer copy prologue from M26-M28
  (`JP 0x0243`, `LD DE,0x300C`, `LD A,D`, `OR E` producing A=0x3C and
  Z=0), then executes `RET Z` at the evidence-backed failing
  `PC 0x0248` (not taken because Z=0), falls through padding, and
  executes `DEC DE` at `PC 0x024D` before halting at `PC 0x024E` with
  DE=0x300B and A=0x3C. This proves the frame loop survives past both
  previously failing PCs.
- Kept the milestone strictly narrow: only the two opcodes designated by
  the T007 blocker's static audit were added. No broader conditional-return
  sweep, no generic 16-bit-`DEC rr` sweep, and no VDP-B, palette, or
  compositor changes. Any further opcode gaps surfaced by running the full
  PacManV8 ROM past `PC 0x024D` are explicitly out of M29 scope and must be
  addressed by a future narrow compatibility milestone.
- Verified on the final tree with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`
  `cmake --build build`
  `ctest --test-dir build --output-on-failure`
  All 141 tests pass (138 prior + the 3 added by this task: two focused
  CPU `TEST_CASE`s with sections, and one integration test).

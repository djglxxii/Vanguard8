# M22-T01 — Cover Missing Timed HD64180 VDP-B Bring-Up Opcode for PacManV8

Status: `completed`
Milestone: `22`
Depends on: `M21-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/spec/02-video.md`
- `docs/spec/04-io.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/22.md`
- `docs/emulator/15-second-pass-audit.md`

Scope:
- Reproduce the PacManV8 VDP-B maze render blocker reported in
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T007-vdp-b-maze-render-and-pellet-display.md`.
- Add only the timed opcode coverage needed for base opcode `0x01`
  (`LD BC,nn`) at `PC 0x01FD` on the VDP-B bring-up path.
- Add regressions that pin the direct instruction behavior (BC loaded from
  the two-byte little-endian immediate, PC advanced by three, flags
  untouched) and the ROM-driven timed path without broadening into general
  opcode completion.

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
- Observed abort: `Unsupported timed Z180 opcode 0x1 at PC 0x1FD`

Done when:
- The evidence-backed PacManV8 runtime frame-dump path no longer throws on
  the blocked `PC 0x01FD`.
- Tests lock `LD BC,nn` immediate-load behavior (BC, PC advance, flag
  preservation) in the timed extracted CPU path.
- A runtime/integration case exercises the previously failing `PC 0x01FD` and
  proves the frame loop survives past it.
- Remaining timed-opcode gaps stay explicit and documented; no broader CPU
  completion is attempted.

Verification:
```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
ctest --test-dir build --output-on-failure
```

Completion summary:
- Re-read `docs/spec/01-cpu.md`, `docs/spec/02-video.md`, `docs/spec/04-io.md`,
  `docs/emulator/03-cpu-and-bus.md`, `docs/emulator/07-implementation-plan.md`,
  `docs/emulator/15-second-pass-audit.md`, and the milestone-22 contract
  before implementation, then kept the change inside the extracted HD64180
  runtime, tests, and workflow docs.
- Extended the extracted z180 core with `op_ld_bc_nn` and registered it at
  `opcodes_[0x01]` (declaration in `third_party/z180/z180_core.hpp`,
  implementation in `third_party/z180/z180_core.cpp`), so the timed runtime
  no longer hits `op_unimplemented` for `LD BC,nn`.
- Added `case 0x01` to the 10-T-state group in
  `src/core/cpu/z180_adapter.cpp::current_instruction_tstates()` alongside
  `LD HL,nn`, `LD SP,nn`, and `JP nn`, matching the documented `LD rr,nn`
  timing.
- Added focused scheduled-path CPU coverage in `tests/test_cpu.cpp` that
  programs `LD BC,0x1234` at reset, pre-seeds BC and all AF flag bits, and
  asserts `bc == 0x1234`, `pc == 0x0003`, 10 T-states, and flag preservation.
- Added runtime integration coverage in `tests/test_integration.cpp`
  (`make_pacmanv8_vdp_b_bring_up_blocker_rom`) that jumps to the
  evidence-backed failing `PC 0x01FD`, executes `LD BC,0x5678`, HALTs, and
  proves the frame loop no longer aborts on `0x01` at `0x01FD`.
- Kept the milestone strictly narrow: no broader Z80/Z180 opcode work, no
  VDP-B, palette, or compositor changes. Any later opcode gaps surfaced by
  running the full PacManV8 ROM (e.g. a subsequent `0x79` at `0x23B`) are
  explicitly out of M22 scope and must be addressed by a future narrow
  compatibility milestone.
- Verified on the final tree with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`
  `cmake --build build`
  `ctest --test-dir build --output-on-failure`
  All 126 tests pass (124 prior + the 2 added by this task).

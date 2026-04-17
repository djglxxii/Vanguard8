# M27-T01 — Cover Missing Timed HD64180 VDP-B Framebuffer Copy LD A,D Opcode for PacManV8

Status: `completed`
Milestone: `27`
Depends on: `M26-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/spec/02-video.md`
- `docs/spec/04-io.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/27.md`
- `docs/emulator/15-second-pass-audit.md`

Scope:
- Reproduce the PacManV8 VDP-B framebuffer copy `LD A,D` blocker reported
  in
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T007-vdp-b-maze-render-and-pellet-display.md`
  after the milestone-26 `LD DE,nn` patch.
- Add only the timed opcode coverage needed for base opcode `0x7A`
  (`LD A,D`) at `PC 0x0246` on the VDP-B framebuffer copy loop path.
- Add regressions that pin the direct instruction behavior (A loaded from
  D, PC advanced by one, 4 T-states, flags untouched) and the ROM-driven
  timed path without broadening into general opcode completion.

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
- Observed abort: `Unsupported timed Z180 opcode 0x7A at PC 0x246`

Done when:
- The evidence-backed PacManV8 runtime frame-dump path no longer throws on
  the blocked `PC 0x0246`.
- Tests lock `LD A,D` register-copy behavior (A, PC advance by one,
  4 T-states, flag preservation) in the timed extracted CPU path.
- A runtime/integration case exercises the previously failing `PC 0x0246`
  and proves the frame loop survives past it.
- Remaining timed-opcode gaps stay explicit and documented; no broader CPU
  completion or generic `LD A,r` sweep is attempted.

Verification:
```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
ctest --test-dir build --output-on-failure
```

Completion summary:
- Re-read `docs/spec/01-cpu.md`, `docs/spec/02-video.md`, `docs/spec/04-io.md`,
  `docs/emulator/03-cpu-and-bus.md`, `docs/emulator/07-implementation-plan.md`,
  `docs/emulator/15-second-pass-audit.md`, and the milestone-27 contract
  before implementation, then kept the change inside the extracted HD64180
  runtime, tests, and workflow docs.
- Extended the extracted z180 core with `op_ld_a_d` and registered it at
  `opcodes_[0x7A]` (declaration in `third_party/z180/z180_core.hpp`,
  implementation in `third_party/z180/z180_core.cpp` mirroring the
  existing `op_ld_a_b` / `op_ld_a_c` helpers: `af_.bytes.hi = de_.bytes.hi;`).
- Added `case 0x7A` to the 4-T-state group in
  `src/core/cpu/z180_adapter.cpp::current_instruction_tstates()` alongside
  `LD A,B` (`0x78`) and `LD A,C` (`0x79`), matching the documented
  register-to-register copy timing.
- Added focused scheduled-path CPU coverage in `tests/test_cpu.cpp`
  (`scheduled CPU covers LD A,D used by the PacManV8 VDP-B framebuffer copy
  path`) that pins the documented semantics: A loaded from D, 4 T-states,
  PC advances by one, D unchanged, and all flags preserved.
- Added runtime integration coverage in `tests/test_integration.cpp`
  (`make_pacmanv8_vdp_b_framebuffer_copy_ld_a_d_blocker_rom`) that jumps
  into the framebuffer copy setup via `JP 0x0243`, pre-loads DE via
  `LD DE,0x5678`, executes `LD A,D` at the evidence-backed failing
  `PC 0x0246`, HALTs, and proves the frame loop no longer aborts on
  `0x7A` at `0x0246`.
- Kept the milestone strictly narrow: only the `LD A,D` opcode was added,
  no broader `LD A,r` sweep, no VDP-B, palette, or compositor changes.
  Any later opcode gaps surfaced by running the full PacManV8 ROM are
  explicitly out of M27 scope and must be addressed by a future narrow
  compatibility milestone.
- Verified on the final tree with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`
  `cmake --build build`
  `ctest --test-dir build --output-on-failure`
  All 136 tests pass (134 prior + the 2 added by this task).

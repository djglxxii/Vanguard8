# M25-T01 — Cover Missing Timed HD64180 VDP-B VRAM Seek OR n Opcode for PacManV8

Status: `completed`
Milestone: `25`
Depends on: `M24-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/spec/02-video.md`
- `docs/spec/04-io.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/25.md`
- `docs/emulator/15-second-pass-audit.md`

Scope:
- Reproduce the PacManV8 VDP-B VRAM seek `OR n` blocker reported in
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T007-vdp-b-maze-render-and-pellet-display.md`
  after the milestone-24 `LD A,B` patch.
- Add only the timed opcode coverage needed for base opcode `0xF6`
  (`OR n`) at `PC 0x0241` on the VDP-B VRAM seek path.
- Add regressions that pin the direct instruction behavior (A <- A | n, PC
  advanced by two, 7 T-states) and Z80-standard flag result (S from bit 7
  of the result, Z if result is zero, H cleared, P/V even-parity, N cleared,
  C cleared) without broadening into a general logical-op table or full
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
- Observed abort: `Unsupported timed Z180 opcode 0xF6 at PC 0x241`

Done when:
- The evidence-backed PacManV8 runtime frame-dump path no longer throws on
  the blocked `PC 0x0241`.
- Tests lock `OR n` behavior (A <- A | n, PC advance by two, 7 T-states,
  and the Z80-standard S/Z/H=0/P-parity/N=0/C=0 flag result) in the timed
  extracted CPU path.
- A runtime/integration case exercises the previously failing `PC 0x0241`
  and proves the frame loop survives past it.
- Remaining timed-opcode gaps stay explicit and documented; no broader CPU
  completion or generic logical-op surface is attempted.

Verification:
```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
ctest --test-dir build --output-on-failure
```

Completion summary:
- Re-read `docs/spec/01-cpu.md`, `docs/spec/02-video.md`, `docs/spec/04-io.md`,
  `docs/emulator/03-cpu-and-bus.md`, `docs/emulator/07-implementation-plan.md`,
  `docs/emulator/15-second-pass-audit.md`, and the milestone-25 contract
  before implementation, then kept the change inside the extracted HD64180
  runtime, tests, and workflow docs.
- Extended the extracted z180 core with `op_or_n` and registered it at
  `opcodes_[0xF6]` (declaration in `third_party/z180/z180_core.hpp`,
  implementation in `third_party/z180/z180_core.cpp` mirroring `op_and_n`
  but with `|` and with `H` cleared instead of set).
- Added `case 0xF6` to the 7-T-state group in
  `src/core/cpu/z180_adapter.cpp::current_instruction_tstates()` alongside
  `LD r,n` and `AND n`, matching the documented immediate logical-op timing.
- Added focused scheduled-path CPU coverage in `tests/test_cpu.cpp` with
  three sections that lock the documented flag result: a typical nonzero
  result (Z80-standard S=0, Z=0, H=0, P even-parity, N=0, C=0), a zero
  result (Z=1 and parity=1, S/H/N/C all cleared even when pre-set), and a
  bit-7 result (S=1, odd-parity clears P/V).
- Added runtime integration coverage in `tests/test_integration.cpp`
  (`make_pacmanv8_vdp_b_vram_seek_or_n_blocker_rom`) that jumps into the
  VDP-B seek helper just before the blocker, pre-loads A via `LD A,n`,
  executes `OR 0x0C` at the evidence-backed failing `PC 0x0241`, HALTs, and
  proves the frame loop no longer aborts on `0xF6` at `0x0241`.
- Kept the milestone strictly narrow: only the `OR n` opcode was added, no
  broader `OR r` or generic logical-op sweep, no VDP-B, palette, or
  compositor changes. Any later opcode gaps surfaced by running the full
  PacManV8 ROM are explicitly out of M25 scope and must be addressed by a
  future narrow compatibility milestone.
- Verified on the final tree with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`
  `cmake --build build`
  `ctest --test-dir build --output-on-failure`
  All 132 tests pass (130 prior + the 2 added by this task; the focused CPU
  test exercises 3 sections under a single Catch2 `TEST_CASE`).

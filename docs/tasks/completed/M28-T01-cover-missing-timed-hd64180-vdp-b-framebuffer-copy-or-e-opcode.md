# M28-T01 — Cover Missing Timed HD64180 VDP-B Framebuffer Copy OR E Opcode for PacManV8

Status: `completed`
Milestone: `28`
Depends on: `M27-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/spec/02-video.md`
- `docs/spec/04-io.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/28.md`
- `docs/emulator/15-second-pass-audit.md`

Scope:
- Reproduce the PacManV8 VDP-B framebuffer copy `OR E` blocker reported
  in
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T007-vdp-b-maze-render-and-pellet-display.md`
  after the milestone-27 `LD A,D` patch.
- Add only the timed opcode coverage needed for base opcode `0xB3`
  (`OR E`) at `PC 0x0247` on the VDP-B framebuffer copy loop path.
- Add regressions that pin the direct instruction behavior (A <- A | E;
  S from bit 7, Z from zero result, H=0, P/V from even parity, N=0,
  C=0; PC advanced by one, 4 T-states) and the ROM-driven timed path
  without broadening into general opcode completion.

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
- Observed abort: `Unsupported timed Z180 opcode 0xB3 at PC 0x247`

Done when:
- The evidence-backed PacManV8 runtime frame-dump path no longer throws on
  the blocked `PC 0x0247`.
- Tests lock `OR E` register logical-OR behavior (A result, flag result
  per documented S/Z/H/P/N/C rules, PC advance by one, 4 T-states) in
  the timed extracted CPU path.
- A runtime/integration case exercises the previously failing `PC 0x0247`
  and proves the frame loop survives past it.
- Remaining timed-opcode gaps stay explicit and documented; no broader CPU
  completion or generic `OR r` sweep is attempted.

Verification:
```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
ctest --test-dir build --output-on-failure
```

Completion summary:
- Re-read `docs/spec/01-cpu.md`, `docs/spec/02-video.md`, `docs/spec/04-io.md`,
  `docs/emulator/03-cpu-and-bus.md`, `docs/emulator/07-implementation-plan.md`,
  `docs/emulator/15-second-pass-audit.md`, and the milestone-28 contract
  before implementation, then kept the change inside the extracted HD64180
  runtime, tests, and workflow docs.
- Extended the extracted z180 core with `op_or_e` and registered it at
  `opcodes_[0xB3]` (declaration in `third_party/z180/z180_core.hpp`,
  implementation in `third_party/z180/z180_core.cpp` mirroring the
  existing `op_or_n` helper but sourcing the operand from `de_.bytes.lo`
  instead of an immediate).
- Added `case 0xB3` to the 4-T-state group in
  `src/core/cpu/z180_adapter.cpp::current_instruction_tstates()` alongside
  `LD A,B` (`0x78`), `LD A,C` (`0x79`), and `LD A,D` (`0x7A`), matching
  the documented register-to-accumulator logical-OR timing.
- Added focused scheduled-path CPU coverage in `tests/test_cpu.cpp`
  (`scheduled CPU covers OR E used by the PacManV8 VDP-B framebuffer copy
  path`) with three sections that lock the documented flag result: a
  typical nonzero result (Z80-standard S=0, Z=0, H=0, P even-parity,
  N=0, C=0), a zero result (Z=1 and parity=1, S/H/N/C all cleared), and
  a bit-7 result (S=1, odd-parity clears P/V).
- Added runtime integration coverage in `tests/test_integration.cpp`
  (`make_pacmanv8_vdp_b_framebuffer_copy_or_e_blocker_rom`) that jumps
  into the framebuffer copy setup via `JP 0x0243`, pre-loads DE via
  `LD DE,0x300C`, copies D into A via `LD A,D`, executes `OR E` at the
  evidence-backed failing `PC 0x0247`, HALTs, and proves the frame loop
  no longer aborts on `0xB3` at `0x0247`.
- Kept the milestone strictly narrow: only the `OR E` opcode was added,
  no broader `OR r` sweep, no VDP-B, palette, or compositor changes.
  Any later opcode gaps surfaced by running the full PacManV8 ROM are
  explicitly out of M28 scope and must be addressed by a future narrow
  compatibility milestone.
- Verified on the final tree with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`
  `cmake --build build`
  `ctest --test-dir build --output-on-failure`
  All 138 tests pass (136 prior + the 2 added by this task; the focused
  CPU test exercises 3 sections under a single Catch2 `TEST_CASE`).

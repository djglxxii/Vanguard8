# M20-T01 — Cover Missing Timed HD64180 Palette Opcode for Pac-Man Bring-Up

Status: `completed`
Milestone: `20`
Depends on: `M19-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/spec/03-audio.md`
- `docs/spec/04-io.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/20.md`
- `docs/emulator/15-second-pass-audit.md`

Scope:
- Reproduce the Pac-Man palette swatch blocker captured under
  `/home/djglxxii/src/pacman/vanguard8_port/tests/evidence/T004-palette/`.
- Add only the timed opcode coverage needed for opcode `0x05` at `PC 0x02B9`
  in the `VCLK 4000` runtime path.
- Add regressions that pin the direct instruction behavior and the ROM-driven
  timed path without broadening into general opcode completion or palette
  surface changes.

Done when:
- The evidence-backed `VCLK 4000` runtime path no longer throws on the blocked
  `PC 0x02B9`.
- Tests lock the newly covered timed opcode behavior and the palette swatch
  runtime path.
- The already-passing `VCLK: off` palette path remains covered.

Completion summary:
- Re-read `docs/spec/01-cpu.md`, `docs/spec/03-audio.md`,
  `docs/spec/04-io.md`, `docs/emulator/03-cpu-and-bus.md`,
  `docs/emulator/07-implementation-plan.md`,
  `docs/emulator/15-second-pass-audit.md`, and the milestone-20 contract
  before implementation, then kept the change inside the extracted HD64180
  runtime, tests, and workflow docs.
- Extended the extracted core and timed instruction table only for the
  evidence-backed blocker: base opcode `0x05` (`DEC B`) at `PC 0x02B9`.
- Added scheduled-path CPU coverage in `tests/test_cpu.cpp` that locks `DEC B`
  timing and documented Z80-compatible flag behavior.
- Added runtime integration coverage in `tests/test_integration.cpp` that jumps
  to the evidence-backed failing `PC 0x02B9` and proves the path survives both
  the already-passing stopped-`VCLK` case and active `VCLK 4000` timing.
- Verified on the final tree with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`
  `cmake --build build`
  `ctest --test-dir build --output-on-failure`

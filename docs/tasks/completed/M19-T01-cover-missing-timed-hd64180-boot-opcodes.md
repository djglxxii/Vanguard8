# M19-T01 — Cover Missing Timed HD64180 Boot Opcodes for Pac-Man Bring-Up

Status: `completed`
Milestone: `19`
Depends on: `M18-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/spec/04-io.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/19.md`
- `docs/emulator/15-second-pass-audit.md`

Scope:
- Reproduce the Pac-Man boot/background blocker captured under
  `/home/djglxxii/src/pacman/vanguard8_port/tests/evidence/T002-boot-background/`.
- Add only the timed opcode coverage and minimal interrupt-mode handling needed
  for `LD B,n` (`0x06`) at `PC 0x0140` and `IM 1` (`ED 0x56`) at `PC 0x0314`.
- Add regressions that pin the direct instruction behavior and the ROM-driven
  timed path without broadening into general opcode completion.

Done when:
- The evidence-backed timed runtime path no longer throws on the blocked PCs
  `0x0140` and `0x0314`.
- Tests lock `LD B,n` immediate-load behavior and `IM 1` handling in the timed
  extracted CPU path.
- Existing documented interrupt semantics, especially mode-independent `INT1`,
  remain covered.

Completion summary:
- Re-read `docs/spec/01-cpu.md`, `docs/spec/04-io.md`,
  `docs/emulator/03-cpu-and-bus.md`, `docs/emulator/07-implementation-plan.md`,
  `docs/emulator/15-second-pass-audit.md`, and the milestone-19 contract
  before implementation, then kept the change inside the extracted HD64180
  runtime, tests, and workflow docs.
- Extended the extracted core and the timed instruction table only for the
  observed Pac-Man boot/background blockers: base opcode `0x06` (`LD B,n`) and
  ED opcode `0x56` (`IM 1`).
- Added scheduled-path CPU regressions in `tests/test_cpu.cpp` that lock the
  `LD B,n` immediate-load timing/behavior and prove `IM 1` still leaves `INT1`
  on the documented mode-independent vectored path.
- Added a runtime integration ROM in `tests/test_integration.cpp` that jumps to
  the evidence-backed failing PCs `0x0140` and `0x0314`, proving the frame loop
  no longer aborts on the Pac-Man boot/background opcode gap.
- Verified on the final tree with:
  `cmake --build build --target vanguard8_tests`
  `ctest --test-dir build --output-on-failure`

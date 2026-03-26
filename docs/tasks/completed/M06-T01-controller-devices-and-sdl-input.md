# M06-T01 — Add Controller Devices and SDL Input Mapping

Status: `completed`
Milestone: `6`
Depends on: `M05-T03`

Implements against:
- `docs/spec/04-io.md`
- `docs/emulator/01-build.md`

Scope:
- Add controller port devices for ports `0x00` and `0x01`.
- Map SDL keyboard and gamepad input to those ports.
- Preserve active-low semantics.

Done when:
- Player 1 and player 2 inputs remain independent.
- Tests lock active-low controller reads.

Completion summary:
- Implemented against `docs/spec/04-io.md`,
  `docs/emulator/01-build.md`, and `docs/emulator/milestones/06.md`.
- Added active-low controller ports in `src/core/io/controller.*`, routed them
  through `src/core/bus.*` on ports `0x00` and `0x01`, and exposed them to the
  frontend through `src/frontend/input.*`.
- Added deterministic SDL-style keyboard and gamepad binding names for both
  players, keeping player 1 and player 2 independent while preserving the
  documented active-low bit layout.
- Locked controller behavior with `tests/test_input.cpp`.
- Verified with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`,
  `cmake --build build`, and `ctest --test-dir build --output-on-failure`.

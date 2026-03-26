# M06-T02 — Add Config Creation and ROM Open Flow

Status: `completed`
Milestone: `6`
Depends on: `M06-T01`

Implements against:
- `docs/emulator/01-build.md`
- `docs/emulator/07-implementation-plan.md`

Scope:
- Create/load the config file.
- Add ROM file open and drag-and-drop support.
- Reject oversized images and invalid formats cleanly.

Done when:
- Config is created with defaults on first run.
- ROM loading validates size and format before execution.

Completion summary:
- Implemented against `docs/emulator/01-build.md`,
  `docs/emulator/07-implementation-plan.md`, and
  `docs/emulator/milestones/06.md`.
- Expanded `src/core/config.*` to persist milestone-6 display settings, input
  bindings, and recent ROM history, and added `src/frontend/rom_loader.*` for
  validated raw cartridge-image loading.
- Added ROM attachment in `src/core/emulator.*` so the frontend/headless paths
  can load a validated cartridge image into the current session rather than
  only checking the file and discarding it.
- Locked oversized and invalid ROM rejection in `tests/test_input.cpp` and
  config round-trip behavior in `tests/test_scaffold.cpp`.
- Verified with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`,
  `cmake --build build`, and `ctest --test-dir build --output-on-failure`.

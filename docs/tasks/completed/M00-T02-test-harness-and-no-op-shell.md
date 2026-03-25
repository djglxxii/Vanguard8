# M00-T02 — Wire Test Harness, Logging/Config Scaffolding, and No-Op Shell

Status: `completed`
Milestone: `0`
Depends on: `M00-T01`

Implements against:
- `docs/emulator/01-build.md`
- `docs/emulator/07-implementation-plan.md`

Scope:
- Wire Catch2 into `ctest`.
- Add logging and config-loading scaffolding.
- Add frontend/headless entrypoints that can start, print build info, and exit
  cleanly.

Done when:
- `ctest` runs successfully from the build tree.
- A no-op headless executable can start and exit cleanly.
- The scaffold leaves milestone 1 memory/bus work untouched.

Completion summary:
- Implemented against `docs/emulator/01-build.md` and
  `docs/emulator/07-implementation-plan.md`.
- Added a no-op `vanguard8_core`, `vanguard8_frontend`, and
  `vanguard8_headless` scaffold, config/logging bootstrap code, and a Catch2
  smoke-test harness wired into `ctest`.
- Verified with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`,
  `cmake --build build`, `ctest --test-dir build --output-on-failure`, and
  `./build/src/vanguard8_headless`.

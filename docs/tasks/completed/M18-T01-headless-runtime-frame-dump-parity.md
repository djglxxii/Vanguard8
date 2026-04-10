# M18-T01 — Make Headless Frame Dumps Match Runtime Output

Status: `completed`
Milestone: `18`
Depends on: `M17-T01`

Implements against:
- `docs/spec/02-video.md`
- `docs/emulator/04-video.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/18.md`

Scope:
- Fix the headless frame-dump path so it captures the actual runtime compositor
  frame for a running ROM session.
- Add regression coverage for runtime dump dimensions and dump/compositor
  parity.
- Keep any preserved fixture-only dump flow explicit instead of overloading the
  same runtime command surface.

Done when:
- Runtime frame dumps are reliable review artifacts for both `256x212` and
  `512x212` scenes.
- The completion summary explains any CLI/documentation change needed to keep
  fixture and runtime capture paths distinct.

Completion summary:
- Re-read `docs/spec/02-video.md`, `docs/emulator/04-video.md`,
  `docs/emulator/07-implementation-plan.md`, and the milestone-18 contract,
  then kept the change inside the headless/frontend surface instead of
  touching the renderer or showcase ROM.
- Updated `src/frontend/headless.cpp` so `--dump-frame path.ppm` now captures
  the current composed runtime frame after the requested ROM/replay session,
  rather than unconditionally taking the built-in fixture path.
- Preserved fixture capture behind an explicit `--dump-fixture path.ppm`
  switch, so runtime inspection and fixture inspection are no longer
  overloaded onto the same CLI option.
- Added narrow frame-dump helpers that route both runtime and fixture captures
  through the same `Display` PPM path, keeping dump width, height, and digest
  aligned with the actual uploaded RGB frame.
- Added direct frontend regressions in `tests/test_headless.cpp` covering:
  a ROM-driven `256x212` runtime dump, a mixed-width `512x212` runtime dump,
  and the explicit fixture-only dump path.
- Updated `tests/CMakeLists.txt` so the headless frontend implementation is
  compiled into the test binary, letting the repo verify the real CLI entry
  point without relying only on an external process wrapper.
- Updated `docs/emulator/00-overview.md` and
  `docs/emulator/02-emulation-loop.md` so the documented headless CLI now
  matches the implemented runtime/fixture split and the binary PPM dump
  surface.
- Verified on the final tree with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`
  `cmake --build build`
  `ctest --test-dir build --output-on-failure`

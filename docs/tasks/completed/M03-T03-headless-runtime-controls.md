# M03-T03 — Add Deterministic Headless Runtime Controls

Status: `completed`
Milestone: `3`
Depends on: `M03-T02`

Implements against:
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/07-implementation-plan.md`

Scope:
- Add headless run, pause, and single-frame step behavior.
- Add deterministic frame and scanline accounting.
- Keep the implementation independent of SDL or ImGui UI.

Done when:
- A headless test can run for N frames with stable logs.
- Runtime controls operate at scheduler boundaries, not ad hoc delays.

Completion summary:
- Implemented against `docs/emulator/02-emulation-loop.md`,
  `docs/emulator/07-implementation-plan.md`, and `docs/emulator/milestones/03.md`.
- Added deterministic headless run, pause, frame-count run, and single-frame
  step behavior in `src/core/emulator.*` and exposed simple CLI controls in
  `src/frontend/headless.cpp`.
- Added stable event-log digest reporting in the headless binary and covered
  pause/run/frame-step behavior in `tests/test_scheduler.cpp`.
- Verified with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`,
  `cmake --build build`, and `ctest --test-dir build --output-on-failure`.

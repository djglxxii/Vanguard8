# M06-T03 — Add Minimal Frontend UX Controls

Status: `completed`
Milestone: `6`
Depends on: `M06-T02`

Implements against:
- `docs/emulator/01-build.md`
- `docs/emulator/07-implementation-plan.md`

Scope:
- Add recent ROM tracking.
- Add fullscreen toggle, scale modes, and frame pacing.
- Keep the frontend limited to the documented minimal user-facing flow.

Done when:
- A user can load a ROM, get video, and control software.
- Basic presentation controls work without entering later-milestone UI scope.

Completion summary:
- Implemented against `docs/emulator/01-build.md`,
  `docs/emulator/07-implementation-plan.md`, and
  `docs/emulator/milestones/06.md`.
- Updated `src/frontend/app.cpp` and `src/frontend/headless.cpp` with the
  milestone-6 minimal session flow: ROM open/recent ROM selection, display
  scale/aspect/fullscreen/frame-pacing settings, controller-input application,
  and known-frame output through the existing video pipeline.
- Kept the frontend intentionally narrow to the repo's current deterministic
  runtime shape: ROM and input are now usable through the frontend/headless
  entry points without pulling in later-milestone debugger or audio work.
- Sanity-checked the binaries with a valid 16 KB ROM image and confirmed both
  frontend and headless paths report controller port state and the same frame
  digest.
- Verified with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`,
  `cmake --build build`, and `ctest --test-dir build --output-on-failure`.

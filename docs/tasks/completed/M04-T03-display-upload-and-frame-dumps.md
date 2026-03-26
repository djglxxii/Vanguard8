# M04-T03 — Add Display Upload and Headless Frame Dumps

Status: `completed`
Milestone: `4`
Depends on: `M04-T02`

Implements against:
- `docs/emulator/01-build.md`
- `docs/emulator/04-video.md`

Scope:
- Add SDL/OpenGL texture upload for the rendered frame.
- Add headless frame-dump output for regression assets.
- Keep display behavior aligned with the single-VDP Graphic 4 path.

Done when:
- A known frame matches between SDL/OpenGL and headless output.
- Regression image generation is available in headless mode.

Completion summary:
- Implemented against `docs/emulator/01-build.md`,
  `docs/emulator/04-video.md`, and `docs/emulator/milestones/04.md`.
- Added the deterministic `Display` upload path in `src/frontend/display.*`
  plus the shared fixture generator in `src/frontend/video_fixture.*`, then
  wired both the frontend and headless binaries to consume the same rendered
  frame.
- Added headless PPM dumps in `src/frontend/headless.cpp` and frontend upload
  verification in `src/frontend/app.cpp`; both produce the same known-frame
  digest for the milestone-4 single-VDP path.
- Added `tests/test_video.cpp` coverage for headless-versus-upload-frame
  equivalence and verified with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`,
  `cmake --build build`, and `ctest --test-dir build --output-on-failure`.

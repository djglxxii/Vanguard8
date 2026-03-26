# M05-T01 — Add the Second VDP and the Compositor

Status: `completed`
Milestone: `5`
Depends on: `M04-T03`

Implements against:
- `docs/spec/00-overview.md`
- `docs/spec/02-video.md`
- `docs/emulator/04-video.md`

Scope:
- Add a second V9938 instance.
- Implement the VDP-A-over-VDP-B mux with VDP-A palette index `0`
  transparency.
- Add layer toggles usable by tests and later debugger UI.

Done when:
- Compositor tests prove transparency and nonzero override behavior.
- The dual-VDP path stays synchronized per frame.

Completion summary:
- Implemented against `docs/spec/00-overview.md`,
  `docs/spec/02-video.md`, `docs/emulator/04-video.md`, and
  `docs/emulator/milestones/05.md`.
- Added a real dual-VDP compositor in `src/core/video/compositor.hpp` with
  VDP-A color-0 transparency, nonzero override behavior, and per-layer toggles
  for VDP-A/VDP-B background and sprite visibility.
- Updated the shared video fixture and frontend/headless upload paths to render
  through the dual-layer compositor instead of the milestone-4 single-VDP path.
- Locked compositing and layer-toggle behavior in `tests/test_video.cpp`.
- Verified with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`,
  `cmake --build build`, and `ctest --test-dir build --output-on-failure`.

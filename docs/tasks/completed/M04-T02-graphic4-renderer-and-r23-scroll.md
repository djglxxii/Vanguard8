# M04-T02 — Implement Graphic 4 Rendering and R#23 Vertical Scroll

Status: `completed`
Milestone: `4`
Depends on: `M04-T01`

Implements against:
- `docs/spec/02-video.md`
- `docs/emulator/04-video.md`

Scope:
- Implement the Graphic 4 scanline renderer for 256x212 4 bpp output.
- Implement vertical scroll via `R#23`.
- Do not implement `R#26`/`R#27`.

Done when:
- Multiple scanlines render with correct framebuffer addressing.
- `R#23` wraps correctly and is locked by tests.

Completion summary:
- Implemented against `docs/spec/02-video.md`,
  `docs/emulator/04-video.md`, and `docs/emulator/milestones/04.md`.
- Added the single-VDP Graphic 4 scanline/frame renderer in
  `src/core/video/v9938.*` and kept the implementation inside the milestone
  boundary: vertical scroll via `R#23` is present while `R#26`/`R#27`,
  dual-VDP compositing, and the command engine remain out of scope.
- Added tests in `tests/test_video.cpp` covering cross-scanline VRAM
  addressing and `R#23` wrap behavior so the documented renderer semantics are
  pinned to the spec.
- Verified with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`,
  `cmake --build build`, and `ctest --test-dir build --output-on-failure`.

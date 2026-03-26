# M05-T02 — Implement Sprites and Covered Status Flags

Status: `completed`
Milestone: `5`
Depends on: `M05-T01`

Implements against:
- `docs/spec/02-video.md`
- `docs/emulator/04-video.md`

Scope:
- Add the sprite mode path required for Graphic 4 and Graphic 3 usage.
- Implement the covered collision and overflow flags.
- Limit behavior to documented cases required by the milestone.

Done when:
- Sprite rendering appears in the composited output path.
- Tests lock the covered collision and overflow cases.

Completion summary:
- Implemented against `docs/spec/02-video.md`,
  `docs/emulator/04-video.md`, and `docs/emulator/milestones/05.md`.
- Extended `src/core/video/v9938.*` with the covered milestone-5 Sprite
  Mode-2 path: per-scanline sprite overlay, 8-sprite overflow handling,
  same-chip collision detection, and collision coordinate capture in
  `S#3-S#6`.
- Kept the implementation intentionally narrow to the repo's documented
  Graphic 4 sprite layout examples for fixtures and tests, without inventing
  a broader table-relocation model that the repo docs do not yet pin down.
- Added sprite overflow, collision, and sprite-visible compositing coverage in
  `tests/test_video.cpp`.
- Verified with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`,
  `cmake --build build`, and `ctest --test-dir build --output-on-failure`.

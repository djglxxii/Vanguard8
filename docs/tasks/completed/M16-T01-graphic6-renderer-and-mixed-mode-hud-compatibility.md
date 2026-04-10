# M16-T01 — Add the Narrow Graphic 6 Renderer and Mixed-Mode HUD Compatibility

Status: `completed`
Milestone: `16`
Depends on: `M15-T01`

Implements against:
- `docs/spec/02-video.md`
- `docs/spec/04-io.md`
- `docs/emulator/04-video.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/16.md`

Scope:
- Implement the documented `Graphic 6` background renderer.
- Add mixed-mode coverage for `VDP-A Graphic 6` composited over
  `VDP-B Graphic 4`.
- Document the intentionally deferred high-resolution mode gaps that remain out
  of scope.

Done when:
- The emulator can render deterministic `Graphic 6` frames with dedicated test
  coverage.
- The completion summary explains the supported `Graphic 6` surface and the
  remaining deferred `Graphic 5`/`Graphic 7` work.

Completion summary:
- Re-read `docs/spec/02-video.md`, `docs/spec/04-io.md`,
  `docs/emulator/04-video.md`, `docs/emulator/08-compatibility-audit.md`, and
  the milestone-16 contract before implementation, then kept the change narrow
  to the documented `Graphic 6` background path plus the minimum frame-pipeline
  work needed to expose it.
- Extended `src/core/video/v9938.*` with `Graphic 6` mode decode
  (`M5=1, M4=0, M3=1, M2=0, M1=0`), `512x212` background rendering, widened
  line buffers to the maximum horizontal mode width, and preserved the existing
  `Graphic 3`/`Graphic 4` sprite surface instead of inventing broader
  high-resolution sprite behavior.
- Updated `src/core/video/compositor.hpp` to emit `256x212` or `512x212`
  frames depending on the widest active VDP layer, with the mixed-width rule
  locked to double-width sampling for 256-pixel layers when composited against
  a 512-pixel layer.
- Added the minimal `src/frontend/` seam required by the milestone:
  `Display` now accepts `256x212` and `512x212` RGB frames, and the OpenGL
  presenter reallocates textures to the uploaded frame size so headless and
  desktop readback continue to work without broader frontend refactoring.
- Added regression coverage in `tests/test_video.cpp` for `Graphic 6`
  framebuffer addressing, `512`-wide RGB output, and `VDP-A Graphic 6` over
  `VDP-B Graphic 4` composition, plus `tests/test_frontend_backends.cpp`
  coverage for `512x212` display upload and presenter readback.
- Updated `docs/spec/02-video.md`, `docs/emulator/04-video.md`,
  `docs/emulator/07-implementation-plan.md`, and `docs/emulator/milestones/16.md`
  to lock the mixed-width compositing rule, the verified `Graphic 6` mode
  flags, and the intentionally deferred `Graphic 5`/`Graphic 7` and
  `Graphic 6` sprite/command-engine gaps.
- Verified on the final tree with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`
  `cmake --build build`
  `ctest --test-dir build --output-on-failure`

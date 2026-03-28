# M13-T03 — Add Sprite Mode 1 plus Graphic 1 and Graphic 2 Renderer Coverage

Status: `completed`
Milestone: `13`
Depends on: `M13-T02`

Implements against:
- `docs/spec/02-video.md`
- `docs/emulator/04-video.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/11-pre-gui-audit.md`
- `docs/emulator/milestones/13.md`

Scope:
- Implement Sprite Mode 1.
- Add Graphic 1 and Graphic 2 background renderers.
- Add targeted fixtures representative of title-screen, menu, and startup paths
  likely to appear in showcase ROMs and early real software.

Done when:
- Covered Graphic 1/2 scenes render correctly with Sprite Mode 1 behavior.
- The repo can validate the most likely tile-mode paths needed for first-wave
  ROM bring-up without relying on unsupported backdrop output.

Completion summary:
- Added covered Graphic 1 and Graphic 2 background renderers in
  `src/core/video/v9938.*`, including the grouped color-table fetch path for
  Graphic 1 and the three-bank tile/color layout for Graphic 2.
- Added Sprite Mode 1 rendering with the documented 4-sprite-per-scanline
  limit, single-color SAT byte-3 handling, and collision/status behavior while
  reusing the covered sprite size and magnification controls from the earlier
  sprite compatibility pass.
- Added regression coverage in `tests/test_video.cpp` for Graphic 1 tile fetch,
  Graphic 2 banked fetch, and Sprite Mode 1 rendering/overflow/collision, and
  updated `docs/emulator/04-video.md` to reflect the expanded covered mode set.

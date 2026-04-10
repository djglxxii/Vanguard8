# Showcase Milestone 7 Checkpoints

Milestone 7 adds one post-freeze late-loop attract scene that demonstrates the
new `Graphic 6` compatibility surface on top of the already accepted showcase
loop. The earlier milestone manifests remain authoritative for the title,
compositing, tile, sprite, command, audio, and bank-6 system-validation phases;
this file freezes the additional bank-7 mixed-mode review surface only.

Traceability:
- `docs/spec/02-video.md`
- `docs/spec/04-io.md`
- `docs/emulator/milestones/16.md`
- `showcase/docs/milestones/07.md`
- `showcase/src/showcase.asm`

Frozen earlier manifests:
- `showcase/tests/milestone-01-checkpoints.md`
- `showcase/tests/milestone-02-checkpoints.md`
- `showcase/tests/milestone-03-checkpoints.md`
- `showcase/tests/milestone-04-checkpoints.md`
- `showcase/tests/milestone-05-checkpoints.md`
- `showcase/tests/milestone-06-checkpoints.md`

Verification commands:
- Build:
  `python3 showcase/tools/package/build_showcase.py`
- Final late-loop run:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 1680 --trace build/showcase/m7.trace --hash-frame 1679 --symbols build/showcase/showcase.sym`

Checkpoint notes:
- `m7-starfield-hud-frame1679`
  Purpose:
  prove the ROM reaches the final bank-7 attract tableau after the accepted
  bank-6 system-validation scene and holds a stable mixed-mode review frame
  while the background scroll cadence is active
  Command:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 1680 --trace build/showcase/m7.trace --hash-frame 1679 --symbols build/showcase/showcase.sym`
  Expected visual:
  a vertically scrolling `VDP-B Graphic 4` starfield behind three fixed
  `VDP-A Graphic 6` HUD panels, with a left `SCORE 123450` readout, a centered
  segmented status meter, and a right `LIVES` panel with three ship icons
  Expected frame hash:
  `c191c3b98a3925e3a3c73f2c2dbfc6ed35b5ed6a67b0875dd9aae050b28f7f0c`
  Expected ROM size:
  `475136`
  Expected event-log digest:
  `11007163357591775607`

Review cues:
- `prt0_stage_75` switches to bank `7` and loads the final scene; the
  recurring scroll cadence then advances through
  `prt0_starfield_tick_0` through `prt0_starfield_tick_11`.
- The top HUD panels must remain fixed in place while the star rows on
  `VDP-B` visibly move between timer ticks via `R#23`.
- The `Graphic 6` HUD is drawn from a compact direct-VRAM bitmap glyph/icon
  set rather than from `Text 1` or `Text 2`.
- Audio is intentionally silenced before the heavy bank-7 visual setup so the
  milestone remains a visual compatibility pass rather than a new audio task.

Trace cues:
- The wrapper trace at `build/showcase/m7.trace` is still a short symbol-aware
  bootstrap trace. Treat it as a symbol smoke check, not a late-scene capture.
- The full 1680-frame command above is the authoritative proof that the ROM
  reaches and stabilizes the final bank-7 mixed-mode scene.

# SR02-T01 — Add Generated-Asset Guidance Plus Title and Dual-VDP Compositing Scenes

Status: `completed`
Milestone: `2`
Depends on: `SR01-T01`

Implements against:
- `docs/spec/02-video.md`
- `showcase/docs/showcase-rom-plan.md`
- `showcase/docs/showcase-implementation-plan.md`
- `showcase/docs/milestones/02.md`

Scope:
- Add the generated-asset guideline and provenance-note docs needed by the ROM.
- Add the title/identity scene.
- Add the dual-VDP compositing scene using coherent generated assets.
- Document scene checkpoints for human review.

Done when:
- The ROM has a coherent title scene and compositing scene.
- Reviewers can identify the compositing behavior from captures without relying
  on verbal explanation alone.
- The completed task summary records the generated-asset conventions now in use.

Completion summary:
- Added `showcase/docs/asset-guidelines.md` and
  `showcase/assets/provenance/milestone-02-scenes.md` to lock the first
  generated-asset conventions: assembly-authored Graphic 4 primitives, small
  repeated palettes, VDP-A color-0 transparency for compositing scenes, and
  provenance notes tied directly to the ROM source.
- Replaced the milestone-1 blinking bank loop in `showcase/src/showcase.asm`
  with a timed milestone-2 scene flow: bank 0 draws a stable title / identity
  screen, a PRT0-driven fixed-ROM handler chain advances after a deterministic
  dwell, then bank 1 and bank 2 initialize the VDP-A overlay and VDP-B reveal
  background for the compositing scene.
- The title scene now presents a dark slate field with mint rails and a blocky
  green/mint `V8` crest, while the compositing scene uses a transparent VDP-A
  frame/badge over blue VDP-B reveal panels so the layer ordering is readable
  at a glance.
- Added `showcase/tests/milestone-02-checkpoints.md` and updated the showcase
  workspace READMEs so reviewers have concrete capture commands and frame-hash
  checkpoints for both the title scene (`frame 119`) and the stable
  compositing scene (`frame 135` onward, including `frame 479` for the
  milestone contract run).
- Verification commands passed:
  `python3 showcase/tools/package/build_showcase.py`
  `python3 showcase/tools/package/run_showcase_headless.py --frames 480 --trace build/showcase/m2.trace --hash-frame 479 --symbols build/showcase/showcase.sym`
- Current milestone-2 limitation: the timed transition is driven by the
  HD64180 internal PRT0 vectored interrupt path, but the ROM must idle in a
  tight `JP` loop rather than `HALT` because the current runtime does not wake
  internal timer interrupts out of `HALT`.

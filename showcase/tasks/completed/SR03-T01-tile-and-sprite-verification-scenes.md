# SR03-T01 — Add Tile-Mode and Sprite Verification Scenes

Status: `completed`
Milestone: `3`
Depends on: `SR02-T01`

Implements against:
- `docs/spec/02-video.md`
- `docs/spec/04-io.md`
- `showcase/docs/showcase-rom-plan.md`
- `showcase/docs/showcase-implementation-plan.md`
- `showcase/docs/milestones/03.md`

Scope:
- Add the tile-mode presentation scene.
- Add the sprite capability scene.
- Add the generated assets and checkpoint notes required to review those
  scenes.

Done when:
- Reviewers can inspect stable tile-mode and sprite captures and understand the
  intended validation target.
- The task summary records which visual cues are meant to expose sprite and
  tile failures.

Completion summary:
- Implemented against `docs/spec/02-video.md` and `showcase/docs/milestones/03.md`
  by extending the milestone-2 timed attract flow with two new deterministic
  review states: a fixed-ROM Graphic 2 menu scene with a single Sprite Mode 1
  cursor, and a final sprite-lab scene reached by switching `BBR` to a new
  third bank page and calling its scene-init entry at logical `0x4000`.
- The tile scene now programs the documented Graphic 2 table layout on VDP-A,
  keeps VDP-B disabled so the checkpoint is visually unambiguous, and uses a
  small assembly-authored tile/icon family plus a Sprite Mode 1 cursor marker
  to make menu-style tile fetch behavior reviewable.
- The sprite scene now uses a new third ROM bank page to avoid crowding the
  milestone-2 compositing banks, clears both VDPs into a dedicated sprite
  review layout, and after the cleanup pass shows three distinct cues in one
  stable frame: two large rear-layer size/magnification sprites at the top,
  nine slot markers with only eight small sprites visible on a scanline, and
  an overlapping small-sprite pair below the limit lane.
- Added `showcase/assets/provenance/milestone-03-scenes.md`,
  `showcase/tests/milestone-03-checkpoints.md`, and README updates so human
  reviewers have the scene intent, provenance assumptions, and exact build/run
  commands in-tree.
- Verification commands passed:
  `python3 showcase/tools/package/build_showcase.py`
  `python3 showcase/tools/package/run_showcase_headless.py --frames 720 --trace build/showcase/m3.trace --hash-frame 719 --symbols build/showcase/showcase.sym`
- Stable milestone-3 hashes after the sprite-scene cleanup pass:
  tile scene frame `263` = `242b25f2e381ee5f1ef4980a42040dbd54da56806021529c1070b00a9ed8abf5`
  sprite scene frame `519` = `0c1c07b0cfa2b0e79142fa59c97aa27230d57fc80f8a86663e2dceae72691261`
  contract frame `719` = `0c1c07b0cfa2b0e79142fa59c97aa27230d57fc80f8a86663e2dceae72691261`
- Current milestone-3 limitation: `sjasmplus` symbol export remains 16-bit, so
  labels in the new third-bank page wrap in `showcase.sym`; this does not
  affect runtime execution because the ROM enters that page through explicit
  `BBR` switching and a fixed logical call target, but deeper bank-aware symbol
  export remains a later tooling concern.

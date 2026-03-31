# Showcase Milestone 3 Checkpoints

Milestone 3 adds two stable review scenes after the milestone-2 title and
compositing flow: a Graphic 2 menu/tile scene and a dual-VDP sprite capability
scene.

Traceability:
- `docs/spec/02-video.md`
- `docs/spec/04-io.md`
- `showcase/assets/provenance/milestone-03-scenes.md`
- `showcase/src/showcase.asm`

Verification commands:
- Build:
  `python3 showcase/tools/package/build_showcase.py`
- Contract run:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 720 --trace build/showcase/m3.trace --hash-frame 719 --symbols build/showcase/showcase.sym`

Checkpoint notes:
- `m3-tile-frame263`
  Command:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 264 --hash-frame 263`
  Expected frame hash:
  `242b25f2e381ee5f1ef4980a42040dbd54da56806021529c1070b00a9ed8abf5`
  Expected visual:
  dark navy Graphic 2 backdrop with a centered teal menu panel, amber option
  bars, pale icon tiles near the header, and a single amber Sprite Mode 1
  cursor pointing at the middle option
- `m3-sprites-frame519`
  Command:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 520 --hash-frame 519`
  Expected frame hash:
  `0c1c07b0cfa2b0e79142fa59c97aa27230d57fc80f8a86663e2dceae72691261`
  Expected visual:
  dark sprite-lab field with two large rear-layer review sprites at the top,
  nine slot markers across the mid scanline-limit lane with only eight small
  foreground sprites visible, and an overlapping small pair below them
- `m3-contract-frame719`
  Command:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 720 --trace build/showcase/m3.trace --hash-frame 719 --symbols build/showcase/showcase.sym`
  Expected frame hash:
  `0c1c07b0cfa2b0e79142fa59c97aa27230d57fc80f8a86663e2dceae72691261`
  Expected visual:
  same stable sprite capability scene as frame 519, proving the timed sequence
  has settled and remains deterministic for the milestone review run

Transition note:
- The milestone-2 title scene still holds through frame `119`.
- The milestone-2 compositing scene remains stable through the next timed dwell.
- The tile scene is the first stable state after the second timed bank swap.
- The cleaned-up sprite scene has settled by frame `519` and remains the final
  steady-state scene for the milestone-3 contract run.

# Showcase Milestone 4 Checkpoints

Milestone 4 adds two stable review states after the milestone-3 tile/sprite
flow: a mixed-mode Graphic 3 over Graphic 4 scene and a staged Graphic 4
command-engine scene.

Traceability:
- `docs/spec/02-video.md`
- `showcase/assets/provenance/milestone-04-scenes.md`
- `showcase/src/showcase.asm`

Verification commands:
- Build:
  `python3 showcase/tools/package/build_showcase.py`
- Contract run:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 960 --trace build/showcase/m4.trace --hash-frame 959 --symbols build/showcase/showcase.sym`

Checkpoint notes:
- `m4-mixed-mode-frame599`
  Command:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 600 --hash-frame 599`
  Expected frame hash:
  `50e05cc396201f45411af566117f56a010c3325d62ea77b5df2c9ff780d2bc72`
  Expected visual:
  a dark Graphic 4 rear layer with cyan/green panel blocks visible through a
  front Graphic 3 tile frame, plus opaque amber/mint VDP-A tiles that clearly
  override the rear layer where color index `0` transparency is not used
- `m4-command-frame719`
  Command:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 720 --hash-frame 719`
  Expected frame hash:
  `00b730680aa8e5c5e6ff62a90a2fc74543fca824555e72e99d65a55c3d175de9`
  Expected visual:
  a fully assembled Graphic 4 command-deck composition with a dark field,
  cyan outer frame, mint/amber filled blocks, and red highlight bars produced
  by staged `HMMV` updates rather than one static VRAM upload
- `m4-contract-frame959`
  Command:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 960 --trace build/showcase/m4.trace --hash-frame 959 --symbols build/showcase/showcase.sym`
  Expected frame hash:
  `00b730680aa8e5c5e6ff62a90a2fc74543fca824555e72e99d65a55c3d175de9`
  Expected visual:
  the same settled command-engine scene as frame `719`, proving the later
  timed banked updates have completed and the milestone-4 review state remains
  deterministic through the contract run

Transition note:
- The milestone-3 sprite scene still holds through frame `519`.
- The mixed-mode scene is the first stable new review state by frame `599`.
- The command-engine scene has settled by frame `719` and remains unchanged at
  the later frame `959` contract checkpoint.
- The milestone-4 scene logic intentionally preserves the documented limits
  around Graphic 3 lines `192-211` and does not rely on horizontal scroll via
  `R#26`/`R#27`.

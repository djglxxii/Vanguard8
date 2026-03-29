# Showcase Milestone 2 Checkpoints

Milestone 2 adds two stable review states: the title scene and the dual-VDP
compositing scene.

Traceability:
- `showcase/docs/asset-guidelines.md`
- `showcase/assets/provenance/milestone-02-scenes.md`
- `showcase/src/showcase.asm`

Verification commands:
- Build:
  `python3 showcase/tools/package/build_showcase.py`
- Contract run:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 480 --trace build/showcase/m2.trace --hash-frame 479 --symbols build/showcase/showcase.sym`

Checkpoint notes:
- `m2-title-frame119`
  Command:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 120 --hash-frame 119`
  Expected frame hash:
  `b23d96230880171ebb353fcef1a1e2bb16b12ec3bff35f2b6751e1ceb981b6d6`
  Expected visual:
  dark slate full-screen field with mint top/bottom rails and a centered blocky
  green/mint `V8` crest on VDP-A
- `m2-compositing-frame135`
  Command:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 136 --hash-frame 135`
  Expected frame hash:
  `d632dad8bb1d031d6c5a26e513a8f24081eadc96a44847ef0d4012cdd82141de`
  Expected visual:
  green/mint VDP-A frame and bridge over a deep-blue VDP-B background with two
  reveal panels and a cyan-linked center band
- `m2-contract-frame479`
  Command:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 480 --trace build/showcase/m2.trace --hash-frame 479 --symbols build/showcase/showcase.sym`
  Expected frame hash:
  `d632dad8bb1d031d6c5a26e513a8f24081eadc96a44847ef0d4012cdd82141de`
  Expected visual:
  same compositing scene as frame 135, proving the scene remains stable after
  the one-way timed transition from the title screen

Transition note:
- The title scene remains stable through frame `119`.
- By frame `135`, the ROM has completed the banked swap into the compositing
  scene and remains there for the rest of the milestone-2 loop.

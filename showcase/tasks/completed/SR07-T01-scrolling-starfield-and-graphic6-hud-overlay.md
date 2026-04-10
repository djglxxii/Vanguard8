# SR07-T01 — Add the Scrolling Starfield and Graphic 6 HUD Overlay Scene

Status: `completed`
Milestone: `7`
Depends on: `SR06-T01`

Implements against:
- `docs/spec/02-video.md`
- `docs/spec/04-io.md`
- `showcase/docs/showcase-rom-plan.md`
- `showcase/docs/showcase-implementation-plan.md`
- `showcase/docs/milestones/07.md`

Scope:
- Add a final banked scene with a vertically scrolling `Graphic 4` starfield.
- Add a fixed `Graphic 6` HUD overlay using a compact bitmap font/icon set.
- Add the checkpoint manifest and review note updates for the new late-loop
  scene.

Done when:
- The new scene demonstrates the intended mixed-mode separation without relying
  on text modes or unverified horizontal-scroll behavior.
- The completion summary explains the HUD asset approach and the updated
  late-loop verification flow.

Completion summary:
- Re-read the milestone-7 lock and contract, then implemented strictly inside
  `showcase/` against `docs/spec/02-video.md`, `docs/spec/04-io.md`,
  `docs/emulator/milestones/16.md`, and `showcase/docs/milestones/07.md`.
- Extended `showcase/src/showcase.asm` with a new bank-7
  `starfield_hud_scene_bank7` final tableau: `VDP-B` now carries a
  vertically scrolling `Graphic 4` starfield driven by `R#23`, while `VDP-A`
  presents a fixed `Graphic 6` HUD built from a compact direct-VRAM bitmap
  glyph/icon set for score, status, and remaining lives.
- Added a narrow ROM-side `INT1` SRAM trampoline and switched the bank-7 handoff
  to a fixed PRT0 scroll-handler chain so the new visual scene stays within
  showcase scope while remaining compatible with the current runtime.
- Replaced the milestone-7 placeholder manifest with the frozen review surface
  in `showcase/tests/milestone-07-checkpoints.md`, and updated the showcase
  README surfaces so the new final-scene verification command is documented.
- Verified the milestone-7 contract commands on the final tree:
  `python3 showcase/tools/package/build_showcase.py`
  and
  `python3 showcase/tools/package/run_showcase_headless.py --frames 1680 --trace build/showcase/m7.trace --hash-frame 1679 --symbols build/showcase/showcase.sym`
- Frozen milestone-7 outputs:
  frame hash `c191c3b98a3925e3a3c73f2c2dbfc6ed35b5ed6a67b0875dd9aae050b28f7f0c`,
  trace file `build/showcase/m7.trace`, event-log digest
  `11007163357591775607`, and ROM size `475136`.

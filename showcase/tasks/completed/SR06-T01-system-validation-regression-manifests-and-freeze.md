# SR06-T01 — Add the System Validation Scene, Regression Manifests, and Final Review Handoff

Status: `completed`
Milestone: `6`
Depends on: `SR05-T01`

Implements against:
- `docs/spec/00-overview.md`
- `docs/spec/01-cpu.md`
- `docs/spec/02-video.md`
- `docs/spec/03-audio.md`
- `docs/spec/04-io.md`
- `showcase/docs/showcase-rom-plan.md`
- `showcase/docs/showcase-implementation-plan.md`
- `showcase/docs/milestones/06.md`

Scope:
- Add the system validation scene.
- Freeze final frame/audio/trace checkpoint manifests.
- Add replay inputs if needed.
- Add final review notes explaining how to review the full loop.

Done when:
- A reviewer can rebuild and run the full ROM loop using only the documented
  showcase commands and manifests.
- The task summary explains the final review handoff and any intentionally
  remaining limitations.

Completion summary:
- Re-read the milestone-6 lock and contract, then implemented strictly inside
  `showcase/` against `docs/spec/00-overview.md`, `docs/spec/01-cpu.md`,
  `docs/spec/02-video.md`, `docs/spec/03-audio.md`, `docs/spec/04-io.md`, and
  `showcase/docs/milestones/06.md`.
- Extended `showcase/src/showcase.asm` with a final bank-6
  `system_validation_scene_bank6` tableau that keeps the mixed-audio state
  active while presenting a compact dual-V9938 Graphic 4 validation dashboard,
  VDP-A transparency framing, and Mode 2 foreground sprites.
- Added the final milestone manifest in
  `showcase/tests/milestone-06-checkpoints.md` and the reviewer handoff note in
  `showcase/docs/final-review-handoff.md`, while updating the showcase README
  surfaces so the full checkpoint chain is documented in one place.
- Verified the milestone-6 contract commands on the final tree:
  `python3 showcase/tools/package/build_showcase.py`
  and
  `python3 showcase/tools/package/run_showcase_headless.py --frames 1440 --trace build/showcase/m6.trace --hash-frame 1439 --hash-audio --symbols build/showcase/showcase.sym`
- Frozen final milestone-6 outputs:
  frame hash `af51373ebdeb1c076cdf76c86329b6248e71f5d738c833e193f7d3ede89e7f46`,
  audio hash `2eed1a37f1e2d36fa500e2e38ec57c1b1676f0b4f5249b1571f585c8785321b6`,
  trace file `build/showcase/m6.trace`, event-log digest
  `2385517300459103588`, and ROM size `327680`.
- No replay input bundle was needed because the full loop remains deterministic
  under idle controller state; the remaining limitations stay the documented
  showcase deferrals rather than new milestone-6 scope.

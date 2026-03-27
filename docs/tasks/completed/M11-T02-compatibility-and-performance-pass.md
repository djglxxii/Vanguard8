# M11-T02 — Run Compatibility and Performance Closure

Status: `completed`
Milestone: `11`
Depends on: `M11-T01`

Implements against:
- `docs/spec/00-overview.md`
- `docs/spec/01-cpu.md`
- `docs/spec/02-video.md`
- `docs/spec/03-audio.md`
- `docs/spec/04-io.md`

Scope:
- Run real-ROM compatibility passes.
- Fix correctness and performance issues that stay within the documented
  hardware contract.
- Record compatibility audit notes for the Z180/HD64180 assumption.

Done when:
- Cross-subsystem integration tests and long-running determinism tests pass.
- Compatibility notes document what was audited and what remains limited.

Completion summary:
- Added a long-running save/replay determinism stress test that runs across a
  save-state restore boundary and locks the current replay/runtime continuity.
- Added an emulator-level integration test that covers ROM bank switching,
  VDP-A interrupt visibility, audio/VCLK activity, and composited video output
  together under a deterministic repeated-run check.
- Recorded the Z180/HD64180 compatibility audit in
  `docs/emulator/08-compatibility-audit.md`, including what the repo now proves
  with tests and what remains a documented implementation assumption or corpus
  limitation because no external game-ROM set is checked into the repo.

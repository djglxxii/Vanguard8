# Current Milestone Lock

- Active milestone: `33`
- Title: `Instruction-Granular Audio Timing for YM2151 Busy Polls`
- Status: `ready-for-review`
- Locked on plan: `docs/emulator/07-implementation-plan.md`
- Contract file: `docs/emulator/milestones/33.md`

Execution rules:
- Only milestone `33` task files present in `docs/tasks/active/` may be
  executed.
- Milestone `31` is accepted.
- Milestone `32` is blocked on the core CPU/audio co-scheduling defect
  documented in `docs/tasks/blocked/M32-T01-frontend-live-audio-playback.md`.
- Keep milestone `33` work inside `src/core/`, `third_party/z180/`, `tests/`,
  and `docs/` as allowed by the milestone contract. `third_party/z180/` is
  limited to narrowly documented timed opcode blockers exposed only after the
  core timing fix lets PacManV8 progress farther.
- The goal is to advance audio hardware time in lockstep with scheduled CPU
  instruction execution so YM2151 busy-poll loops can observe busy clear and
  PacManV8 can progress past `audio_ym_write_bc` at `PC 0x2B8B`.
- Preserve deterministic execution by replacing the invalid all-zero
  PacManV8 300-frame digest guard from M32 with updated, nonzero audio/video
  regressions produced by the corrected timing model.
- Do not bundle frontend, debugger UI, new audio features, VDP feature work,
  opcode expansion beyond any blocker exposed by this timing fix, recording,
  export, or visualization work into this milestone.
- Milestone `33` task `M33-T01` is completed and moved to
  `docs/tasks/completed/`.
- Canonical Vanguard8 headless binary for verification is
  `cmake-build-debug/src/vanguard8_headless`. The stale `build/`
  CMake directory was removed on 2026-04-19; any prior task or
  field-manual note still pointing at `build/src/vanguard8_headless`
  is out of date.

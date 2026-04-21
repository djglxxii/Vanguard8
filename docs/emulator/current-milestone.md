# Current Milestone Lock

- Active milestone: `35`
- Title: `Agent Observability Primitives for Headless ROM Verification`
- Status: `ready-to-implement`
- Locked on plan: `docs/emulator/07-implementation-plan.md`
- Contract file: `docs/emulator/milestones/35.md`

Execution rules:
- Only milestone `35` task files present in `docs/tasks/active/` may be
  executed.
- Milestone `31` is accepted.
- Milestone `32` remains blocked on the core CPU/audio co-scheduling defect
  documented in `docs/tasks/blocked/M32-T01-frontend-live-audio-playback.md`.
- Milestone `33` verification commands were rerun on 2026-04-20 after
  refreshing the stale PacManV8 T017 300-frame audio regression hash to
  `24ce40791e466f9f686ee472b5798128065458e06a51f826666ae444ddfb5c75`.
- Milestone `34` is accepted. Task `M34-T01` is in `docs/tasks/completed/`.
- Keep milestone `35` work inside the allowed paths declared in
  `docs/emulator/milestones/35.md`: `src/frontend/`, `src/headless_main.cpp`,
  `tests/`, and the doc/task files listed in the contract.
- The goal is to close the observability gap that re-blocked PacManV8 T020
  after M34 landed: the canonical `vanguard8_headless` binary cannot
  distinguish between "intermission state never reached",
  "reached but crashed before drawing", and
  "drew but draw is not visible in the composed framebuffer."
- Do not bundle core-behavior changes, new tracking buffers, GUI/ImGui work,
  opcode/VDP/audio/scheduler work, or PacManV8 ROM edits into this milestone.
- Milestone `35` task `M35-T01` is planned in `docs/tasks/planned/`. Promote
  it into `docs/tasks/active/` when work begins.
- Canonical Vanguard8 headless binary for verification is
  `cmake-build-debug/src/vanguard8_headless`.

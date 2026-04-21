# Current Milestone Lock

- Active milestone: `34`
- Title: `Timed HD64180 PacManV8 Intermission Opcode Coverage`
- Status: `ready-for-review`
- Locked on plan: `docs/emulator/07-implementation-plan.md`
- Contract file: `docs/emulator/milestones/34.md`

Execution rules:
- Only milestone `34` task files present in `docs/tasks/active/` may be
  executed.
- Milestone `31` is accepted.
- Milestone `32` remains blocked on the core CPU/audio co-scheduling defect
  documented in `docs/tasks/blocked/M32-T01-frontend-live-audio-playback.md`.
- Milestone `33` verification commands were rerun on 2026-04-20 after
  refreshing the stale PacManV8 T017 300-frame audio regression hash to
  `24ce40791e466f9f686ee472b5798128065458e06a51f826666ae444ddfb5c75`.
- Keep milestone `34` work inside `src/core/`, `third_party/z180/`, `tests/`,
  and `docs/` as allowed by the milestone contract.
- The goal is to close the next timed-HD64180 compatibility gap exposed by
  PacManV8 T020 with narrow opcode-family coverage for:
  `DJNZ`, `JR NC/JR C`, `ADD A,r` / `ADD A,(HL)`, `ADD HL,ss`, and
  `SBC HL,ss`.
- Do not bundle broad opcode completion, scheduler/audio/frontend/debugger
  work, VDP changes, or PacManV8 ROM edits into this milestone.
- Milestone `34` task `M34-T01` is completed and moved to
  `docs/tasks/completed/`.
- Canonical Vanguard8 headless binary for verification is
  `cmake-build-debug/src/vanguard8_headless`. The stale `build/`
  CMake directory was removed on 2026-04-19; any prior task or
  field-manual note still pointing at `build/src/vanguard8_headless`
  is out of date.

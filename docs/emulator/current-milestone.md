# Current Milestone Lock

- Active milestone: `8`
- Title: `HD64180 DMA/Timers and VDP Command Engine`
- Status: `active`
- Locked on plan: `docs/emulator/07-implementation-plan.md`
- Contract file: `docs/emulator/milestones/08.md`

Execution rules:
- Only milestone `8` task files present in `docs/tasks/active/` may be
  executed.
- Do not pull in milestone `9+` tasks until milestone `8` is accepted and this
  lock file is advanced.
- Keep milestone `8` work inside `src/core/cpu/`, `src/core/video/`,
  `src/core/`, `third_party/z180/`, and `tests/` unless the milestone contract
  changes in the same edit.

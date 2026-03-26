# Current Milestone Lock

- Active milestone: `7`
- Title: `Audio Chips, Mixer, and Dedicated ADPCM Interrupt Flow`
- Status: `active`
- Locked on plan: `docs/emulator/07-implementation-plan.md`
- Contract file: `docs/emulator/milestones/07.md`

Execution rules:
- Only milestone `7` task files present in `docs/tasks/active/` may be
  executed.
- Do not pull in milestone `8+` tasks until milestone `7` is accepted and this
  lock file is advanced.
- Keep milestone `7` work inside `src/core/audio/`, `src/core/`,
  `third_party/nuked-opm/`, `third_party/ayumi/`, `third_party/msm5205/`, and
  `tests/` unless the milestone contract changes in the same edit.

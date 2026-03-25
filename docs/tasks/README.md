# Task Workflow

This directory holds the execution queue for implementation work derived from
`docs/emulator/07-implementation-plan.md`.

Structure:
- `planned/`
  Backlog of approved task files grouped by milestone. Promote from here into
  `active/` when the current milestone allows it.
- `active/`
  The only task files a coding agent may execute.
- `completed/`
  Finished task files with appended completion summaries.
- `blocked/`
  Tasks that could not be completed, with appended blocker summaries.

Rules:
- Keep `docs/emulator/current-milestone.md` aligned with the milestone that
  owns the active tasks.
- Promote only tasks that belong to the active milestone.
- Preserve task IDs when moving files between directories.
- Do not edit task scope in flight unless the milestone contract is updated in
  the same change.

Task file fields:
- `Status`
- `Milestone`
- `Depends on`
- `Implements against`
- `Scope`
- `Done when`
- `Completion summary`

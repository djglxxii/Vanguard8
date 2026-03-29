# Showcase Task Workflow

This directory holds the execution queue for showcase-ROM work derived from
`showcase/docs/showcase-implementation-plan.md`.

Structure:
- `planned/`
  Backlog of approved showcase task files grouped by milestone. Promote from
  here into `active/` only when the current showcase milestone allows it and a
  human has approved starting that task.
- `active/`
  The only showcase task files a coding agent may execute.
- `completed/`
  Finished showcase task files with appended completion summaries. These files
  are the human-review markdown artifacts for completed tasks.
- `blocked/`
  Tasks that could not be completed, with appended blocker summaries.

Rules:
- Keep `showcase/docs/current-milestone.md` aligned with the milestone that
  owns the active task.
- Promote only tasks that belong to the active showcase milestone.
- Preserve task IDs when moving files between directories.
- Do not edit task scope in flight unless the milestone contract is updated in
  the same change.
- After completing a task, move it to `completed/` and stop until a human has
  reviewed and approved the summary.

Task file fields:
- `Status`
- `Milestone`
- `Depends on`
- `Implements against`
- `Scope`
- `Done when`
- `Completion summary`


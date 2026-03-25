# Milestone Acceptance Checklist

Use this checklist before changing a milestone from `ready_for_verification` to
`accepted`.

- The active milestone in `docs/emulator/current-milestone.md` matches the work
  that was implemented.
- The milestone contract in `docs/emulator/milestones/NN.md` was reread before
  final verification.
- Only the active milestone's allowed paths were modified, or the contract was
  updated in the same change.
- The relevant spec and emulator design docs were reread and cited in the task
  summary.
- All task files for the milestone were moved out of `docs/tasks/active/`.
- Completed task files were moved to `docs/tasks/completed/` with completion
  summaries appended.
- Blocked task files were moved to `docs/tasks/blocked/` with concrete blockers
  appended.
- The milestone's declared verification commands were rerun on the final tree.
- Required tests were added or updated to lock the implemented behavior.
- Explicit non-goals and forbidden extras were not implemented.
- Any missing or ambiguous spec behavior was documented and resolved in docs
  before code was locked to it.
- Remaining limitations are narrow, intentional, and recorded in docs or task
  summaries.

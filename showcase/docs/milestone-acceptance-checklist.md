# Showcase Milestone Acceptance Checklist

Use this checklist before changing a showcase milestone from
`ready_for_verification` to `accepted`.

- The active milestone in `showcase/docs/current-milestone.md` matches the work
  that was implemented.
- The milestone contract in `showcase/docs/milestones/NN.md` was reread before
  final verification.
- Only the active milestone's allowed paths were modified, or the contract was
  updated in the same change.
- The relevant spec, emulator, and showcase planning docs were reread and cited
  in the task summary.
- The active task file was moved out of `showcase/tasks/active/`.
- The completed task file was moved to `showcase/tasks/completed/` with the
  completion summary appended.
- Any blocked task file was moved to `showcase/tasks/blocked/` with concrete
  blockers appended.
- The milestone's declared verification commands were rerun on the final tree.
- The completed task summary is sufficient for human review of what changed,
  what was verified, and what remains limited.
- A human explicitly approved the completed task before the next task or
  milestone was promoted.
- Explicit non-goals and forbidden extras were not implemented.
- Remaining limitations are narrow, intentional, and recorded in showcase docs
  or the task summary.


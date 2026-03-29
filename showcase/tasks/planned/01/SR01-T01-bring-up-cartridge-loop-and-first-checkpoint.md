# SR01-T01 — Implement the Bring-Up Cartridge Loop, MMU/Vector Setup, and First Banked Checkpoint

Status: `planned`
Milestone: `1`
Depends on: `SR00-T01`

Implements against:
- `docs/spec/00-overview.md`
- `docs/spec/01-cpu.md`
- `docs/spec/04-io.md`
- `showcase/docs/showcase-rom-plan.md`
- `showcase/docs/showcase-implementation-plan.md`
- `showcase/docs/milestones/01.md`

Scope:
- Add ROM-side boot/reset code, MMU setup, and interrupt vector setup.
- Add the first deterministic scene loop scaffold.
- Add at least one banked-content transition.
- Add the first checkpoint entry and the notes needed to review it.

Done when:
- The ROM boots through a deterministic loop and crosses a banked transition.
- The first checkpoint can be reproduced with the documented run command.
- The completed task summary explains the current loop structure and any
  remaining milestone-1 limitations.

Completion summary:
- Pending. Append before moving to `showcase/tasks/completed/`.


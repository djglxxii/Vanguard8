# M03-T01 — Implement Master-Cycle Counter and Scheduler Core

Status: `active`
Milestone: `3`
Depends on: `M02-T03`

Implements against:
- `docs/spec/00-overview.md`
- `docs/emulator/02-emulation-loop.md`

Scope:
- Add the master-cycle counter and event queue.
- Define the scheduler interface used by the core runtime.
- Keep timing rooted in the 14.31818 MHz master clock.

Done when:
- Scheduled events execute in cycle order deterministically.
- CPU cycle accounting stays monotonic through scheduler activity.

Completion summary:
- Pending.

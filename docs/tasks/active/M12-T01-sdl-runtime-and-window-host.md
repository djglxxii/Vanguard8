# M12-T01 — Refactor the Frontend into a Persistent SDL Runtime and Window Host

Status: `active`
Milestone: `12`
Depends on: `none`

Implements against:
- `docs/emulator/00-overview.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/10-desktop-gui-audit.md`
- `docs/emulator/milestones/12.md`

Scope:
- Replace the current one-shot frontend launcher with a persistent runtime.
- Implement the SDL window lifecycle surface and event pump foundation.
- Keep the work narrow enough that milestone 12 still focuses on the playable
  desktop baseline rather than the later core-compatibility or heavy-debugger
  backlog.

Done when:
- `vanguard8_frontend` opens a real desktop window and stays alive until quit.
- Window/backend initialization and shutdown paths are test-covered through the
  new frontend host seams.

Completion summary:

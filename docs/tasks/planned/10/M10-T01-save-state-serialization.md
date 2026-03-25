# M10-T01 — Implement Save-State Serialization

Status: `planned`
Milestone: `10`
Depends on: `M09-T03`

Implements against:
- `docs/emulator/00-overview.md`
- `docs/emulator/07-implementation-plan.md`

Scope:
- Add a versioned save-state format.
- Cover CPU, MMU, scheduler, VRAM, palettes, audio state, and other core
  machine state.
- Keep the format explicit and testable.

Done when:
- Save/load round-trips preserve the documented emulator state surface.
- Tests lock versioned serialization behavior.

Completion summary:
- Pending.

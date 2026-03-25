# M02-T02 — Implement MMU Translation and Illegal BBR Guardrails

Status: `planned`
Milestone: `2`
Depends on: `M02-T01`

Implements against:
- `docs/spec/00-overview.md`
- `docs/spec/01-cpu.md`
- `docs/emulator/03-cpu-and-bus.md`

Scope:
- Implement translation through `CBAR`, `CBR`, and `BBR`.
- Handle `OUT0` writes to the internal MMU registers.
- Warn when `BBR >= 0xF0`.

Done when:
- MMU translation matches the Vanguard 8 boot mapping.
- Tests lock the illegal `BBR` warning path.

Completion summary:
- Pending.

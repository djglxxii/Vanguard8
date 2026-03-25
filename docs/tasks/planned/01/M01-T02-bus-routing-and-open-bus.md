# M01-T02 — Implement Bus Routing and Open-Bus Behavior

Status: `planned`
Milestone: `1`
Depends on: `M01-T01`

Implements against:
- `docs/spec/00-overview.md`
- `docs/emulator/03-cpu-and-bus.md`

Scope:
- Add the physical memory map to `Bus`.
- Route unmapped memory and unmapped I/O reads to `0xFF`.
- Emit warning logs for unmapped accesses.

Done when:
- Fixed ROM, banked ROM, and SRAM route exactly as documented.
- Tests cover `0xFF` behavior for unmapped memory and ports.

Completion summary:
- Pending.

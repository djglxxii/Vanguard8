# M05-T03 — Implement VDP-A Status Semantics and IRQ Routing

Status: `planned`
Milestone: `5`
Depends on: `M05-T02`

Implements against:
- `docs/spec/00-overview.md`
- `docs/spec/02-video.md`
- `docs/spec/01-cpu.md`

Scope:
- Implement VDP-A V-blank and H-blank CPU-visible status behavior.
- Clear `S#0` and `S#1` flags on the documented reads.
- Keep VDP-B interrupt output isolated from the CPU.

Done when:
- VDP-A IRQs reach INT0 correctly.
- Tests prove that VDP-B never asserts a CPU interrupt.

Completion summary:
- Pending.

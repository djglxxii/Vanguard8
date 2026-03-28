# M13-T01 — Implement the V9938 Read-Path Fixes and Targeted HD64180 Audit Coverage

Status: `active`
Milestone: `13`
Depends on: `M12-T03`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/spec/02-video.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/11-pre-gui-audit.md`
- `docs/emulator/milestones/13.md`

Scope:
- Add the V9938 VRAM read-ahead latch.
- Honor VDP screen-disable behavior through `R#1` bit 6.
- Add focused HD64180 tests for the covered `MLT`, `TST`, and broader
  `IN0`/`OUT0` behavior needed for ROM bring-up confidence.

Done when:
- VRAM reads and screen blanking follow the written spec in covered tests.
- The targeted HD64180 extension surface is explicitly regression-covered.

Completion summary:

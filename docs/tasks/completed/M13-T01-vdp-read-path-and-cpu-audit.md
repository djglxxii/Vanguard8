# M13-T01 — Implement the V9938 Read-Path Fixes and Targeted HD64180 Audit Coverage

Status: `completed`
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
  `IN0`/`OUT0` behavior needed for ROM bring-up confidence, including the
  narrow extracted opcode surface in `third_party/z180/` where required.

Done when:
- VRAM reads and screen blanking follow the written spec in covered tests.
- The targeted HD64180 extension surface is explicitly regression-covered.

Completion summary:
- Added the missing V9938 read-path behavior in `src/core/video/v9938.*` so
  VRAM reads now follow the required one-read-behind latch semantics, and
  `R#1` bit 6 now blanks the display to the backdrop color instead of leaking
  partially updated VRAM contents.
- Expanded the extracted HD64180 subset in `third_party/z180/z180_core.*` with
  the audited `MLT`, `TST`, and non-`A` `IN0`/`OUT0` opcode surface needed for
  ROM bring-up confidence, with `TST` flags documented in `docs/spec/01-cpu.md`.
- Added regression coverage for the new VDP and CPU behavior, updated fixture
  and integration setup to enable display output explicitly, and hardened the
  real SDL/OpenGL frontend tests to skip cleanly when no desktop video backend
  is available in the environment.

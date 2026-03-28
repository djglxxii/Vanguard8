# M13-T02 — Expand Sprite Compatibility for Real ROM Bring-Up

Status: `completed`
Milestone: `13`
Depends on: `M13-T01`

Implements against:
- `docs/spec/02-video.md`
- `docs/emulator/04-video.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/11-pre-gui-audit.md`
- `docs/emulator/milestones/13.md`

Scope:
- Add 16x16 sprite size and sprite magnification behavior.
- Replace the remaining fixed sprite-table assumptions with register-relative
  `R#5`/`R#6`/`R#11` addressing where the spec already defines it.
- Preserve the covered Mode 2 status/collision behavior already in place.

Done when:
- Covered sprite fixtures no longer assume fixed 8x8 size or fixed default
  table addresses.
- The sprite path matches the documented register-controlled layout for the
  covered modes.

Completion summary:
- Reworked the covered Mode 2 sprite path in `src/core/video/v9938.*` so it now
  derives the sprite attribute, color, and pattern tables from `R#5`/`R#6`/`R#11`,
  honors `R#1` sprite size and magnification, and fetches 16×16 sprite rows from
  four-pattern groups instead of the old fixed 8×8 assumption.
- Updated the covered default sprite layout in `docs/spec/02-video.md` and
  `docs/emulator/04-video.md` to document the register-relative table packing and
  the corrected 2 KB sprite pattern table sizing for the repo's 64 KB VRAM model.
- Added regression coverage in `tests/test_video.cpp` for 16×16 sprites,
  magnification, and relocated sprite-table bases while preserving the existing
  Mode 2 overflow and collision behavior.

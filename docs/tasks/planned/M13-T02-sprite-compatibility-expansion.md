# M13-T02 — Expand Sprite Compatibility for Real ROM Bring-Up

Status: `planned`
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

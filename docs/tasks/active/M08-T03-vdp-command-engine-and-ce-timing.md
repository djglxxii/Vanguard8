# M08-T03 — Implement VDP Command Engine and CE Timing

Status: `active`
Milestone: `8`
Depends on: `M08-T02`

Implements against:
- `docs/spec/02-video.md`
- `docs/emulator/04-video.md`

Scope:
- Implement the documented VDP command set.
- Expose `S#2.CE` while commands are in progress.
- Clear `CE` at the correct cycle boundary.

Done when:
- CPU polling distinguishes in-progress versus complete states.
- Tests lock command completion timing.

Completion summary:
- Pending.

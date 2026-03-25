# M03-T02 — Add Blanking and /VCLK Event Timing

Status: `planned`
Milestone: `3`
Depends on: `M03-T01`

Implements against:
- `docs/spec/00-overview.md`
- `docs/spec/03-audio.md`
- `docs/emulator/02-emulation-loop.md`

Scope:
- Schedule H-blank, V-blank, V-blank end, and MSM5205 `/VCLK`.
- Expose stable assertion timing for INT0 and INT1 sources.
- Record deterministic event positions for tests.

Done when:
- Repeated runs produce identical event timing.
- Tests cover the documented cycle positions for the scheduled events.

Completion summary:
- Pending.

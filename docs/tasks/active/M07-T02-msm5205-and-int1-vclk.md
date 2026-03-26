# M07-T02 — Integrate MSM5205 and INT1-Driven /VCLK Timing

Status: `active`
Milestone: `7`
Depends on: `M07-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/spec/03-audio.md`
- `docs/emulator/05-audio.md`

Scope:
- Integrate the MSM5205 adapter and control registers.
- Drive `/VCLK` timing through INT1 at the documented cadence.
- Keep all timing rooted in the scheduler.

Done when:
- Control register changes update `/VCLK` cadence correctly.
- INT1 timing tests lock the 4/6/8 kHz-equivalent intervals.

Completion summary:
- Pending.

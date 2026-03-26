# M07-T02 — Integrate MSM5205 and INT1-Driven /VCLK Timing

Status: `completed`
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
- Added a narrow extracted MSM5205 core in `third_party/msm5205/` and wrapped
  it with `src/core/audio/msm5205_adapter.cpp` /
  `src/core/audio/msm5205_adapter.hpp`.
- Port `0x60` now selects `4 kHz`, `6 kHz`, `8 kHz`, or stop/reset, port
  `0x61` latches the next ADPCM nibble, and scheduler-driven `/VCLK` events now
  assert CPU `INT1` through `Bus`.
- Tests now lock control-register cadence updates and stable INT1-driven `/VCLK`
  counts in `tests/test_audio.cpp`.

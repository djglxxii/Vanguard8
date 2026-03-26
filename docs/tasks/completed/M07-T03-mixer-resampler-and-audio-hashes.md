# M07-T03 — Add Mixer, Resampler, and Deterministic Audio Hashing

Status: `completed`
Milestone: `7`
Depends on: `M07-T02`

Implements against:
- `docs/spec/03-audio.md`
- `docs/emulator/05-audio.md`

Scope:
- Mix YM2151, AY, and MSM5205 output.
- Resample to the frontend audio output rate.
- Add deterministic headless audio hash coverage.

Done when:
- Audio output is synchronized and audible.
- Headless runs produce stable audio hashes.

Completion summary:
- Added the deterministic core mixer in `src/core/audio/audio_mixer.cpp` /
  `src/core/audio/audio_mixer.hpp` and wired audio advancement into
  `src/core/bus.cpp` and `src/core/emulator.cpp`.
- The mixer now samples the common 55,930 Hz timeline from master-cycle events,
  resamples to 48 kHz with a master-clock-rational stepper, and exposes a
  deterministic stereo output digest through `Emulator`.
- Repeated-run hash coverage now lives in `tests/test_audio.cpp`, satisfying the
  milestone-7 headless/core determinism requirement without reaching outside the
  milestone's allowed paths.

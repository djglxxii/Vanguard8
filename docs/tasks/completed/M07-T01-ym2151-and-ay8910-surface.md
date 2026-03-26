# M07-T01 — Integrate YM2151 and AY-3-8910 Register Surfaces

Status: `completed`
Milestone: `7`
Depends on: `M06-T03`

Implements against:
- `docs/spec/03-audio.md`
- `docs/emulator/05-audio.md`

Scope:
- Wrap Nuked-OPM for YM2151.
- Wrap Ayumi for AY-3-8910.
- Expose the documented register/status surface for both chips.

Done when:
- YM2151 and AY register access behaves per spec.
- Tests lock YM2151 busy/status handling and AY latch/read/write behavior.

Completion summary:
- Vendored Nuked-OPM and Ayumi into `third_party/nuked-opm/` and
  `third_party/ayumi/`, then wrapped them in
  `src/core/audio/ym2151.cpp` / `src/core/audio/ym2151.hpp` and
  `src/core/audio/ay8910.cpp` / `src/core/audio/ay8910.hpp`.
- Bus ports `0x40`, `0x41`, `0x50`, and `0x51` now drive the documented
  register and status surfaces instead of inert stubs.
- Tests now lock YM2151 busy/status timing and AY latch/read/write behavior in
  `tests/test_audio.cpp`.

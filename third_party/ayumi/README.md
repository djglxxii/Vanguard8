# Ayumi

Vendored source:
- Repository: `https://github.com/true-grue/ayumi`
- Commit: `07c08b4874c359169e4a028edf73f046d8b763e2`

Vendored files for milestone 7:
- `ayumi.c`
- `ayumi.h`

Usage in this repo:
- `src/core/audio/ay8910.cpp` wraps the vendored core for the AY address latch
  on port `0x50`, register read/write behavior on port `0x51`, and deterministic
  mono-to-stereo center panning on the master-cycle timeline.

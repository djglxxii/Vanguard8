# Nuked-OPM

Vendored source:
- Repository: `https://github.com/nukeykt/Nuked-OPM`
- Commit: `23ea53bb442b3f761ded3cd8a27399dd46db34fc`

Vendored files for milestone 7:
- `opm.c`
- `opm.h`

Usage in this repo:
- `src/core/audio/ym2151.cpp` wraps the vendored core for ports `0x40` and
  `0x41`, YM busy/status reads, timer IRQ visibility, and deterministic stereo
  sample capture on the master-cycle timeline.

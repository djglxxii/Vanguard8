# MSM5205 Narrow Extraction

Derived source:
- Repository: `https://github.com/mamedev/mame`
- Reference commit: `b6fc5ca5fda5f0abb1b759cc7e11a37377e10d01`
- Reference file: `src/devices/sound/msm5205.cpp`

Milestone-7 extraction scope:
- `msm5205_core.cpp`
- `msm5205_core.hpp`

This extraction keeps only the ADPCM difference-table generation and step/signal
update logic needed for the documented Vanguard 8 MSM5205 surface. Device
framework code, timers, and MAME sound-stream plumbing were intentionally left
out because milestone 7 only requires the control register, nibble feed, `/VCLK`
cadence, and deterministic decoded output for the core mixer/tests.

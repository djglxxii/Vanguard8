# Showcase Tools

This directory is reserved for build helpers and asset conversion scripts for
the showcase ROM.

Current milestone-0 tools:
- `package/build_showcase.py`
  Deterministic `sjasmplus` wrapper that emits `build/showcase/showcase.rom`
  and a debugger-friendly `build/showcase/showcase.sym`.
- `package/run_showcase_headless.py`
  Stable wrapper around `vanguard8_headless` with default showcase ROM/symbol
  paths and a two-step runtime-plus-trace flow when `--trace` is requested.

Planned later tools:
- palette packer
- tile/sprite converter
- Graphic 4 bitmap packer
- ADPCM prep helper

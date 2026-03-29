# Showcase Packaging Wrappers

Stable milestone-0 entry points:
- `python3 showcase/tools/package/build_showcase.py`
- `python3 showcase/tools/package/run_showcase_headless.py --frames 1 --trace build/showcase/m0.trace`

Behavior:
- `build_showcase.py` assembles `showcase/src/showcase.asm` with `sjasmplus`,
  writes `build/showcase/showcase.rom`, and converts the assembler symbol dump
  into the repo's `ADDR LABEL` symbol format at `build/showcase/showcase.sym`.
- `run_showcase_headless.py` defaults to the showcase ROM and symbol outputs.
  When `--trace` is combined with runtime options such as `--frames`, it first
  runs the deterministic headless session, then captures a separate symbol-aware
  trace so both milestone requirements are satisfied with one wrapper command.
  The wrapper defaults trace capture to 32 decoded instructions unless
  `--trace-instructions` is provided explicitly.

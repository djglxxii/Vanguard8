# Showcase ROM Final Review Handoff

This note is the milestone-6 reviewer handoff for the Vanguard 8 showcase ROM.

## Purpose

The showcase ROM is a deterministic regression artifact, not a content demo.
Review it by comparing the documented checkpoint commands and expected outputs
 to the running build, not by judging it as a polished game loop.

## Build

```bash
python3 showcase/tools/package/build_showcase.py
```

Expected outputs:
- `build/showcase/showcase.rom`
- `build/showcase/showcase.sym`

## Review Surfaces

Use these manifests in order:
- `showcase/tests/milestone-01-checkpoints.md`
- `showcase/tests/milestone-02-checkpoints.md`
- `showcase/tests/milestone-03-checkpoints.md`
- `showcase/tests/milestone-04-checkpoints.md`
- `showcase/tests/milestone-05-checkpoints.md`
- `showcase/tests/milestone-06-checkpoints.md`

## Full-Loop Command

```bash
python3 showcase/tools/package/run_showcase_headless.py --frames 1440 --trace build/showcase/m6.trace --hash-frame 1439 --hash-audio --symbols build/showcase/showcase.sym
```

Review expectations:
- the command completes without runtime errors
- the trace is symbol-aware and resolves labels from `showcase.sym`
- the final frame hash and audio hash match the milestone-6 manifest

Frozen milestone-6 outputs:
- Frame hash at frame `1439`:
  `af51373ebdeb1c076cdf76c86329b6248e71f5d738c833e193f7d3ede89e7f46`
- Audio hash:
  `2eed1a37f1e2d36fa500e2e38ec57c1b1676f0b4f5249b1571f585c8785321b6`

## Intended Final Loop Shape

The ROM advances through these validation phases:
- boot/MMU/vector bring-up
- title scene
- dual-VDP compositing scene
- tile-mode scene
- sprite scene
- mixed-mode scene
- Graphic 4 command-engine scene
- YM-only, AY-only, MSM-only, and mixed-audio windows
- final system-validation tableau

## Remaining Intentional Limits

- No unverified horizontal scroll via `R#26`/`R#27`
- No Text 1/Text 2, Graphic 5/6/7, or 4-player expansion content
- No showcase-only emulator features beyond the documented runtime/tooling
- No replay input bundle was needed; the full loop remains deterministic with
  idle controller state

If a mismatch appears, use the milestone-specific manifest for the first phase
that fails instead of jumping straight to the end-of-loop command.

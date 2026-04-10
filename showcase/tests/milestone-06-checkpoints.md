# Showcase Milestone 6 Checkpoints

Milestone 6 freezes the showcase ROM as a full-loop regression artifact. The
earlier milestone manifests remain the authoritative checkpoints for the title,
compositing, tile, sprite, mixed-mode, command, and audio phases; this file
adds the final steady-state system-validation review surface and the one-command
full-loop check.

Traceability:
- `docs/spec/00-overview.md`
- `docs/spec/01-cpu.md`
- `docs/spec/02-video.md`
- `docs/spec/03-audio.md`
- `docs/spec/04-io.md`
- `showcase/docs/milestones/06.md`
- `showcase/src/showcase.asm`

Frozen earlier manifests:
- `showcase/tests/milestone-01-checkpoints.md`
- `showcase/tests/milestone-02-checkpoints.md`
- `showcase/tests/milestone-03-checkpoints.md`
- `showcase/tests/milestone-04-checkpoints.md`
- `showcase/tests/milestone-05-checkpoints.md`

Verification commands:
- Build:
  `python3 showcase/tools/package/build_showcase.py`
- Final full-loop run:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 1440 --trace build/showcase/m6.trace --hash-frame 1439 --hash-audio --symbols build/showcase/showcase.sym`

Checkpoint notes:
- `m6-system-validation-frame1439`
  Purpose:
  prove the ROM reaches the final banked system-validation tableau after the
  mixed-audio phase and remains stable through the milestone-6 review frame
  Command:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 1440 --trace build/showcase/m6.trace --hash-frame 1439 --hash-audio --symbols build/showcase/showcase.sym`
  Expected visual:
  a final dual-VDP Graphic 4 dashboard with four rear-layer validation bays, a
  lower status deck, a transparent VDP-A frame and divider bars, and two small
  foreground Mode 2 sprites over the composed background while mixed audio is
  still active
  Expected frame hash:
  `af51373ebdeb1c076cdf76c86329b6248e71f5d738c833e193f7d3ede89e7f46`
  Expected audio hash:
  `2eed1a37f1e2d36fa500e2e38ec57c1b1676f0b4f5249b1571f585c8785321b6`

Trace cues:
- The wrapper emits a separate symbol-aware bootstrap trace at
  `build/showcase/m6.trace`; use it as a symbol smoke check, not as a
  late-scene capture.
- The full-loop runtime command above is the authoritative proof that the ROM
  reaches the final bank-6 system-validation tableau and remains stable through
  frame `1439`.

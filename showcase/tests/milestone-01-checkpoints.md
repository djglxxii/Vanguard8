# Showcase Milestone 1 Checkpoints

Implemented against `docs/spec/00-overview.md`, `docs/spec/01-cpu.md`,
`docs/spec/04-io.md`, and `showcase/docs/milestones/01.md`.

## Checkpoint `m1-frame239-banked-loop`

Purpose:
- Prove the ROM reaches a deterministic fixed-ROM loop that repeatedly crosses
  the bank window while keeping a stable frame result at the milestone review
  frame.

Build command:

```bash
python3 showcase/tools/package/build_showcase.py
```

Review command:

```bash
python3 showcase/tools/package/run_showcase_headless.py --frames 240 --trace build/showcase/m1.trace --hash-frame 239 --symbols build/showcase/showcase.sym
```

Expected results:
- `build/showcase/showcase.rom` size: `49152` bytes
- `build/showcase/showcase.sym` contains the milestone-1 labels
- Frame `239` hash:
  `582ae1c79b4c4d09cedab52937047d3f04b60e0f2dda59454ae96e28ddd50475`

Trace cues in `build/showcase/m1.trace`:
- Steps `21` and `25` show `OUT0 (0x39),A`, proving the ROM selects the bank
  window explicitly.
- Steps `22` and `26` show `LD A,(0x4001)`, proving the ROM reads banked bytes
  from the same logical address after changing `BBR`.
- The bank probe runs before palette setup, so these cues are present within
  the wrapper's default 32 traced instructions.

Current milestone-1 limitation:
- The loop crosses the bank boundary at CPU speed rather than dwelling on a
  human-visible multi-frame scene. The deterministic frame hash and the banked
  trace cues are the intended review artifacts for this first checkpoint.

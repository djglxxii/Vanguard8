# Showcase Milestone 5 Checkpoints

Milestone 5 adds the audio verification scene after the milestone-4 command
scene and is intentionally reviewed with shorter cumulative probes before any
final full-loop acceptance run.

Traceability:
- `docs/spec/03-audio.md`
- `docs/spec/04-io.md`
- `showcase/docs/milestones/05.md`
- `showcase/src/showcase.asm`

Verification commands:
- Build:
  `python3 showcase/tools/package/build_showcase.py`
- Development probes:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 1000 --hash-frame 999`
  `python3 showcase/tools/package/run_showcase_headless.py --frames 1040 --hash-audio`
  `python3 showcase/tools/package/run_showcase_headless.py --frames 1080 --hash-audio`
  `python3 showcase/tools/package/run_showcase_headless.py --frames 1120 --hash-audio`
  `python3 showcase/tools/package/run_showcase_headless.py --frames 1160 --hash-audio`
- Optional final acceptance run:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 1200 --trace build/showcase/m5.trace --hash-audio --symbols build/showcase/showcase.sym`

Checkpoint notes:
- `m5-scene-entry-frame999`
  Purpose:
  prove the ROM exits the milestone-4 command scene and reaches the audio-scene
  setup path without paying for the entire milestone-5 loop
  Command:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 1000 --hash-frame 999`
  Expected result:
  the command completes without scheduler/runtime errors and emits a stable
  frame hash for the first post-command-scene audio setup state
- `m5-ym-window-frame1039`
  Purpose:
  first short audio-hash probe after the audio scene is active
  Command:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 1040 --hash-audio`
  Expected result:
  the command completes without scheduler/runtime errors and prints an audio
  hash suitable for later milestone-5 review notes
- `m5-ay-window-frame1079`
  Purpose:
  second short cumulative audio-hash probe covering the next audio scene window
  Command:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 1080 --hash-audio`
  Expected result:
  the command completes without scheduler/runtime errors and prints an audio
  hash distinct from the earlier probe if the scene advanced correctly
- `m5-msm-window-frame1119`
  Purpose:
  first targeted probe that should exercise the INT1-driven MSM5205 section
  Command:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 1120 --hash-audio`
  Expected result:
  the command completes without scheduler/runtime errors and reaches the late
  audio scene without requiring the full 1200-frame contract run
- `m5-mixed-window-frame1159`
  Purpose:
  short cumulative probe for the mixed-audio section before any full-loop pass
  Command:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 1160 --hash-audio`
  Expected result:
  the command completes without scheduler/runtime errors and prints a stable
  audio hash for the combined-audio portion of the loop

Review posture:
- These probes are the default milestone-5 development loop.
- The optional `1200`-frame command is reserved for final acceptance or for
  confirming that the whole audio loop remains stable end to end after the
  shorter probes are already green.
- Expected hashes are intentionally deferred until the blocked runtime issue is
  closed and the milestone-5 loop is stable enough to freeze review artifacts.

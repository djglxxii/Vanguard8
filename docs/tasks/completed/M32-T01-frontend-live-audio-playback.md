# M32-T01 — Frontend Live Audio Playback

Status: `completed`
Milestone: `32`
Depends on: `M31-T01`
Resolved by: `M33-T01`

Implements against:
- `docs/spec/03-audio.md`
- `docs/emulator/05-audio.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/32.md`
- `/home/djglxxii/src/PacManV8/docs/tasks/completed/T016-psg-sound-effects.md`
- `/home/djglxxii/src/PacManV8/docs/tasks/active/T017-fm-music.md`

Scope:
- Diagnose why `cmake-build-debug/src/vanguard8_frontend` is silent even
  though the deterministic headless mixer path produces the accepted PacManV8
  T016/T017 audio hash.
- Fix the live frontend audio path inside `src/frontend/`, using
  `AudioMixer::consume_output_bytes()` as the only PCM source for SDL.
- Touch `src/core/audio/` only if the investigation proves the mixer does not
  populate output bytes under the frontend scheduler path, and preserve all
  existing headless audio hashes if that happens.
- Add the fake-device, pump, lifecycle, PCM-parity, and digest regression
  coverage required by `docs/emulator/milestones/32.md`.
- Update `docs/emulator/05-audio.md` so it documents the shipping GUI audio
  path instead of describing the SDL ring buffer as future work.

Done when:
- The frontend opens an audio device before the first running frame, pumps PCM
  every running frame, respects queue back-pressure, and closes the device on
  shutdown.
- A fake frontend audio device records PCM byte streams that are bit-identical
  to the headless per-frame `AudioMixer::consume_output_bytes()` stream for a
  fixed ROM/frame-count schedule.
- Existing audio-hash-gated tests remain green, including the PacManV8
  300-frame digest
  `0d634fb059d15b12c4a8faf2412fbe08b85d187a31b1f22278ce3662f3b44390`.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes.
- Manual launch of `cmake-build-debug/src/vanguard8_frontend
  /home/djglxxii/src/PacManV8/build/pacman.rom` produces audible T016/T017
  cues in the 300-frame review window under normal desktop SDL audio.

Out of scope:
- Chip-side audio behavior changes.
- New frontend features beyond audio bring-up controls.
- New video, input, debugger, CPU, VDP, scheduler, recording, export, or audio
  visualization work.

Verification commands:
```bash
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure
cmake-build-debug/src/vanguard8_headless \
    --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
    --frames 300 --hash-audio \
    --expect-audio-hash \
    0d634fb059d15b12c4a8faf2412fbe08b85d187a31b1f22278ce3662f3b44390
cmake-build-debug/src/vanguard8_frontend \
    /home/djglxxii/src/PacManV8/build/pacman.rom
```

## Completion summary

Completed on 2026-04-20 after M33-T01 resolved the underlying blocker.

Frontend audio bring-up (delivered under milestone 32 on 2026-04-19):
- Positional ROM launch support for `vanguard8_frontend`.
- `AudioQueuePump` mixer-pull API that preserves pending PCM while SDL queue
  back-pressure is active.
- SDL output configured as `AUDIO_S16LSB` to match `AudioMixer` byte layout.
- Fake-device lifecycle, SDL dummy-device open/queue/close/reopen/error,
  headless/frontend PCM parity, and PacManV8 300-frame digest regression
  coverage.
- `docs/emulator/05-audio.md` rewritten to describe the shipping GUI audio
  path instead of marking the SDL ring buffer as future work.

Core co-scheduling fix (delivered under milestone 33 on 2026-04-19, see
`docs/tasks/completed/M33-T01-fix-instruction-granular-audio-timing.md`):
- `Emulator::run_cpu_until()` now advances audio, VDP command engines, CPU
  T-state accounting, and `master_cycle_` in scheduled instruction-sized
  chunks instead of aging audio once before the whole event-slice CPU loop.
- PacManV8 T017 no longer stalls in the YM2151 busy-poll helper at
  `PC 0x2B8B`; audio output bytes contain nonzero PCM in the review window
  and frame 300 is nonblack.
- The invalid M32 all-zero PacManV8 300-frame digest guard was replaced with
  the nonzero audio SHA-256
  `24ce40791e466f9f686ee472b5798128065458e06a51f826666ae444ddfb5c75` and a
  nonblack frame pixel-payload SHA-256
  `4a63cec305375edd4b20e85ba9830d83888e2eaf4327a29c229cfc7ce7a79693`.
- The narrow post-fix opcode blocker `ADD A,n` (`0xC6`) at `PC 0x2B1C` was
  implemented with timed adapter and flag coverage.

Net effect on M32 exit criteria:
- Launching `cmake-build-debug/src/vanguard8_frontend /path/to/pacman.rom`
  now produces audible PacManV8 T016/T017 cues under normal desktop SDL
  audio, satisfying the listening exit criterion.
- `ctest --test-dir cmake-build-debug --output-on-failure` is green.
- The M32 determinism regression guard (the all-zero 300-frame audio digest)
  was superseded by the nonzero M33 digest, as documented in the M33 task
  and in `docs/emulator/current-milestone.md`.

The "Incompletion summary" below is retained as the historical record of why
M32 was paused on 2026-04-19 before M33 took on the scheduler fix.

## Progress log

| Date | Entry |
|------|-------|
| 2026-04-19 | Activated milestone 32 and created this task from `docs/emulator/milestones/32.md`. |
| 2026-04-19 | Implemented the first frontend audio pass: positional ROM launch support for the milestone listening command, `AudioQueuePump` mixer-pull API that preserves pending PCM while back-pressured, explicit SDL `AUDIO_S16LSB` output format, fake-device lifecycle coverage, SDL dummy-device open/queue/close/reopen/error coverage, headless/frontend PCM parity coverage, PacManV8 300-frame digest regression coverage, and `docs/emulator/05-audio.md` shipping-path documentation. Automated verification passes; manual speaker listening remains pending before task completion. |
| 2026-04-19 | Blocked after manual report and follow-up investigation: the current PacManV8 T017 ROM produces black frames and deterministic silence in headless mode too, so SDL/frontend playback is not the root cause. |
| 2026-04-20 | Unblocked and marked completed: M33-T01 shipped the instruction-granular CPU/audio co-scheduling fix that was out of scope for M32, which made PacManV8 audible through the already-delivered frontend path. Moved from `docs/tasks/blocked/` to `docs/tasks/completed/`. |

## Incompletion summary

Blocked on 2026-04-19.

Automated frontend work completed:
- `vanguard8_frontend /path/to/rom` positional launch support for the
  milestone listening command.
- SDL queue path requests signed 16-bit little-endian stereo PCM to match
  `AudioMixer` bytes.
- `AudioQueuePump` can pull directly from `AudioMixer` without consuming
  pending PCM while SDL queue back-pressure is active.
- Fake-device lifecycle, SDL dummy-device open/queue/close/reopen/error,
  headless/frontend PCM parity, and PacManV8 300-frame digest regression
  coverage were added.
- `cmake --build cmake-build-debug` passed.
- `ctest --test-dir cmake-build-debug --output-on-failure` passed
  (156/156, with the pre-existing showcase test skipped).
- The milestone's canonical 300-frame digest command passed with
  `0d634fb059d15b12c4a8faf2412fbe08b85d187a31b1f22278ce3662f3b44390`.

Concrete blocker:
- The canonical digest is the SHA-256 of 959,128 zero bytes. It locks
  deterministic silence, not audible PacManV8 T016/T017 output.
- Headless frame dumps for frames 1, 60, 300, and 3,600 are all-black
  (`256x212`, zero nonzero pixel bytes).
- A diagnostic core probe after 300 frames reported:
  `pc=0x2b8b af=0x8090 bc=0x4201 de=0x0 hl=0x2c8c sp=0xfefc`,
  `audio_nonzero=0`, `vdp_a_r1=0`, `vdp_a_r8=0`, `vdp_b_r1=0`.
- The same probe after 3,600 frames reported the same `PC`, register state,
  zero nonzero PCM bytes, and zero VDP display registers.
- The ROM is stuck in the YM2151 busy-poll/write helper
  (`audio_ym_write_bc`, `PC 0x2B8B`) before video initialization and before
  the documented T016/T017 cue schedule.
- The likely underlying defect is core CPU/audio co-scheduling: inside
  `Emulator::run_cpu_until()`, `Bus::run_audio(delta)` advances audio for the
  whole event slice before executing the scheduled CPU instruction loop, but
  audio time is not advanced between individual CPU instructions inside that
  loop. A ROM busy-polling the YM2151 can therefore execute many poll
  iterations without the YM2151 busy state aging in lockstep.

Why this task cannot complete under the current contract:
- Milestone 32 forbids reopening scheduler work and requires preserving the
  current PacManV8 300-frame digest.
- Making the PacManV8 ROM audible requires changing the core CPU/audio timing
  relationship or otherwise changing ROM execution timing, which would also
  invalidate the current silence digest.
- The milestone contract is internally inconsistent: its exit criterion
  requires audible PacManV8 cues by frame 300, while its required digest guard
  currently proves all-zero PCM through frame 300.

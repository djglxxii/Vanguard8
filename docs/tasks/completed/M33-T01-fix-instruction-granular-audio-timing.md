# M33-T01 - Fix Instruction-Granular Audio Timing

Status: `completed`
Milestone: `33`
Depends on: `M32-T01`

Implements against:
- `docs/spec/00-overview.md`
- `docs/spec/01-cpu.md`
- `docs/spec/03-audio.md`
- `docs/spec/04-io.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/05-audio.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/33.md`
- `docs/tasks/blocked/M32-T01-frontend-live-audio-playback.md`

Scope:
- Fix the scheduled CPU/audio co-scheduling defect that leaves the current
  PacManV8 T017 ROM stuck in the YM2151 busy-poll helper at `PC 0x2B8B` with
  all-zero audio and all-black video through at least 3,600 frames.
- Keep the fix in the core timing path. The frontend SDL queue path is not in
  scope for this task.
- If the corrected timing exposes a new CPU opcode blocker before the M33 exit
  criteria can be verified, add only the specific extracted Z180 opcode needed.
  The first documented post-fix blocker is `ADD A,n` (`0xC6`) at `PC 0x2B1C`.
- Add focused tests for YM2151 busy-poll progress under scheduled CPU
  execution, deterministic repeatability, audio sample-count accounting, and
  PacManV8 nonzero output after the corrected timing model.
- Replace the invalid M32 all-zero digest guard with post-fix evidence.

Done when:
- A ROM-shaped YM2151 busy-poll loop advances because audio time ages between
  scheduled CPU instructions inside a frame.
- PacManV8 is no longer stuck at `PC 0x2B8B` in the review window.
- PacManV8 audio output bytes contain nonzero PCM in the review window, and
  the new SHA-256 is recorded.
- PacManV8 video output is nonblack at the selected review frame, or the task
  records the exact later nonblack frame and locks that in regression coverage.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes.

Out of scope:
- Frontend playback changes.
- New audio features or synthetic audio.
- VDP feature expansion.
- Broad CPU opcode completion outside a narrowly documented blocker surfaced
  by this timing fix.

Verification commands:
```bash
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure
cmake-build-debug/src/vanguard8_headless \
    --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
    --frames 300 --hash-audio
cmake-build-debug/src/vanguard8_headless \
    --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
    --frames 300 \
    --dump-frame /tmp/vanguard8-m33-pacman-frame-300.ppm \
    --hash-audio
```

## Completion summary

Completed on 2026-04-19.

Implemented against `docs/spec/00-overview.md`, `docs/spec/01-cpu.md`,
`docs/spec/03-audio.md`, `docs/spec/04-io.md`,
`docs/emulator/02-emulation-loop.md`, `docs/emulator/03-cpu-and-bus.md`,
`docs/emulator/05-audio.md`, and `docs/emulator/milestones/33.md`.

Core timing fix:
- `Emulator::run_cpu_until()` now advances audio, VDP command engines, CPU
  T-state accounting, and `master_cycle_` in scheduled instruction-sized
  chunks instead of advancing audio once before the whole event-slice CPU loop.
- HALT/no-instruction idle time still advances audio and VDP command state to
  the scheduler event boundary without banking unused HALT cycles for later
  instruction execution.
- The corrected timing lets ROM-visible YM2151 busy/status polling age between
  instructions inside a frame.

Narrow post-fix opcode blocker:
- After the timing fix moved PacManV8 past `PC 0x2B8B`, the ROM exposed
  missing timed opcode `ADD A,n` (`0xC6`) at `PC 0x2B1C`.
- The milestone contract was tightened to allow `third_party/z180/` only for
  this narrow post-fix blocker, then `ADD A,n` was implemented with timed
  adapter coverage and flag tests.

Regression coverage:
- Added a ROM-shaped YM2151 busy-poll integration test that writes YM2151 data,
  loops on `IN A,(0x40)` / `TST 0x80` / `JR NZ`, and reaches HALT inside one
  frame because busy clears as CPU instructions execute.
- Added deterministic repeat checks for the busy-poll scenario: completed frame
  count, final PC/register state, event-log digest, audio sample count, and
  audio digest.
- Added a HALT-heavy audio sample-count guard proving mixer sample count and
  output byte count remain derived from the master-cycle timeline.
- Replaced the invalid M32 all-zero PacManV8 guard with a nonzero 300-frame
  audio/video regression.

Observed PacManV8 evidence:
- `cmake-build-debug/src/vanguard8_headless --rom
  /home/djglxxii/src/PacManV8/build/pacman.rom --frames 300 --hash-audio`
  now reports audio SHA-256
  `9ced653d70f55ff0ce2b2e0d81a45ea18d204032ee9731ddd7d700bb10475a3e`.
- The old all-zero SHA-256
  `0d634fb059d15b12c4a8faf2412fbe08b85d187a31b1f22278ce3662f3b44390`
  is no longer accepted by tests.
- Frame 300 dumped to `/tmp/vanguard8-m33-pacman-frame-300.ppm` is nonblack;
  pixel payload SHA-256:
  `4a63cec305375edd4b20e85ba9830d83888e2eaf4327a29c229cfc7ce7a79693`.
- The PacManV8 regression test asserts CPU `PC != 0x2B8B`, nonzero PCM bytes,
  nonzero composed frame bytes, and the new audio SHA-256 above.

Verification:
- `cmake --build cmake-build-debug` passed.
- `ctest --test-dir cmake-build-debug --output-on-failure` passed
  (159/159; one pre-existing showcase test skipped).
- `cmake-build-debug/src/vanguard8_headless --rom
  /home/djglxxii/src/PacManV8/build/pacman.rom --frames 300 --hash-audio`
  passed with the new nonzero hash above.
- `cmake-build-debug/src/vanguard8_headless --rom
  /home/djglxxii/src/PacManV8/build/pacman.rom --frames 300 --dump-frame
  /tmp/vanguard8-m33-pacman-frame-300.ppm --hash-audio` passed with nonzero
  audio and nonblack frame output.

## Progress log

| Date | Entry |
|------|-------|
| 2026-04-19 | Created and activated after M32 was blocked by the PacManV8 YM2151 busy-poll/core timing defect. |
| 2026-04-19 | Completed core instruction-granular audio timing, added the narrow `ADD A,n` post-fix opcode blocker, replaced the M32 all-zero digest guard with nonzero PacManV8 audio/video evidence, and verified the milestone commands. |

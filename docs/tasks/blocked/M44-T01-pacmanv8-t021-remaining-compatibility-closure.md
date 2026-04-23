# M44-T01 — PacManV8 T021 Remaining Compatibility Closure

Status: `blocked`
Milestone: `44`
Depends on: `M43-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/44.md`
- `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`

Scope:
- Use the PacManV8 T021 blocked task as the running blocker ledger for
  the remaining Vanguard8-side work.
- Close the remaining timed HD64180 opcode gaps and narrow headless
  replay, inspection, or reporting mismatches directly exposed by the
  T021 replay path, instead of opening separate milestone/task pairs
  for each new blocker.
- Kick off from the current confirmed blocker:
  - `SUB E` (`0x93`) at `PC=0x0FAE` in `ghost_update_all_targets`.
  - Same-path look-ahead already identified in the blocked task:
    `SUB C` (`0x91`) at `PC=0x1169` and `PC=0x117A` in
    `ghost_candidate_distance`.
- Add focused tests, non-perturbation proof, and task-log updates for
  each blocker cleared under this milestone.
- Keep every newly surfaced in-scope T021 blocker under this single
  task file unless the remaining issue is proven external to Vanguard8
  or outside milestone 44's allowed subsystem surface.

Done when:
- The canonical PacManV8 T021 command completes end-to-end against
  Vanguard8 without a Vanguard8-side timed-opcode abort or
  headless/report-format mismatch.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes with
  all new coverage included and no pre-existing regression relaxed.
- Unaffected fixture replay, frame, audio, and event-log digests remain
  pinned unless an intentional change is explicitly recorded.
- This task ends with either a completion summary or an incompletion
  summary that proves the remaining blocker is external to Vanguard8 or
  outside milestone 44's scope.

Out of scope:
- Broad extracted-core CPU completion beyond replay-proven T021 gaps.
- PacManV8 ROM, harness, or evidence edits from this repo.
- Unrelated VDP, audio, scheduler, debugger, save-state, or desktop GUI
  changes.
- Reopening the per-opcode milestone split for the remaining T021 work.

Verification commands:
```bash
cd /home/djglxxii/src/PacManV8
python3 tools/build.py

cd /home/djglxxii/src/Vanguard8
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure

cd /home/djglxxii/src/PacManV8
python3 tools/pattern_replay_tests.py \
    --evidence-dir tests/evidence/T021-pattern-replay-and-fidelity-testing
```

## Progress log

| Date | Entry |
|------|-------|
| 2026-04-23 | Created to replace future per-opcode PacManV8 T021 micro-milestones after reviewing the latest blocked-task log. Starting point: timed `SUB E` (`0x93`) at `PC=0x0FAE`; same-path `SUB C` (`0x91`) look-ahead already identified. |
| 2026-04-23 | Added timed `SUB C` (`0x91`) and `SUB E` (`0x93`) coverage. Focused CPU coverage passed, and the PacManV8 T021 harness advanced past the prior `PC=0x0FAE` blocker and the same-path `SUB C` sites. |
| 2026-04-23 | The replay then exposed a new in-scope same-path opcode blocker: `Unsupported timed Z180 opcode 0x2F at PC 0x1187`, correlated to `CPL` in `ghost_abs_a` after `ghost_candidate_distance`. Added timed `CPL` coverage. |
| 2026-04-23 | With `SUB C`, `SUB E`, and `CPL` covered, the canonical T021 harness no longer aborts on a Vanguard8 timed opcode or headless report-format mismatch. It now runs to fidelity assertions and fails both PacManV8 replay cases with gameplay-state mismatches: score/dots remain `0/0` and Pac-Man remains at tile `(14, 26)` while `active=1`, `replay_frame` advances, and `last_dir` reflects replay input. Remaining blocker is outside Vanguard8 M44 scope. |

## Incompletion Summary

Date: 2026-04-23

Implemented against:
- `docs/spec/01-cpu.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/44.md`
- `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`

Completed within M44 scope:
- Added timed extracted-core support for `SUB C` (`0x91`) and
  `SUB E` (`0x93`) in the existing Z80-compatible arithmetic path.
  The implementation updates `A`, sets Z80 subtract flags, advances
  `PC` by one byte, and classifies both register forms as 4 T-states.
- Kept neighboring `SUB D` (`0x92`) unsupported to avoid a speculative
  `SUB r` family sweep beyond the replay-proven T021 scope.
- Added timed extracted-core support for `CPL` (`0x2F`) after the T021
  replay advanced to `PC=0x1187` in PacManV8 `ghost_abs_a`. The
  implementation complements `A`, sets H/N, preserves S/Z/PV/C, advances
  `PC` by one byte, and classifies it as 4 T-states.
- Added focused CPU coverage in `tests/test_cpu.cpp` for operand/result
  behavior, flag behavior, `PC` advance, adapter T-state classification,
  and the out-of-scope `SUB D` guard.

Verification completed:
- `cd /home/djglxxii/src/PacManV8 && python3 tools/build.py` passed and
  rebuilt `build/pacman.rom`.
- `cd /home/djglxxii/src/Vanguard8 && cmake --build
  cmake-build-debug` passed.
- Focused CPU coverage passed:
  `ctest --test-dir cmake-build-debug --output-on-failure -R
  "SUB C and SUB E|CPL used"`.
- Full Vanguard8 regression suite passed:
  `ctest --test-dir cmake-build-debug --output-on-failure` at `193/193`,
  with the pre-existing showcase milestone-7 test skipped.
- The direct frame-444 repro now exits cleanly instead of aborting on
  `0x93`:
  ```bash
  cd /home/djglxxii/src/Vanguard8
  cmake-build-debug/src/vanguard8_headless \
      --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
      --replay /home/djglxxii/src/PacManV8/tests/evidence/T021-pattern-replay-and-fidelity-testing/replays/early-level-pattern.v8r \
      --frames 444 \
      --inspect-frame 444 \
      --inspect /tmp/m44-t021-444.txt \
      --peek-logical 8270:13 \
      --hash-frame 444
  ```
  Representative output included `Frames completed: 444`, event-log
  digest `7898294682385330591`, and frame SHA-256
  `4a63cec305375edd4b20e85ba9830d83888e2eaf4327a29c229cfc7ce7a79693`.

Remaining blocker:
- The canonical PacManV8 T021 harness now runs without a Vanguard8
  timed-opcode abort or headless/report-format mismatch, but fails its
  replay fidelity assertions:
  ```text
  [FAIL] early-level-pattern
    failure: early-level-pattern/activated@90: score expected 60, observed 0
    failure: early-level-pattern/activated@90: dots expected 6, observed 0
    failure: early-level-pattern/activated@90: pac_tile expected (7, 26), observed (14, 26)
  [FAIL] short-corner-route
    failure: short-corner-route/activated@90: score expected 60, observed 0
    failure: short-corner-route/activated@90: dots expected 6, observed 0
    failure: short-corner-route/activated@90: pac_tile expected (7, 26), observed (14, 26)
  result: 0/2 passed
  ```
- The generated PacManV8 evidence shows `active=1`, `replay_frame`
  advances, and `last_dir` changes according to replay input at later
  checkpoints, so Vanguard8 replay loading and inspection output are
  functioning. The mismatched score, dot, and Pac-Man position state is
  a PacManV8 gameplay/expectation issue, not a Vanguard8 timed CPU or
  headless-observability defect.
- Milestone 44 forbids PacManV8 source, harness, or evidence edits from
  this repo and forbids unrelated subsystem work. No further Vanguard8
  changes are authorized until a PacManV8-side task or a new cross-repo
  contract identifies a Vanguard8 defect.

# M45-T01 — Timed HD64180 `AND r` / `AND (HL)` Opcode Coverage

Status: `blocked`
Milestone: `45`
Depends on: `M44-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/45.md`
- `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`

Scope:
- Add timed extracted-core coverage for the full base `AND r` register
  family plus `AND (HL)`:
  - `AND B` (`0xA0`)
  - `AND C` (`0xA1`)
  - `AND D` (`0xA2`)
  - `AND E` (`0xA3`) — currently confirmed T021 blocker at
    `PC=0x125B` in `collision_consume_tile`.
  - `AND H` (`0xA4`)
  - `AND L` (`0xA5`)
  - `AND (HL)` (`0xA6`) — currently suspected immediate follow-on
    at `PC=0x1260`.
  - `AND A` (`0xA7`)
- Add focused CPU tests that pin, for each opcode:
  - Result semantics: A ← A AND operand.
  - Flag semantics: S from bit 7 of result, Z from result == 0, H=1,
    P/V from parity of result, N=0, C=0.
  - `PC` advance (1 byte; none of these forms has an operand byte).
  - Adapter T-state classification, using the already-pinned patterns
    for register forms and memory-read forms in the existing CPU
    tests.
- For `AND (HL)`, the test must exercise the memory read path so the
  memory-read timing and bus classification is locked, not only the
  ALU path.
- Add a negative guard preserving the existing fail-fast
  `Unsupported timed Z180 opcode` contract for a still-out-of-scope
  sister form (for example `OR E` / `0xB3` or `XOR E` / `0xAB`).
- Regenerate non-perturbation proof against an existing fixture ROM
  that does not exercise the newly covered opcodes: frame, audio,
  and event-log digests must stay byte-identical before and after.
- Rerun the canonical PacManV8 T021 harness as the primary runtime
  proof. The harness must advance past both `PC=0x125B` and
  `PC=0x1260`; record the next distinct blocker in the progress log
  if one surfaces.

Done when:
- All eight opcodes above are dispatched by the timed extracted core
  with matching focused tests.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes with
  the new coverage included and no pre-existing regression relaxed.
- Fixture non-perturbation digests remain pinned.
- The PacManV8 T021 `tools/pattern_replay_tests.py` command either
  completes end-to-end or records a new distinct blocker that is
  explicitly out of M45 scope (and therefore requires a separate
  follow-up milestone contract — M45 does not bundle it).
- This task ends with either a completion summary or an incompletion
  summary naming the remaining blocker and the exact direct repro.

Out of scope:
- `OR r`, `XOR r`, `CP r`, rotate/shift, index-register, or
  ED-prefix forms. Those only come in-scope if a new T021 repro
  proves them, and in that case a new milestone contract is opened,
  not this task broadened.
- PacManV8 ROM, harness, or evidence edits from this repo.
- Unrelated VDP, audio, scheduler, debugger, save-state, headless
  format, or desktop GUI changes.
- Reopening the per-opcode milestone split for the remaining T021
  work.

Verification commands:
```bash
cd /home/djglxxii/src/PacManV8
python3 tools/build.py

cd /home/djglxxii/src/Vanguard8
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure

# Primary proof: the full T021 replay-validation harness completes, or
# records the next distinct blocker while remaining inside M45 scope.
cd /home/djglxxii/src/PacManV8
python3 tools/pattern_replay_tests.py \
    --evidence-dir tests/evidence/T021-pattern-replay-and-fidelity-testing

# Current direct repro anchor from the blocked task kickoff state.
cd /home/djglxxii/src/Vanguard8
cmake-build-debug/src/vanguard8_headless \
    --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
    --replay /home/djglxxii/src/PacManV8/tests/evidence/T021-pattern-replay-and-fidelity-testing/replays/early-level-pattern.v8r \
    --frames 40 \
    --inspect-frame 40 \
    --inspect /tmp/m45-t021-40.txt \
    --peek-logical 8270:13 \
    --hash-frame 40
```

## Progress log

| Date | Entry |
|------|-------|
| 2026-04-23 | Created, state: planned. Supersedes `M44-T01` (which hit an out-of-scope PacManV8 fidelity issue, now resolved by the PacManV8 team). Current confirmed T021 blocker is `Unsupported timed Z180 opcode 0xA3 at PC 0x125B` (`AND E`) with suspected immediate follow-on `AND (HL)` (`0xA6`) at `PC=0x1260`. |
| 2026-04-23 | Implemented the full M45 `AND r` / `AND (HL)` timed surface and focused CPU coverage. Vanguard8 verification passed, the frame-40 direct repro now clears the prior `AND E` / `AND (HL)` path, and the canonical PacManV8 T021 harness now stops on a new out-of-scope timed opcode: `Unsupported timed Z180 opcode 0xD6 at PC 0x3EA`. Moving task to blocked pending a follow-up milestone contract. |

## Incompletion Summary

Date: 2026-04-23

Implemented against:
- `docs/spec/01-cpu.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/45.md`
- `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`

Completed within M45 scope:
- Added timed extracted-core support for the full base `AND r`
  family and `AND (HL)`:
  `AND B` (`0xA0`), `AND C` (`0xA1`), `AND D` (`0xA2`),
  `AND E` (`0xA3`), `AND H` (`0xA4`), `AND L` (`0xA5`),
  `AND (HL)` (`0xA6`), and `AND A` (`0xA7`).
- Factored the existing `AND n` flag behavior so all covered `AND`
  forms set S from bit 7, Z from zero result, H=1, P/V from parity,
  and clear N/C.
- Added adapter timing classification: register forms cost 4 T-states;
  `AND (HL)` costs 7 T-states and uses the logical memory read path.
- Added focused CPU coverage in `tests/test_cpu.cpp` for every M45
  opcode, including operand/result behavior, flag behavior, `PC`
  advance, T-state classification, an `AND (HL)` memory-read
  breakpoint, and a negative guard proving `XOR E` (`0xAB`) remains
  unsupported as an out-of-scope sister opcode.

Verification completed:
- `cd /home/djglxxii/src/Vanguard8 && cmake --build
  cmake-build-debug` passed.
- Focused M45 CPU coverage passed:
  ```bash
  ctest --test-dir cmake-build-debug -R "AND r and AND" --output-on-failure
  ```
- Full Vanguard8 regression suite passed:
  `ctest --test-dir cmake-build-debug --output-on-failure` at
  `194/194`, with the pre-existing showcase milestone-7 test skipped.
  The existing `vanguard8_headless_replay_regression` ctest entry
  also passed, preserving the fixture frame/audio/event-log
  non-perturbation guard.
- `cd /home/djglxxii/src/PacManV8 && python3 tools/build.py` passed
  and rebuilt `build/pacman.rom`.
- The M45 direct frame-40 repro now exits cleanly instead of aborting
  on `AND E` (`0xA3`) or the follow-on `AND (HL)` (`0xA6`):
  ```bash
  cd /home/djglxxii/src/Vanguard8
  cmake-build-debug/src/vanguard8_headless \
      --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
      --replay /home/djglxxii/src/PacManV8/tests/evidence/T021-pattern-replay-and-fidelity-testing/replays/early-level-pattern.v8r \
      --frames 40 \
      --inspect-frame 40 \
      --inspect /tmp/m45-t021-40.txt \
      --peek-logical 8270:13 \
      --hash-frame 40
  ```
  Representative output included `Frames completed: 40`, event-log
  digest `9745782898622768779`, and frame SHA-256
  `4a63cec305375edd4b20e85ba9830d83888e2eaf4327a29c229cfc7ce7a79693`.

Remaining blocker:
- The canonical PacManV8 T021 harness now advances past the M45
  `AND E` / `AND (HL)` sites, then stops on a new distinct
  out-of-scope timed opcode:
  ```text
  terminate called after throwing an instance of 'std::runtime_error'
    what():  Unsupported timed Z180 opcode 0xD6 at PC 0x3EA
  ```
- `0xD6` is `SUB n`, which is outside M45's allowed `AND r` /
  `AND (HL)` scope. The PC maps to PacManV8
  `movement_distance_to_next_center_px`, specifically
  `/home/djglxxii/src/PacManV8/src/movement.asm:326`
  (`sub MOVEMENT_TILE_CENTER`).
- Direct repro:
  ```bash
  cd /home/djglxxii/src/Vanguard8
  cmake-build-debug/src/vanguard8_headless \
      --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
      --replay /home/djglxxii/src/PacManV8/tests/evidence/T021-pattern-replay-and-fidelity-testing/replays/early-level-pattern.v8r \
      --frames 92 \
      --hash-frame 92
  ```
  `--frames 91` passes; `--frames 92` aborts with the `0xD6` error above.

Resolution needed:
- Define a follow-up milestone/task for `SUB n` (`0xD6`) or whatever
  exact replay-proven opcode family the next contract authorizes.
  Do not broaden M45 to include non-`AND` opcodes.

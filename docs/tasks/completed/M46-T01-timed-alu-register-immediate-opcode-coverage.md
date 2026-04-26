# M46-T01 — Timed HD64180 ALU Register/Immediate Tail Opcode Coverage

Status: `active`
Milestone: `46`
Depends on: `M45-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/46.md`
- `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`
  (in particular the `### Recommended fix (Vanguard8 repo)` section)

## Scope

Add timed extracted-core coverage for the remaining ALU register
and immediate opcode tail in a single pass, following the structure
set out in the PacManV8 T021 blocked-task "Recommended fix
(Vanguard8 repo)" section.

**Step 1 — new range-checks** in
`src/core/cpu/z180_adapter.cpp`,
`Z180Adapter::current_instruction_tstates()`, inserted alongside
the existing `LD r,r'`, `ADD A,r`, and `AND r` range-checks (right
after the `AND r` block, before the opening `switch (opcode)`):

```cpp
    // ADC A,r  (0x88..0x8F; 0x8E = ADC A,(HL))
    if (opcode >= 0x88U && opcode <= 0x8FU) {
        return opcode == 0x8EU ? 7 : 4;
    }
    // SUB r    (0x90..0x97; 0x96 = SUB (HL))
    if (opcode >= 0x90U && opcode <= 0x97U) {
        return opcode == 0x96U ? 7 : 4;
    }
    // SBC A,r  (0x98..0x9F; 0x9E = SBC A,(HL))
    if (opcode >= 0x98U && opcode <= 0x9FU) {
        return opcode == 0x9EU ? 7 : 4;
    }
    // XOR r    (0xA8..0xAF; 0xAE = XOR (HL))
    if (opcode >= 0xA8U && opcode <= 0xAFU) {
        return opcode == 0xAEU ? 7 : 4;
    }
```

**Step 1b — remove redundant explicit entries** that are now
subsumed by the new range-checks. These lines currently live in
the `return 4` switch block of the same function:

- `case 0x91:` — `SUB C`. Covered by the new `SUB r` range-check.
- `case 0x93:` — `SUB E`. Covered by the new `SUB r` range-check.
- `case 0xAF:` — `XOR A`. Covered by the new `XOR r` range-check.

Do **not** touch the existing explicit `OR r` (`0xB0..0xB7`) or
`CP r` (`0xB8..0xBF`) entries — those ranges are already fully
covered, and the recommended fix explicitly excludes them from
this patch.

**Step 2 — immediate-arithmetic peers.** The `return 7` case-label
list (currently containing `ADD A,n` / `0xC6`, `AND n` / `0xE6`,
`OR n` / `0xF6`, and `CP n` / `0xFE`) must also list the four
missing peers, in the same block:

```cpp
    case 0xCE:   // ADC A,n
    case 0xD6:   // SUB n           <-- confirmed failing at PC=0x3EA
    case 0xDE:   // SBC A,n
    case 0xEE:   // XOR n
        return 7;
```

**Step 3 — focused CPU tests.** Extend `tests/test_cpu.cpp` with
timed-opcode coverage for the new families, following the patterns
already used by the `SUB C` / `SUB E` (milestone 44) and `AND r`
(milestone 45) tests. At minimum assert that
`current_instruction_tstates()` returns:

- **4** for each register form:
  - `ADC A,r` (`0x88..0x8D`, `0x8F`)
  - `SUB r`   (`0x90..0x95`, `0x97`)
  - `SBC A,r` (`0x98..0x9D`, `0x9F`)
  - `XOR r`   (`0xA8..0xAD`, `0xAF`)
- **7** for each `(HL)` form:
  - `ADC A,(HL)` (`0x8E`)
  - `SUB (HL)`   (`0x96`)
  - `SBC A,(HL)` (`0x9E`)
  - `XOR (HL)`   (`0xAE`)
- **7** for each immediate form:
  - `ADC A,n` (`0xCE`)
  - `SUB n`   (`0xD6`)
  - `SBC A,n` (`0xDE`)
  - `XOR n`   (`0xEE`)

For each family, add at least one representative case pinning
operand/result and flag behavior per published HD64180/Z80
semantics:

- `ADC A,*`: A ← A + operand + C; S from bit 7, Z from zero, H
  from bit-3 carry, P/V from signed overflow, N=0, C from bit-7
  carry.
- `SUB *` : A ← A − operand; S from bit 7, Z from zero, H from
  bit-3 borrow, P/V from signed overflow, N=1, C from bit-7
  borrow.
- `SBC A,*`: A ← A − operand − C; flags as for `SUB` but
  accounting for the carry input.
- `XOR *`: A ← A XOR operand; S from bit 7, Z from zero, H=0, P/V
  from parity, N=0, C=0.

`PC` advance must be pinned: 1 byte for register and `(HL)` forms,
2 bytes for each immediate form.

For each `(HL)` form (`0x8E`, `0x96`, `0x9E`, `0xAE`), the test
must exercise the memory read path so the adapter's memory-read
timing and bus classification are pinned, not only the ALU path.

Add a negative guard preserving the existing fail-fast
`Unsupported timed Z180 opcode` contract for a still-out-of-scope
sister form (for example one of the rotate/shift family bases or
an ED-prefix opcode). This catches accidental broadening of M46
scope.

Regenerate non-perturbation proof against an existing fixture ROM
that does not exercise the newly covered opcodes: frame, audio,
and event-log digests must stay byte-identical before and after.

Rerun the canonical PacManV8 T021 harness as the primary runtime
proof. The harness must advance past `PC=0x3EA` (`SUB n`) and the
same-function `SUB B` sites at `PC=0x3BE`/`0x3CA`/`0x3D2` without
aborting; record the next distinct blocker in the progress log if
one surfaces.

## Done when

- All four new register-form range-checks are present in
  `Z180Adapter::current_instruction_tstates()` and the three
  redundant explicit entries (`0x91`, `0x93`, `0xAF`) have been
  removed from the `return 4` switch block.
- All four new immediate forms (`0xCE`, `0xD6`, `0xDE`, `0xEE`)
  are dispatched by the timed extracted core alongside the existing
  `ADD A,n` / `AND n` / `OR n` / `CP n` entries.
- Focused tests pin the adapter T-state classification, operand/
  result, flag behavior, `PC` advance, and memory-read path for each
  newly covered opcode in the list above.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes
  with the new coverage included and no pre-existing regression
  relaxed.
- Fixture non-perturbation digests remain pinned.
- The PacManV8 T021 `tools/pattern_replay_tests.py` command either
  completes end-to-end or records a new distinct blocker that is
  explicitly out of M46 scope (and therefore requires a separate
  follow-up milestone contract — M46 does not bundle it).
- This task ends with either a completion summary or an
  incompletion summary naming the remaining blocker and the exact
  direct repro.

## Out of scope

- Rotate/shift families, index-register (`IX`/`IY`) forms, and
  ED-prefix forms. Those only come in-scope if a new T021 repro
  proves them, and in that case a new milestone contract is
  opened, not this task broadened.
- Modifying the existing explicit `OR r` (`0xB0..0xB7`) or `CP r`
  (`0xB8..0xBF`) entries.
- PacManV8 ROM, harness, or evidence edits from this repo.
- Unrelated VDP, audio, scheduler, debugger, save-state, headless
  format, or desktop GUI changes.
- Reopening the per-opcode milestone split for the remaining T021
  work.

## Verification commands

```bash
cd /home/djglxxii/src/PacManV8
python3 tools/build.py

cd /home/djglxxii/src/Vanguard8
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure

# Primary proof: the full T021 replay-validation harness completes,
# or records the next distinct blocker while remaining inside M46
# scope.
cd /home/djglxxii/src/PacManV8
python3 tools/pattern_replay_tests.py \
    --evidence-dir tests/evidence/T021-pattern-replay-and-fidelity-testing

# Current direct repro anchor from the blocked-task kickoff state.
# Pre-patch: aborts with
#   "Unsupported timed Z180 opcode 0xD6 at PC 0x3EA"
# Post-patch: must clear the SUB n site (and the sibling SUB B
# sites) and either complete or record the next distinct blocker.
cd /home/djglxxii/src/Vanguard8
cmake-build-debug/src/vanguard8_headless \
    --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
    --replay /home/djglxxii/src/PacManV8/tests/evidence/T021-pattern-replay-and-fidelity-testing/replays/early-level-pattern.v8r \
    --frames 92 \
    --hash-frame 92
```

## Progress log

| Date | Entry |
|------|-------|
| 2026-04-24 | Created, state: active. Built from the PacManV8 T021 blocked-task `### Recommended fix (Vanguard8 repo)` section, which authorizes closing the full register/immediate ALU timed-opcode tail (`ADC A,r`, `SUB r`, `SBC A,r`, `XOR r`, plus `ADC A,n` / `SUB n` / `SBC A,n` / `XOR n`) in one pass. Activation blocker: `Unsupported timed Z180 opcode 0xD6 at PC 0x3EA` (`SUB n` in `movement_distance_to_next_center_px`). |
| 2026-04-24 | Implemented the full M46 ALU register/immediate tail and verification passed. The canonical PacManV8 T021 harness now passes both replay cases end-to-end, so no follow-up blocker was surfaced. Moving task to completed. |

## Completion Summary

Date: 2026-04-24

Implemented against:
- `docs/spec/01-cpu.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/46.md`
- `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`

Completed within M46 scope:
- Added timed extracted-core execution support for all M46 register-form
  ALU families:
  `ADC A,r` (`0x88..0x8F`), `SUB r` (`0x90..0x97`),
  `SBC A,r` (`0x98..0x9F`), and `XOR r` (`0xA8..0xAF`).
  The `(HL)` forms (`0x8E`, `0x96`, `0x9E`, `0xAE`) read through
  the logical-memory path.
- Added immediate ALU execution support for `ADC A,n` (`0xCE`),
  `SUB n` (`0xD6`), `SBC A,n` (`0xDE`), and `XOR n` (`0xEE`).
- Added adapter timing classification in
  `Z180Adapter::current_instruction_tstates()`: register forms cost
  4 T-states; `(HL)` and immediate forms cost 7 T-states.
- Removed the now-redundant explicit adapter switch entries for
  `0x91`, `0x93`, and `0xAF`. The existing explicit `OR r`
  (`0xB0..0xB7`) and `CP r` (`0xB8..0xBF`) entries were left
  unchanged as required.
- Added focused CPU coverage in `tests/test_cpu.cpp` for every M46
  opcode's T-state classification, representative operand/result and
  flag behavior for each family, PC advance for register/`(HL)` and
  immediate forms, memory-read breakpoints for all four `(HL)` forms,
  and an out-of-scope ED-prefix negative guard.
- Updated older out-of-scope guards that intentionally expected
  `SUB D` and `XOR E` to be unsupported before M46. They now guard
  still-out-of-scope rotate/CB-prefix opcodes instead.

Verification completed:
- `cd /home/djglxxii/src/Vanguard8 && cmake --build
  cmake-build-debug` passed.
- Focused CPU coverage passed:
  `cmake-build-debug/tests/vanguard8_tests "[cpu]"` with 72 test
  cases and 1251 assertions.
- Full Vanguard8 regression suite passed:
  `ctest --test-dir cmake-build-debug --output-on-failure` at
  `195/195`, with the pre-existing showcase milestone-7 test skipped.
  The existing `vanguard8_headless_replay_regression` ctest entry
  passed.
- Explicit non-perturbation fixture replay passed:
  ```bash
  cmake-build-debug/src/vanguard8_headless \
      --rom tests/replays/replay_fixture.rom \
      --replay tests/replays/replay_fixture.v8r \
      --frames 60 \
      --hash-frame 60 \
      --hash-audio
  ```
  Representative output: event-log digest `6563162820683566367`,
  frame 60 SHA-256
  `e46b5246bda293e09e199967b99ac352f931c04e2ad88e775b06a3b93ccb838c`,
  and audio SHA-256
  `734c69bba0fceee9199a4d44febcae8bdde585874f1f7cb260d80b6ea7683d68`.
- `cd /home/djglxxii/src/PacManV8 && python3 tools/build.py`
  passed and rebuilt `build/pacman.rom`.
- The canonical PacManV8 T021 harness passed end-to-end:
  ```bash
  cd /home/djglxxii/src/PacManV8
  python3 tools/pattern_replay_tests.py \
      --evidence-dir tests/evidence/T021-pattern-replay-and-fidelity-testing
  ```
  Result: `2/2 passed` (`early-level-pattern` and
  `short-corner-route`).
- The direct frame-92 repro now exits cleanly instead of aborting
  on `SUB n` (`0xD6`) or the sibling `SUB B` sites:
  ```bash
  cd /home/djglxxii/src/Vanguard8
  cmake-build-debug/src/vanguard8_headless \
      --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
      --replay /home/djglxxii/src/PacManV8/tests/evidence/T021-pattern-replay-and-fidelity-testing/replays/early-level-pattern.v8r \
      --frames 92 \
      --hash-frame 92
  ```
  Representative output: `Frames completed: 92`, event-log digest
  `11640653684771125119`, and frame SHA-256
  `4a63cec305375edd4b20e85ba9830d83888e2eaf4327a29c229cfc7ce7a79693`.

Remaining blocker:
- None surfaced by M46 verification. The PacManV8 T021 replay
  validation harness passes both cases end-to-end.

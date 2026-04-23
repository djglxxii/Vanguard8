# M39-T01 — Timed HD64180 16-bit Pair INC/DEC and CB-Prefix Opcode Coverage for PacManV8 T021

Status: `blocked`
Milestone: `39`
Depends on: `M38-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/39.md`
- `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`

Scope:
- Implement timed execution for four missing 16-bit register-pair
  INC/DEC opcodes in the extracted HD64180 runtime, as authorized by
  milestone 39:
  - `DEC BC` (`0x0B`) — the confirmed blocker at PacManV8 `PC 0x1209`
    in `collision_init`.
  - `INC DE` (`0x13`) — look-ahead in the same `collision_init` loop,
    advancing the pellet-bitset byte pointer after each 8-cell mask
    wrap.
  - `DEC HL` (`0x2B`) — look-ahead in `ghost_mode_tick` /
    `ghost_mode_tick_frightened` and the score / dot bookkeeping
    paths reached by replay validation.
  - `INC BC` (`0x03`) — sister form of `DEC BC`, closed in this pass
    to avoid another single-opcode rebuild cycle on the BC half of
    the 16-bit pair INC/DEC family (precedent: M34, M37, M38).
- Each 16-bit pair INC/DEC opcode must:
  - Apply a 16-bit `+1` (`INC`) or `-1` (`DEC`) to the target pair
    with natural 16-bit wrap (`0xFFFF + 1 → 0x0000`,
    `0x0000 - 1 → 0xFFFF`).
  - Advance `PC` by 1.
  - Leave all flags (S/Z/H/PV/N/C) untouched.
  - Report **6 T-states** at the adapter boundary, registered in the
    existing 6-T-state group in `src/core/cpu/z180_adapter.cpp`
    alongside `0x1B` (`DEC DE`) and `0x23` (`INC HL`).
- Implementation must follow the existing pattern already used for
  `op_dec_de` and `op_inc_hl` in `third_party/z180/z180_core.cpp`
  (single 16-bit add or subtract on the target pair, no flag updates).
- Implement the first CB-prefix dispatch surface in the extracted
  HD64180 runtime, restricted strictly to the sub-opcodes listed in
  the T021 blocker:
  - `SRL A` (`CB 3F`) — shift `A` right by one: bit 0 → C; bit 7 ←
    0; S cleared (always, since bit 7 ends at 0); Z reflects the new
    `A == 0`; H cleared; PV reflects parity of new `A`; N cleared;
    `PC` advances by 2; adapter reports **8 T-states**.
  - `BIT 4,A` (`CB 67`), `BIT 5,A` (`CB 6F`), `BIT 6,A` (`CB 77`),
    `BIT 7,A` (`CB 7F`) — Z set to inverse of the tested bit of `A`;
    H set; N cleared; S and PV per the standard `BIT b,r` semantics
    for `r=A`; C unchanged; `A` unchanged; `PC` advances by 2;
    adapter reports **8 T-states**.
- The CB-prefix dispatch handler must:
  - Register against `opcodes_[0xCB]` in
    `Core::install_opcode_table()`.
  - Fetch the next byte as the CB sub-opcode using the same
    `fetch_byte()` helper used elsewhere in the core.
  - Route only the listed CB sub-opcodes to their handlers.
  - Throw a `std::runtime_error` for any other CB sub-opcode using
    the existing
    `Unsupported timed Z180 opcode 0xCB <subop> at PC <pc>` message
    style, so the "fail fast on untimed" contract is preserved
    end-to-end.
- The adapter must route `case 0xCB:` in
  `Z180Adapter::instruction_tstates(...)` to a new helper
  `cb_instruction_tstates(subop)` that:
  - Returns 8 for each of the listed CB sub-opcodes.
  - Falls through to the same unsupported-opcode runtime error
    pattern used by the main dispatch and by `ed_instruction_tstates`
    for any other CB sub-opcode.

Blocker reference:
- PacManV8 blocked task:
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`
- Key evidence from that blocker:
  - The canonical headless runtime aborts with
    ```text
    terminate called after throwing an instance of 'std::runtime_error'
      what():  Unsupported timed Z180 opcode 0xB at PC 0x1209
    ```
    while running the T021 replay validation harness.
  - Failure site is `collision_init` (begins at `0x1198`); ROM bytes
    around the abort:
    ```text
    00001200: 87 20 03 3e 01 13 32 12 82 0b 78 b1 20 c1 c9 af
                                      ^^
                                      DEC BC at PC=0x1209
    ```
  - Strongly suspected next timed-opcode gaps in the same active path:
    - `INC DE` (`0x13`) — used in `collision_init` to advance to the
      next pellet-bitset byte after each 8-cell mask wrap.
    - `DEC HL` (`0x2B`) — used by `ghost_mode_tick` /
      `ghost_mode_tick_frightened` and the score / dot bookkeeping
      paths reached by replay validation.
    - `CB`-prefix forms `SRL A` (existing movement/collision code) and
      `BIT 4..7,A` (new replay input decoder); the current emulator
      timed-dispatch table has no `0xCB` entry at all.
  - The PacManV8 source-level T021 work is otherwise complete: the
    replay asset pipeline, the deterministic checkpoint harness
    (`tools/pattern_replay_tests.py`), and the human-readable replay
    sources under `tests/replays/pattern_sources/` are in place; only
    emulator-side opcode coverage blocks acceptance.
- Canonical repro command (post-fix expectation in parentheses):
  ```bash
  cd /home/djglxxii/src/PacManV8
  python3 tools/build.py
  python3 tools/pattern_replay_tests.py \
      --evidence-dir tests/evidence/T021-pattern-replay-and-fidelity-testing
  # (expected: completes without "Unsupported timed Z180 opcode"
  # abort at 0x0B / 0x13 / 0x2B / 0xCB; reaches at least the first
  # authored replay checkpoint with stable digest output across
  # repeat runs)
  ```

Done when:
- New CPU tests pin `INC BC`, `DEC BC`, `INC DE`, and `DEC HL`
  behavior (including 16-bit wrap edges), `SRL A` semantics
  (including the bit-0-set carry case and the all-zero result Z
  case), and `BIT 4..7,A` semantics for both the bit-clear (Z=1) and
  bit-set (Z=0) cases, plus adapter T-state classification per the
  milestone contract.
- Negative tests confirm that an out-of-scope CB sub-opcode
  (`RES 0,B` / `CB 80`) and an out-of-scope main-table 16-bit pair
  INC/DEC opcode (`INC SP` / `0x33`) still raise the existing
  `Unsupported timed Z180 opcode` runtime error, with the offending
  opcode (or `0xCB <subop>`) and PC reported in the message.
- The PacManV8 T021 repro command runs to completion without
  `Unsupported timed Z180 opcode 0xB at PC 0x1209` (and without any
  follow-on `0x13` / `0x2B` / `0xCB` abort along the active replay
  validation path), and the replay validation harness reaches at
  least its first authored checkpoint with stable digest output
  across repeat runs.
- A non-perturbation regression on an existing fixture ROM that does
  not exercise the new opcodes produces byte-identical frame, audio,
  and event-log digests before and after the fix. Reuse an existing
  M33 / M35 / M37 / M38-verified fixture already present under
  `tests/`.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes with
  the new opcode tests included and no pre-existing CPU test is
  relaxed.

Out of scope:
- Any general HD64180 opcode completion beyond the four 16-bit pair
  INC/DEC forms and the listed CB sub-opcodes above.
- `INC SP` (`0x33`) and `DEC SP` (`0x3B`). They are not on any
  confirmed active path in the T021 blocker or its look-ahead and
  are explicitly out of scope.
- Broader CB-prefix coverage: no `RES b,r`, no `SET b,r`, no
  `BIT b,r` for `r ≠ A`, no `(HL)`-target shift / rotate / bit-test
  forms, no rotate/shift forms other than `SRL A` (so no `RLC`,
  `RRC`, `RL`, `RR`, `SLA`, `SRA`, `SLL`), and no
  `DD CB d <op>` / `FD CB d <op>` indexed CB variants.
- Changes to interrupt acceptance, HALT-wake semantics, M35
  observability surface, M37 `LD r,n` coverage, M38 `RET cc`
  coverage, VDP, audio, scheduler, frontend, debugger, save-state,
  or PacManV8 ROM code.
- Refreshing PacManV8 T021 acceptance evidence; that happens in the
  PacManV8 repo after this task is completed.

Required tests:
- Extracted-core coverage in `tests/test_cpu.cpp` (or the adjacent
  CPU test file the project already uses):
  - `DEC BC` (`0x0B`): with `BC = 0x1234`, after execution
    `BC == 0x1233`, all other registers unchanged, `PC` advanced by
    1, all flags unchanged, adapter reports 6 T-states. Include a
    second case with `BC = 0x0000` to confirm the
    `0x0000 → 0xFFFF` wrap.
  - `INC BC` (`0x03`): with `BC = 0x1234`, after execution
    `BC == 0x1235`; second case with `BC = 0xFFFF` to confirm the
    `0xFFFF → 0x0000` wrap.
  - `INC DE` (`0x13`): same contract for the `DE` pair, including
    the `0xFFFF → 0x0000` wrap.
  - `DEC HL` (`0x2B`): same contract for the `HL` pair, including
    the `0x0000 → 0xFFFF` wrap.
  - `collision_init`-style sequence around `PC = 0x1209`: load `BC`
    with a small non-zero count, run `DEC BC` followed by an
    already-timed opcode (`LD A,B` / `OR C` / `JR NZ,e`) to confirm
    the decrement participates correctly in the surrounding loop and
    that the loop terminates on `BC == 0`.
  - `SRL A` (`CB 3F`): with `A = 0x81`, after execution `A == 0x40`,
    C set (from old bit 0), S cleared, Z cleared, H cleared, PV
    reflects parity of `0x40`, N cleared, `PC` advanced by 2,
    adapter reports 8 T-states. Second case with `A = 0x01`
    confirming Z is set when the result is zero and C is set from
    the old bit 0.
  - `BIT 4,A` (`CB 67`), `BIT 5,A` (`CB 6F`), `BIT 6,A` (`CB 77`),
    `BIT 7,A` (`CB 7F`): for each, two cases — one with the tested
    bit clear (Z = 1) and one with the tested bit set (Z = 0).
    Confirm H is set, N is cleared, C is preserved across the
    instruction, `A` is preserved, `PC` advanced by 2, adapter
    reports 8 T-states.
  - Negative test: executing `RES 0,B` (`CB 80`) raises
    `std::runtime_error` whose message contains `Unsupported timed
    Z180 opcode 0xCB 0x80 at PC` (or the equivalent format used by
    the existing main / ED dispatch error path), confirming the
    unsupported-CB-subop fail-fast contract.
  - Negative test: executing `INC SP` (`0x33`) raises
    `std::runtime_error` whose message contains
    `Unsupported timed Z180 opcode 0x33 at PC`, confirming the
    out-of-scope main-table opcode is **not** silently routed to a
    16-bit pair INC/DEC handler.
- Adapter-level T-state coverage confirming each new opcode is
  classified in the correct group (6 T-states for the 16-bit pair
  INC/DEC forms; 8 T-states for the listed CB sub-opcodes via
  `cb_instruction_tstates`).
- Non-perturbation guard against a fixture ROM that does not
  exercise the new opcodes: frame, audio, and event-log digests must
  be byte-identical before and after the fix.

Verification commands:
```bash
cd /home/djglxxii/src/PacManV8
python3 tools/build.py

cd /home/djglxxii/src/Vanguard8
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure

# Primary proof: the T021 replay validation harness now runs past
# PC=0x1209 (DEC BC in collision_init) and past the follow-on
# INC DE / DEC HL / CB-prefix sites without an "Unsupported timed
# Z180 opcode" abort, and reaches at least the first authored
# checkpoint with stable digest output across repeat runs.
cd /home/djglxxii/src/PacManV8
python3 tools/pattern_replay_tests.py \
    --evidence-dir tests/evidence/T021-pattern-replay-and-fidelity-testing

# Headless smoke proof from the Vanguard 8 side: a short headless
# run against the PacManV8 ROM that exercises collision_init no
# longer aborts on 0x0B / 0x13 / 0x2B / 0xCB and produces a stable
# frame hash across repeat runs.
cd /home/djglxxii/src/Vanguard8
cmake-build-debug/src/vanguard8_headless \
    --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
    --frames 60 --hash-frame 60
```

## Incompletion Summary

Date: 2026-04-22

Implemented against `docs/spec/01-cpu.md`,
`docs/emulator/02-emulation-loop.md`,
`docs/emulator/03-cpu-and-bus.md`, and
`docs/emulator/milestones/39.md`.

Completed within the active milestone scope:
- Added timed extracted-core execution for `INC BC` (`0x03`), `DEC BC`
  (`0x0B`), `INC DE` (`0x13`), and `DEC HL` (`0x2B`), preserving flags,
  using natural 16-bit wrap, and advancing `PC` by 1.
- Added the narrow `CB` prefix dispatch for `SRL A` (`CB 3F`) and
  `BIT 4..7,A` (`CB 67`/`6F`/`77`/`7F`), with unsupported CB sub-opcodes
  still failing fast.
- Added adapter timing classification: 6 T-states for the new pair
  INC/DEC opcodes and 8 T-states for the listed CB sub-opcodes.
- Added focused CPU tests for normal and wrap cases, CB flag semantics,
  the `collision_init`-style `DEC BC` loop, adapter timing, and the
  required negative cases for `CB 80` and `INC SP` (`0x33`).

Verification completed:
- `cmake --build cmake-build-debug` passed.
- `ctest --test-dir cmake-build-debug --output-on-failure` passed:
  182 tests total, 181 run, 1 existing skipped.
- `cd /home/djglxxii/src/PacManV8 && python3 tools/build.py` passed.
- Vanguard8 no-input headless smoke passed:
  `cmake-build-debug/src/vanguard8_headless --rom /home/djglxxii/src/PacManV8/build/pacman.rom --frames 60 --hash-frame 60`
  produced frame SHA-256
  `4a63cec305375edd4b20e85ba9830d83888e2eaf4327a29c229cfc7ce7a79693`
  and event-log digest `6563162820683566367`.

Concrete blocker:
- The PacManV8 T021 replay validation harness now runs past the original
  `DEC BC` blocker at `PC=0x1209`, but it cannot complete because it
  reaches a new out-of-scope timed opcode:

  ```text
  terminate called after throwing an instance of 'std::runtime_error'
    what():  Unsupported timed Z180 opcode 0xEB at PC 0x11DA
  ```

`0xEB` is `EX DE,HL`. It is not listed in milestone 39 allowed scope and
is not one of the authorized look-ahead opcodes, so it was not implemented
under this task. A follow-up milestone/task is needed to authorize and test
that opcode before the T021 replay harness can satisfy its full done
criteria.

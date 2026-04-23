# M41-T01 — Timed HD64180 `SRL H` and `RR L` CB-Prefix Opcode Coverage for PacManV8 T021

Status: `blocked`
Milestone: `41`
Depends on: `M40-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/41.md`
- `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`

Scope:
- Implement timed execution for exactly two missing CB-prefix
  sub-opcodes in the extracted HD64180 runtime, as authorized by
  milestone 41:
  - `SRL H` (`0xCB 0x3C`) — the confirmed blocker at PacManV8
    `PC 0x1302` in the `collision_prepare_tile` divide-by-eight path.
  - `RR L` (`0xCB 0x1D`) — the immediate same-path look-ahead opcode
    after the confirmed `SRL H` site.
- `SRL H` must:
  - Shift `H` right by one logical bit: new `H = H >> 1`, with bit 7
    of new `H` cleared; new carry = old `H` bit 0.
  - Update flags: S cleared, Z from new `H == 0`, H cleared, P/V from
    parity of new `H`, N cleared, C as above.
  - Leave `L`, `A`, and all other registers untouched.
  - Advance `PC` by 2 (CB prefix + sub-opcode) through normal fetch.
  - Report **8 T-states** at the adapter boundary, routed through the
    existing M39 `cb_instruction_tstates` helper.
- `RR L` must:
  - Rotate `L` right through carry: new bit 7 = old carry, new
    bit 0..6 = old bit 1..7, new carry = old bit 0.
  - Update flags: S from new bit 7 of `L`, Z from new `L == 0`, H
    cleared, P/V from parity of new `L`, N cleared, C as above.
  - Leave `H`, `A`, and all other registers untouched.
  - Advance `PC` by 2 through normal fetch.
  - Report **8 T-states** at the adapter boundary, routed through the
    existing `cb_instruction_tstates` helper.
- Extend `Core::op_cb_prefix` in `third_party/z180/z180_core.cpp` to
  dispatch `0x3C` and `0x1D`, preserving the existing fail-fast
  contract for every other still-unsupported CB sub-opcode.

Blocker reference:
- PacManV8 blocked task:
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`
- Key evidence from that blocker:
  - After M40, the canonical replay harness passes the previous
    `EX DE,HL` / `OR (HL)` sites and aborts with:
    ```text
    terminate called after throwing an instance of 'std::runtime_error'
      what():  Unsupported timed Z180 opcode 0xCB 0x3C at PC 0x1302
    ```
  - ROM bytes around the abort (failure site near
    `collision_prepare_tile`, `0x12C7`):
    ```text
    00001300: 10 82 cb 3c cb 1d cb 3c cb 1d cb 3c cb 1d 11 80
                      ^^
                      SRL H at PC=0x1302
    ```
  - Source context:
    ```asm
            ld hl, (COLLISION_WORK_INDEX)
            srl h
            rr l
            srl h
            rr l
            srl h
            rr l
    ```
  - The immediate same-path `RR L` (`0xCB 0x1D`) is expected to abort
    next because the current CB-prefix dispatch only routes `SRL A`
    (`CB 3F`) and `BIT 4..7,A` (`CB 67`/`6F`/`77`/`7F`).
- Canonical repro command:
  ```bash
  cd /home/djglxxii/src/PacManV8
  python3 tools/build.py
  python3 tools/pattern_replay_tests.py \
      --evidence-dir tests/evidence/T021-pattern-replay-and-fidelity-testing
  # Expected after this task: no unsupported-opcode abort at CB 3C
  # or the immediate CB 1D site; any later out-of-scope opcode is
  # recorded as a new blocker.
  ```

Done when:
- New CPU tests pin `SRL H` behavior and 8-T-state adapter timing,
  including a zero-result case and a carry-out case.
- New CPU tests pin `RR L` behavior and 8-T-state adapter timing,
  covering both entry-carry states, a zero result, and a sign-bit
  result.
- A sequential `collision_prepare_tile`-style test proves that
  `SRL H -> RR L` executed three times over an initial `HL` value
  yields `HL_final == HL_initial >> 3` with the expected final carry
  reported in the flags.
- A negative test confirms a nearby out-of-scope CB sub-opcode such
  as `SRL L` (`CB 3D`) or `RR H` (`CB 1C`) still raises the existing
  `Unsupported timed Z180 opcode 0xCB 0x<subop>` runtime error.
- The PacManV8 T021 repro command runs past `PC=0x1302` and the
  immediate `RR L` site without a timed-opcode abort, or records a
  later out-of-scope opcode as a new blocker without changing this
  task's scope.
- A non-perturbation regression on an existing fixture ROM that does
  not exercise the new opcodes produces byte-identical frame, audio,
  and event-log digests before and after the fix.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes with
  the new opcode tests included and no pre-existing CPU test is
  relaxed.

Out of scope:
- Any general HD64180 CB-prefix completion beyond `SRL H` and `RR L`.
- Broader SRL coverage: no `SRL B/C/D/E/L`, no `SRL (HL)`.
- Broader rotate coverage: no `RR B/C/D/E/H/A`, no `RR (HL)`, no
  `RLC`/`RRC`/`RL`/`SLA`/`SRA`/`SLL` forms.
- `RES b,r`, `SET b,r`, and `BIT b,r` for any `r` other than the
  M39-covered `A`.
- `(HL)`-target shift/rotate/bit operations.
- Index-register CB variants (`DD CB d <op>`, `FD CB d <op>`).
- Changes to interrupt acceptance, HALT-wake semantics, M35
  observability surface, M39 pair INC/DEC or CB-prefix coverage, M40
  `EX DE,HL` / `OR (HL)` coverage, VDP, audio, scheduler, frontend,
  debugger, save-state, or PacManV8 ROM code.
- Refreshing PacManV8 T021 acceptance evidence; that happens in the
  PacManV8 repo after this task is completed.

Required tests:
- Extracted-core coverage in `tests/test_cpu.cpp`:
  - `SRL H` (`0xCB 0x3C`) typical case: with `H = 0x81` and `L`
    untouched, after execution `H == 0x40`, C set, S clear, Z clear,
    H clear, P/V reflects parity of `0x40`, N clear, `PC` advanced
    by 2, adapter reports 8 T-states.
  - `SRL H` zero-result case: with `H = 0x01`, after execution
    `H == 0x00`, Z set, C set, P/V set (parity of 0), S/H/N clear.
  - `SRL H` no-carry case: with `H = 0x80`, after execution
    `H == 0x40`, C clear, Z clear, P/V reflects parity of `0x40`.
  - `RR L` typical case with carry clear on entry: with `L = 0x02`
    and carry clear, after execution `L == 0x01`, C clear (old bit 0
    was 0), S/Z/H/N clear, P/V reflects parity of `0x01`.
  - `RR L` with carry set on entry: with `L = 0x02` and carry set,
    after execution `L == 0x81`, C clear, S set (bit 7 of new `L`),
    Z clear, P/V reflects parity of `0x81`, H/N clear.
  - `RR L` with carry out: with `L = 0x01` and carry clear, after
    execution `L == 0x00`, C set, Z set, P/V set, S/H/N clear.
  - `RR L` sign-bit case: with `L = 0x00` and carry set, after
    execution `L == 0x80`, C clear, S set, Z clear, H/N clear.
  - Sequential `collision_prepare_tile`-style sequence: with
    `HL = 0x1234` and carry clear, run `SRL H -> RR L` three times
    and confirm `HL == 0x0246` (equals `0x1234 >> 3`), with the
    final carry matching the last bit shifted out (bit 2 of the
    original `HL`, which is 1 for `0x1234`), `PC` advanced by 6
    total. Include a second case with `HL = 0x0007` proving the low
    bits cascade through to leave the result zero with carry set.
  - Negative test: executing `SRL L` (`CB 3D`) or `RR H` (`CB 1C`)
    raises `std::runtime_error` whose message contains
    `Unsupported timed Z180 opcode 0xCB 0x<subop> at PC`, confirming
    the out-of-scope CB sub-opcode remains unsupported.
- Adapter-level T-state coverage confirming both `CB 3C` and `CB 1D`
  are classified as 8 T-states via `cb_instruction_tstates`.
- Non-perturbation guard against a fixture ROM that does not exercise
  the new opcodes: frame, audio, and event-log digests must be
  byte-identical before and after the fix.

Verification commands:
```bash
cd /home/djglxxii/src/PacManV8
python3 tools/build.py

cd /home/djglxxii/src/Vanguard8
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure

# Primary proof: the T021 replay validation harness now runs past
# PC=0x1302 (SRL H in collision_prepare_tile) and the immediate
# RR L site without an "Unsupported timed Z180 opcode" abort.
cd /home/djglxxii/src/PacManV8
python3 tools/pattern_replay_tests.py \
    --evidence-dir tests/evidence/T021-pattern-replay-and-fidelity-testing

# Headless smoke proof from the Vanguard 8 side.
cd /home/djglxxii/src/Vanguard8
cmake-build-debug/src/vanguard8_headless \
    --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
    --frames 60 --hash-frame 60
```

## Incompletion Summary

Date: 2026-04-23

Implemented against:
- `docs/spec/01-cpu.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/41.md`
- `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`

Completed within M41 scope:
- Added timed extracted-core support for `SRL H` (`0xCB 0x3C`) in
  `third_party/z180/z180_core.cpp`: shifts `H` right by one, bit 7
  of new `H` is cleared, old `H` bit 0 becomes carry; flags follow
  the Z80 `SRL` contract (S cleared, Z from result, H cleared, P/V
  parity, N cleared). `PC` advances by 2 via normal CB-prefix fetch.
- Added timed extracted-core support for `RR L` (`0xCB 0x1D`) in
  `third_party/z180/z180_core.cpp`: rotates `L` right through the
  carry flag — new bit 7 is the old carry, new carry is old `L`
  bit 0; S from new bit 7, Z from result, H cleared, P/V parity, N
  cleared.
- Extended `Core::op_cb_prefix` to dispatch `0x1D` and `0x3C`
  alongside the existing M39 `SRL A` / `BIT 4..7,A` entries while
  preserving the fail-fast `std::runtime_error` contract for every
  still-unsupported CB sub-opcode.
- Added adapter timing in `src/core/cpu/z180_adapter.cpp`:
  `cb_instruction_tstates` now returns **8 T-states** for `0x1D`
  and `0x3C`.
- Added focused `tests/test_cpu.cpp` coverage for both M41 opcodes,
  including `SRL H` zero-result / carry-out / no-carry cases, `RR L`
  cases for each entry-carry state, zero result, and sign-bit
  injection, the `collision_prepare_tile` divide-by-eight sequence
  (`HL = 0x1234 -> 0x0246` with final carry set, and
  `HL = 0x0007 -> 0x0000` with carry set and Z set), and negative
  guards proving `SRL L` (`CB 3D`) and `RR H` (`CB 1C`) still raise
  the unsupported-opcode runtime error pattern.

Verification completed:
- `cd /home/djglxxii/src/PacManV8 && python3 tools/build.py` passed.
- `cd /home/djglxxii/src/Vanguard8 && cmake --build cmake-build-debug`
  passed.
- `cmake-build-debug/tests/vanguard8_tests "[cpu]"` passed:
  67 test cases, 815 assertions.
- `ctest --test-dir cmake-build-debug --output-on-failure` passed:
  190/190 tests passed; existing showcase milestone 7 test skipped.
- Non-perturbation fixture replay:
  `cmake-build-debug/src/vanguard8_headless --rom tests/replays/replay_fixture.rom --replay tests/replays/replay_fixture.v8r --frames 60 --hash-frame 60 --hash-audio`
  produced event-log digest `6563162820683566367`, frame 60 SHA-256
  `e46b5246bda293e09e199967b99ac352f931c04e2ad88e775b06a3b93ccb838c`,
  and audio SHA-256
  `734c69bba0fceee9199a4d44febcae8bdde585874f1f7cb260d80b6ea7683d68`
  — byte-identical to the M40 completion summary.
- M41 headless smoke proof passed:
  `cmake-build-debug/src/vanguard8_headless --rom /home/djglxxii/src/PacManV8/build/pacman.rom --frames 60 --hash-frame 60`
  produced event-log digest `6563162820683566367` and frame 60
  SHA-256 `4a63cec305375edd4b20e85ba9830d83888e2eaf4327a29c229cfc7ce7a79693`.

Blocker:
- The primary PacManV8 T021 replay validation command now runs past
  the previous `PC=0x1302` `SRL H` (`0xCB 0x3C`) failure and the
  immediate `RR L` (`0xCB 0x1D`) sites, then reaches a new
  out-of-scope timed opcode:

  ```text
  terminate called after throwing an instance of 'std::runtime_error'
    what():  Unsupported timed Z180 opcode 0x37 at PC 0x1315
  ```

- `0x37` is `SCF` (Set Carry Flag) — outside the M41 allowed scope,
  which permits only `SRL H` and `RR L`.

Resolution needed:
- Define a follow-up milestone/task for the newly exposed timed
  opcode `SCF` (`0x37`) before implementing it or adjacent opcodes.
  Do not broaden M41 to cover this opcode.


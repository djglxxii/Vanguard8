# M40-T01 — Timed HD64180 `EX DE,HL` and `OR (HL)` Opcode Coverage for PacManV8 T021

Status: `blocked`
Milestone: `40`
Depends on: `M39-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/40.md`
- `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`

Scope:
- Implement timed execution for exactly two missing opcodes in the
  extracted HD64180 runtime, as authorized by milestone 40:
  - `EX DE,HL` (`0xEB`) — the confirmed blocker at PacManV8
    `PC 0x11DA` in `collision_init`.
  - `OR (HL)` (`0xB6`) — the immediate same-path look-ahead opcode
    after the confirmed `EX DE,HL` site.
- `EX DE,HL` must:
  - Swap the full 16-bit `DE` and `HL` register pairs.
  - Advance `PC` by 1.
  - Leave all flags untouched.
  - Report **4 T-states** at the adapter boundary.
- `OR (HL)` must:
  - Read the operand byte from logical address `HL`.
  - Compute `A <- A | (HL)`.
  - Set S from result bit 7, Z from zero result, P/V from even
    parity; clear H, N, and C.
  - Advance `PC` by 1.
  - Report **7 T-states** at the adapter boundary.
- Preserve the existing fail-fast contract for all still-unsupported
  opcodes.

Blocker reference:
- PacManV8 blocked task:
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`
- Key evidence from that blocker:
  - After M39, the canonical replay harness passes the previous
    `DEC BC` / pair INC/DEC / CB-prefix gaps and aborts with:
    ```text
    terminate called after throwing an instance of 'std::runtime_error'
      what():  Unsupported timed Z180 opcode 0xEB at PC 0x11DA
    ```
  - Failure site is `collision_init` (begins at `0x1198`); ROM bytes
    around the abort:
    ```text
    000011d0: fe 02 28 06 fe 03 28 14 18 22 eb 3a 12 82 b6 77
                                              ^^
                                              EX DE,HL at PC=0x11DA
    ```
  - Source context:
    ```asm
    .set_pellet:
            ex de, hl
            ld a, (COLLISION_WORK_MASK)
            or (hl)
            ld (hl), a
            ex de, hl
    ```
  - The immediate same-path `OR (HL)` (`0xB6`) is likely to abort
    next because the timed dispatch table covers register `OR` forms
    `0xB0..0xB5` and `0xB7`, but not `0xB6`.
- Canonical repro command:
  ```bash
  cd /home/djglxxii/src/PacManV8
  python3 tools/build.py
  python3 tools/pattern_replay_tests.py \
      --evidence-dir tests/evidence/T021-pattern-replay-and-fidelity-testing
  # Expected after this task: no unsupported-opcode abort at 0xEB
  # or the immediate 0xB6 site; any later out-of-scope opcode is
  # recorded as a new blocker.
  ```

Done when:
- New CPU tests pin `EX DE,HL` behavior and 4-T-state adapter timing.
- New CPU tests pin `OR (HL)` behavior and 7-T-state adapter timing,
  including zero-result and sign/parity cases.
- A sequential `collision_init`-style test proves `EX DE,HL ->
  OR (HL) -> LD (HL),A -> EX DE,HL` uses the swapped pointer and
  restores register-pair roles.
- A negative test confirms a nearby out-of-scope exchange opcode such
  as `EX (SP),HL` (`0xE3`) still raises the existing
  `Unsupported timed Z180 opcode` runtime error.
- The PacManV8 T021 repro command runs past `PC=0x11DA` and the
  immediate `OR (HL)` site without a timed-opcode abort, or records a
  later out-of-scope opcode as a new blocker without changing this
  task's scope.
- A non-perturbation regression on an existing fixture ROM that does
  not exercise the new opcodes produces byte-identical frame, audio,
  and event-log digests before and after the fix.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes with
  the new opcode tests included and no pre-existing CPU test is
  relaxed.

Out of scope:
- Any general HD64180 opcode completion beyond `EX DE,HL` and
  `OR (HL)`.
- Broader exchange-family work: no `EX AF,AF'`, no `EXX`, no
  `EX (SP),HL`, and no index-register exchange forms.
- Broader logical/arithmetic work beyond the single `(HL)` OR form.
- Changes to interrupt acceptance, HALT-wake semantics, M35
  observability surface, M39 pair INC/DEC or CB-prefix coverage, VDP,
  audio, scheduler, frontend, debugger, save-state, or PacManV8 ROM
  code.
- Refreshing PacManV8 T021 acceptance evidence; that happens in the
  PacManV8 repo after this task is completed.

Required tests:
- Extracted-core coverage in `tests/test_cpu.cpp`:
  - `EX DE,HL` (`0xEB`): with `DE = 0x1234` and `HL = 0xABCD`,
    after execution `DE == 0xABCD`, `HL == 0x1234`, `PC` advanced
    by 1, flags untouched, adapter reports 4 T-states.
  - `OR (HL)` (`0xB6`) typical case: with `A = 0x42`, `HL`
    pointing to `0x24`, after execution `A == 0x66`, S/Z/H/N/C
    clear, P/V set for even parity, `PC` advanced by 1, adapter
    reports 7 T-states.
  - `OR (HL)` zero-result case: with `A = 0x00` and `(HL) = 0x00`,
    Z and P/V set, S/H/N/C clear.
  - `OR (HL)` sign/parity case: with a result that sets bit 7 and
    has odd parity, S set and P/V clear.
  - Sequential `collision_init`-style sequence:
    `EX DE,HL -> LD A,(mask_addr) -> OR (HL) -> LD (HL),A ->
    EX DE,HL`, proving the byte at the swapped-in `HL` address is
    updated and `DE`/`HL` are restored by the final exchange.
  - Negative test: executing `EX (SP),HL` (`0xE3`) raises
    `std::runtime_error` whose message contains
    `Unsupported timed Z180 opcode 0xE3 at PC`, confirming the
    out-of-scope exchange-family opcode remains unsupported.
- Adapter-level T-state coverage confirming `0xEB` is 4 T-states and
  `0xB6` is 7 T-states.
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
# PC=0x11DA (EX DE,HL in collision_init) and the immediate OR (HL)
# site without an "Unsupported timed Z180 opcode" abort.
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

Date: 2026-04-22

Implemented against:
- `docs/spec/01-cpu.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/40.md`
- `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`

Completed within M40 scope:
- Added timed extracted-core support for `EX DE,HL` (`0xEB`) in
  `third_party/z180/`: swaps full 16-bit `DE` and `HL`, advances `PC`
  by 1 through normal fetch, and leaves flags untouched.
- Added timed extracted-core support for `OR (HL)` (`0xB6`) in
  `third_party/z180/`: reads the byte at logical `HL`, updates
  `A <- A | (HL)`, sets S/Z/PV from the result, and clears H/N/C.
- Added adapter timing in `src/core/cpu/z180_adapter.cpp`:
  `0xEB` reports 4 T-states and `0xB6` reports 7 T-states.
- Added focused `tests/test_cpu.cpp` coverage for both M40 opcodes,
  including flag behavior, PC advance, adapter T-state reporting,
  a `collision_init`-style `EX DE,HL -> LD A,(nn) -> OR (HL) ->
  LD (HL),A -> EX DE,HL` sequence, and a negative guard proving
  `EX (SP),HL` (`0xE3`) remains unsupported.

Verification completed:
- `cd /home/djglxxii/src/PacManV8 && python3 tools/build.py` passed.
- `cd /home/djglxxii/src/Vanguard8 && cmake --build cmake-build-debug`
  passed.
- `cmake-build-debug/tests/vanguard8_tests "[cpu]"` passed:
  63 test cases, 758 assertions.
- `ctest --test-dir cmake-build-debug --output-on-failure` passed:
  186/186 tests passed; existing showcase milestone 7 test skipped.
- Non-perturbation fixture replay:
  `cmake-build-debug/src/vanguard8_headless --rom tests/replays/replay_fixture.rom --replay tests/replays/replay_fixture.v8r --frames 60 --hash-frame 60 --hash-audio`
  produced event-log digest `6563162820683566367`, frame 60 SHA-256
  `e46b5246bda293e09e199967b99ac352f931c04e2ad88e775b06a3b93ccb838c`,
  and audio SHA-256
  `734c69bba0fceee9199a4d44febcae8bdde585874f1f7cb260d80b6ea7683d68`.
- M40 headless smoke proof passed:
  `cmake-build-debug/src/vanguard8_headless --rom /home/djglxxii/src/PacManV8/build/pacman.rom --frames 60 --hash-frame 60`
  produced event-log digest `6563162820683566367` and frame 60
  SHA-256 `4a63cec305375edd4b20e85ba9830d83888e2eaf4327a29c229cfc7ce7a79693`.

Blocker:
- The primary PacManV8 T021 replay validation command now runs past
  the previous `PC=0x11DA` `EX DE,HL` (`0xEB`) failure and the
  immediate `OR (HL)` (`0xB6`) site, then reaches a new out-of-scope
  timed opcode:

  ```text
  terminate called after throwing an instance of 'std::runtime_error'
    what():  Unsupported timed Z180 opcode 0xCB 0x3C at PC 0x1302
  ```

- ROM bytes around the new blocker:

  ```text
  000012f0: fd 32 12 82 2a 10 82 11 71 04 19 7e 32 13 82 2a
  00001300: 10 82 cb 3c cb 1d cb 3c cb 1d cb 3c cb 1d 11 80
                    ^^
                    CB 3C at PC=0x1302
  ```

- `build/pacman.sym` maps the region to `collision_prepare_tile`
  (`0x12C7`) before `collision_check_all_ghosts` (`0x1319`).
  `CB 3C` is `SRL H`, and it is outside the M40 allowed scope, which
  permits only `EX DE,HL` and `OR (HL)`.

Resolution needed:
- Define a follow-up milestone/task for the newly exposed timed
  CB-prefix surface before implementing `CB 3C` or adjacent opcodes.
  Do not broaden M40 to cover this opcode.

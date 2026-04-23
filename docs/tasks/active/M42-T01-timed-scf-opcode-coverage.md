# M42-T01 — Timed HD64180 `SCF` Opcode Coverage for PacManV8 T021

Status: `active`
Milestone: `42`
Depends on: `M41-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/42.md`
- `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`

Scope:
- Implement timed execution for exactly one missing opcode in the
  extracted HD64180 runtime, as authorized by milestone 42:
  - `SCF` (`0x37`) — the confirmed blocker at PacManV8 `PC 0x1315`
    immediately after the `collision_prepare_tile` divide-by-eight
    sequence, used as a "return with carry set" idiom
    (`SCF` followed by `RET`).
- `SCF` must:
  - Set the carry flag (C = 1).
  - Clear the half-carry flag (H = 0) and the add/subtract flag
    (N = 0).
  - Leave S, Z, and P/V untouched.
  - Leave `A` and every other register untouched.
  - Advance `PC` by 1 through normal fetch.
  - Report **4 T-states** at the adapter boundary.
- Preserve the existing fail-fast contract for all still-unsupported
  opcodes.

Blocker reference:
- PacManV8 blocked task:
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`
- Key evidence from the M41 incompletion summary:
  - After M41, the canonical replay harness passes the previous
    `SRL H` / `RR L` CB-prefix sites and aborts with:
    ```text
    terminate called after throwing an instance of 'std::runtime_error'
      what():  Unsupported timed Z180 opcode 0x37 at PC 0x1315
    ```
  - ROM bytes around the abort:
    ```text
    00001310: 81 19 3a 13 82 37 c9 b7 c9 21 20 81 cd 38 13 b7
                             ^^
                             SCF at PC=0x1315
    ```
  - The immediate downstream path (`RET` at `0x1316`, `OR A` at
    `0x1317`, `RET` at `0x1318`, `LD HL,nn` at `0x1319`, `CALL nn`
    at `0x131C`, `OR A` at `0x131F`, `RET NZ` at `0x1320`, ...) is
    already fully covered by the existing timed dispatch, so no
    look-ahead opcode is bundled into this task.
- Canonical repro command:
  ```bash
  cd /home/djglxxii/src/PacManV8
  python3 tools/build.py
  python3 tools/pattern_replay_tests.py \
      --evidence-dir tests/evidence/T021-pattern-replay-and-fidelity-testing
  # Expected after this task: no unsupported-opcode abort at 0x37;
  # any later out-of-scope opcode is recorded as a new blocker.
  ```

Done when:
- New CPU tests pin `SCF` behavior and 4-T-state adapter timing,
  including both entry-carry states and explicit S/Z/P/V
  preservation coverage.
- A sequential `SCF -> RET` test proves the idiom returns to a
  stacked address with carry set and other preserved flags intact.
- A negative test confirms a nearby out-of-scope opcode such as
  `CCF` (`0x3F`) still raises the existing
  `Unsupported timed Z180 opcode 0x3F at PC` runtime error.
- The PacManV8 T021 repro command runs past `PC=0x1315` without a
  timed-opcode abort, or records a later out-of-scope opcode as a
  new blocker without changing this task's scope.
- A non-perturbation regression on an existing fixture ROM that
  does not exercise the new opcode produces byte-identical frame,
  audio, and event-log digests before and after the fix.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes
  with the new opcode tests included and no pre-existing CPU test
  is relaxed.

Out of scope:
- Any general HD64180 opcode completion beyond `SCF`.
- Broader flag-manipulation coverage: no `CCF`, no `DAA`, no `CPL`.
- Broader arithmetic/logical work.
- Changes to interrupt acceptance, HALT-wake semantics, M35
  observability surface, M39–M41 handlers, VDP, audio, scheduler,
  frontend, debugger, save-state, or PacManV8 ROM code.
- Refreshing PacManV8 T021 acceptance evidence; that happens in the
  PacManV8 repo after this task is completed.

Required tests:
- Extracted-core coverage in `tests/test_cpu.cpp`:
  - `SCF` (`0x37`) entry C=0/H=1/N=1 case: with initial flags
    `flag_sign | flag_zero | flag_half | flag_parity_overflow |
    flag_subtract` and `A = 0x5A`, after execution flags equal
    `flag_sign | flag_zero | flag_parity_overflow | flag_carry`
    (S/Z/PV preserved, H/N cleared, C set), `A == 0x5A`, `PC`
    advanced by 1, adapter reports 4 T-states.
  - `SCF` entry C=1/H=0/N=0 case: with initial flags
    `flag_carry` only and `A = 0x00`, after execution flags equal
    `flag_carry` (C stays set, H/N still clear, S/Z/PV all still
    zero), `A == 0x00`, `PC` advanced by 1.
  - Register-preservation case: populate every register pair
    (`AF`/`BC`/`DE`/`HL`/`SP`/`IX`/`IY` and the shadow
    `AF'`/`BC'`/`DE'`/`HL'`) with distinct non-trivial values,
    execute `SCF`, and confirm only `F` changes in the documented
    way.
  - Sequential `SCF -> RET` idiom: place a return address at a
    known stack location, execute `SCF` then `RET`, and confirm
    `PC` equals the return target, `SP` advanced by 2, and carry
    is set with S/Z/P/V preserved from the SCF-entry state.
  - Negative test: executing `CCF` (`0x3F`) raises
    `std::runtime_error` whose message contains
    `Unsupported timed Z180 opcode 0x3F at PC`, confirming the
    out-of-scope sister opcode remains unsupported.
- Adapter-level T-state coverage confirming `0x37` is 4 T-states.
- Non-perturbation guard against a fixture ROM that does not
  exercise the new opcode: frame, audio, and event-log digests must
  be byte-identical before and after the fix.

Verification commands:
```bash
cd /home/djglxxii/src/PacManV8
python3 tools/build.py

cd /home/djglxxii/src/Vanguard8
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure

# Primary proof: the T021 replay validation harness now runs past
# PC=0x1315 (SCF) without an "Unsupported timed Z180 opcode" abort.
cd /home/djglxxii/src/PacManV8
python3 tools/pattern_replay_tests.py \
    --evidence-dir tests/evidence/T021-pattern-replay-and-fidelity-testing

# Headless smoke proof from the Vanguard 8 side.
cd /home/djglxxii/src/Vanguard8
cmake-build-debug/src/vanguard8_headless \
    --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
    --frames 60 --hash-frame 60
```

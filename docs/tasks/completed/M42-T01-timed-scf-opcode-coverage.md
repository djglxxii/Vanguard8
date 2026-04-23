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

## Completion summary

Completed on 2026-04-23.

Implemented against `docs/spec/01-cpu.md`,
`docs/emulator/02-emulation-loop.md`,
`docs/emulator/03-cpu-and-bus.md`,
`docs/emulator/07-implementation-plan.md`,
`docs/emulator/milestones/42.md`, and
`/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`.

Fix:
- Added one timed-core opcode handler in
  `third_party/z180/z180_core.cpp`:
  - `Core::op_scf()` — preserves the S/Z/P/V bits of F, clears
    H/N, and sets C. Implemented as
    `af_.bytes.lo = (af_.bytes.lo & (flag_sign | flag_zero |
    flag_parity_overflow)) | flag_carry;`. `A` and every other
    register are untouched; `PC` advances through the standard
    one-byte fetch.
- Declared the handler in `third_party/z180/z180_core.hpp`
  adjacent to the existing `op_ex_de_hl` declaration.
- Registered the opcode in the dispatch table next to the existing
  `JR cc,e` entries:
  - `opcodes_[0x37] = &Core::op_scf;`
- Added `case 0x37:` to the adapter's 4-T-state `current_instruction_tstates()` case list in
  `src/core/cpu/z180_adapter.cpp`, alongside the other
  4-T-state opcodes (`0xEB`, `0xAF`, ...).
- No flag semantics were added for `CCF`, `DAA`, `CPL`, or any
  other flag-manipulation opcode. No changes to interrupt
  acceptance, HALT-wake semantics, M35 observability surface,
  M37–M41 handlers, VDP, audio, scheduler, frontend, debugger,
  save-state, or PacManV8 ROM code.

Tests added to `tests/test_cpu.cpp`:
- `scheduled CPU covers SCF used by PacManV8 T021
  collision_prepare_tile (M42)`, containing five sections:
  - Entry `C=0, H=1, N=1` with `S/Z/PV` all set and `A=0x5A`:
    after execution `F = S|Z|PV|C`, `A=0x5A`, `PC` advances by 1,
    adapter reports 4 T-states.
  - Entry `C=1, H=0, N=0` with `S/Z/PV` all clear and `A=0x00`:
    after execution `F = C`, `A=0x00`, `PC` advances by 1,
    adapter reports 4 T-states.
  - Register-preservation case: populates every register pair
    (`AF`, `BC`, `DE`, `HL`, `SP`, `IX`, `IY`, `AF'`, `BC'`,
    `DE'`, `HL'`) with distinct non-trivial values; after `SCF`
    only `F` changes (the documented way) while `A` and every
    other register pair retain their pre-execution values.
  - Sequential `SCF -> RET` idiom: stacks a return target (`0x0234`)
    at the SRAM stack, executes `SCF` then `RET`; `PC` equals
    `0x0234`, `SP` advanced by 2, `F = S|Z|PV|C` (S/Z/PV
    preserved from SCF entry, carry set).
  - Negative guard: `CCF` (`0x3F`) still raises
    `std::runtime_error` whose message contains
    `Unsupported timed Z180 opcode 0x3F at PC`.

Non-perturbation:
- Full `ctest --test-dir cmake-build-debug --output-on-failure`:
  `190 / 191` pass with `1` skipped (`showcase milestone 7 late
  loop leaves VDP-A in Graphic 6 and produces a 512 wide frame`),
  same skip set as before. No pre-existing test was modified or
  relaxed.
- `vanguard8_headless_replay_regression` ctest entry passes with
  the pinned frame hash
  `e46b5246bda293e09e199967b99ac352f931c04e2ad88e775b06a3b93ccb838c`
  and audio hash
  `48beda9f68f15ac4e3fca3f8b54ebea7832a906d358cc49867ae508287039ddf`
  unchanged, confirming byte-identical frame, audio, and event-log
  digests on a fixture ROM that does not exercise SCF.

Runtime regression against PacManV8 T021:
- Rebuilt PacManV8 ROM (`build/pacman.rom`) on 2026-04-23 with
  no PacManV8-side edits.
- `python3 tools/pattern_replay_tests.py
  --evidence-dir tests/evidence/T021-pattern-replay-and-fidelity-testing`
  no longer aborts with `Unsupported timed Z180 opcode 0x37 at
  PC 0x1315`. The headless binary now runs to the requested
  `--frames 60` checkpoint and produces a valid inspection
  report (end-of-frame state `PC=0x0067`, `halted=true`,
  `IFF1=true`, `IFF2=true` — the expected idle resting point).
  Representative frame evidence from the canonical headless
  smoke command:
  - Frame SHA-256 (60):
    `4a63cec305375edd4b20e85ba9830d83888e2eaf4327a29c229cfc7ce7a79693`
  - Event log digest: `6563162820683566367`
  - Event log size: `12840`
  - Master cycles: `14305200`; CPU T-states: `7152600`.
- The PacManV8 `pattern_replay_tests.py` harness now fails with
  `pattern_replay_tests.py error: inspection report did not
  contain logical 0x8270:13` — this is **not** a new unsupported
  timed opcode, and the inspection report is in fact being
  generated correctly. The mismatch is between the PacManV8
  harness regex `BYTE_ROW_PATTERN = r"^\\s+0x([0-9a-f]{4}):..."`
  (expects 4-digit hex row prefixes) and Vanguard 8's
  `src/frontend/headless_inspect.cpp::append_byte_row`, which
  formats logical-peek row display addresses using `hex20`
  (5-digit), emitting `  0x08270:` instead of `  0x8270:`. That
  row-format mismatch sits outside M42's allowed paths
  (`src/frontend/` is not in scope); it becomes the next T021
  blocker to route through a follow-up milestone — most
  naturally a narrow inspection-report formatting fix in
  Vanguard 8, but the PacManV8 harness side is also viable.

Files changed:
- `third_party/z180/z180_core.hpp`
- `third_party/z180/z180_core.cpp`
- `src/core/cpu/z180_adapter.cpp`
- `tests/test_cpu.cpp`
- `docs/emulator/current-milestone.md`
- `docs/tasks/active/M42-T01-timed-scf-opcode-coverage.md`
  (moved to `docs/tasks/completed/`)

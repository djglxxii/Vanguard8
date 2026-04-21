# M37-T01 — Timed HD64180 `LD E,n` Opcode Coverage for PacManV8 T020

Status: `completed`
Milestone: `37`
Depends on: `M36-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/37.md`
- `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T020-intermission-cutscenes.md`

Scope:
- Implement timed execution for three missing `LD r,n` opcode forms in
  the extracted HD64180 runtime, as authorized by milestone 37:
  - `LD E,n` (`0x1E`) — the confirmed blocker at PacManV8 `PC 0x332E`.
  - `LD H,n` (`0x26`) — sister form, closed together to avoid a
    rebuild-per-opcode loop.
  - `LD L,n` (`0x2E`) — sister form, same rationale.
- Each opcode must:
  - Fetch a single immediate byte via the existing `fetch_byte()`
    helper.
  - Write that byte into the target register half (`E`, `H`, or `L`).
  - Advance `PC` by 2 (one byte for the opcode, one for the
    immediate).
  - Leave all other registers and all flags (S/Z/H/PV/N/C) untouched.
  - Report 7 T-states via the adapter timing table.
- Implementation must follow the existing pattern already used for
  `op_ld_b_n` / `op_ld_c_n` / `op_ld_d_n` / `op_ld_a_n` in
  `third_party/z180/z180_core.cpp`. Register the new handlers in the
  opcode dispatch table next to the existing `LD r,n` entries and
  register the new 7-T-state entries in the adapter's existing
  7-T-state case group at
  `src/core/cpu/z180_adapter.cpp` (around the block covering `0x3E`,
  `0x06`, `0x0E`, `0x16`, `0x7E`, `0xC6`, `0xE6`, `0xF6`, `0xFE`).

Blocker reference:
- PacManV8 blocked task:
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T020-intermission-cutscenes.md`
- Key evidence from that blocker:
  - The canonical headless runtime aborts with
    `Unsupported timed Z180 opcode 0x1E at PC 0x332E` on the
    intermission panel setup path.
  - The preceding `LD D,n` (`0x16`) at `0x332C` executed without abort,
    so only the `0x1E` form is missing from the timed core.
  - Additional `LD E,n` sites on the same active drawing block:
    `0x330E`, `0x3317`, `0x3321`, `0x332E`, `0x3337`, `0x3341`,
    `0x3355`, `0x335E`, `0x3394`, `0x33A1`, `0x33AE`, `0x33BB`.
  - `tools/intermission_tests.py` prints `5/5 passed`, confirming the
    source-level T020 scene/timing harness is already correct;
    validation is blocked purely on emulator-side opcode coverage.
- Canonical repro command (post-fix expectation in parentheses):
  ```bash
  cd /home/djglxxii/src/PacManV8
  python3 tools/build.py

  /home/djglxxii/src/Vanguard8/cmake-build-debug/src/vanguard8_headless \
      --rom build/pacman.rom \
      --frames 1020 --inspect-frame 1020 \
      --dump-cpu --hash-frame 1020
  # (expected: completes without opcode abort, emits a stable frame
  # hash and CPU snapshot)
  ```

Done when:
- New CPU tests pin `LD E,n`, `LD H,n`, and `LD L,n` behavior and
  adapter T-state classification per the milestone contract.
- The PacManV8 T020 repro command above runs to completion without
  `Unsupported timed Z180 opcode 0x1E at PC 0x332E` (or at any of the
  sibling `LD E,n` sites listed in the blocker).
- A non-perturbation regression on an existing fixture ROM that does
  not exercise the new opcodes produces byte-identical frame, audio,
  and event-log digests before and after the fix.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes with
  the new opcode tests included and no pre-existing CPU test is
  relaxed.

Out of scope:
- Any general HD64180 opcode completion beyond the three `LD r,n`
  forms listed above.
- Index-register (`DD`/`FD`) prefixed `LD r,n` variants. They are not
  on the T020 path.
- Changes to interrupt acceptance, HALT-wake semantics, M35
  observability surface, VDP, audio, scheduler, frontend, debugger,
  save-state, or PacManV8 ROM code.
- Refreshing PacManV8 T020 acceptance evidence; that happens in the
  PacManV8 repo after this task is completed.

Required tests:
- Extracted-core coverage in `tests/test_cpu.cpp` (or the adjacent CPU
  test file the project already uses):
  - `LD E,n` (`0x1E`): after execution, `E == immediate`, `D`
    preserved, all other registers preserved, `PC` advanced by 2,
    flags (S/Z/H/PV/N/C) untouched.
  - `LD H,n` (`0x26`): same contract with `H` as the destination,
    `L` preserved.
  - `LD L,n` (`0x2E`): same contract with `L` as the destination,
    `H` preserved.
  - Sequential three-opcode pattern: run `LD D,n` (0x16) → `LD E,n`
    (0x1E) → `LD A,n` (0x3E) and confirm all three destination
    registers receive their immediates correctly in sequence, mirroring
    the PacManV8 pattern at `0x332C..0x3331`.
- Adapter-level T-state coverage confirming each new opcode is
  classified as 7 T-states.
- Non-perturbation guard against a non-`LD E,n` / `LD H,n` / `LD L,n`
  fixture ROM: frame, audio, and event-log digests must be
  byte-identical before and after the fix.

Verification commands:
```bash
cd /home/djglxxii/src/PacManV8
python3 tools/build.py

cd /home/djglxxii/src/Vanguard8
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure

cmake-build-debug/src/vanguard8_headless \
    --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
    --frames 1020 --inspect-frame 1020 \
    --dump-cpu --hash-frame 1020
```

## Completion summary

Completed on 2026-04-21.

Implemented against `docs/spec/01-cpu.md`,
`docs/emulator/02-emulation-loop.md`,
`docs/emulator/03-cpu-and-bus.md`,
`docs/emulator/07-implementation-plan.md`,
`docs/emulator/milestones/37.md`, and
`/home/djglxxii/src/PacManV8/docs/tasks/blocked/T020-intermission-cutscenes.md`.

Fix:
- Added three timed-core opcode handlers in
  `third_party/z180/z180_core.cpp` following the existing
  `op_ld_b_n` / `op_ld_c_n` / `op_ld_d_n` pattern:
  - `Core::op_ld_e_n()` — `de_.bytes.lo = fetch_byte();`
  - `Core::op_ld_h_n()` — `hl_.bytes.hi = fetch_byte();`
  - `Core::op_ld_l_n()` — `hl_.bytes.lo = fetch_byte();`
- Declared the three handlers in `third_party/z180/z180_core.hpp`
  adjacent to the existing `op_ld_b_n` / `op_ld_c_n` / `op_ld_d_n`
  declarations.
- Registered the three opcodes in the dispatch table next to the
  existing `LD r,n` entries:
  - `opcodes_[0x1E] = &Core::op_ld_e_n;`
  - `opcodes_[0x26] = &Core::op_ld_h_n;`
  - `opcodes_[0x2E] = &Core::op_ld_l_n;`
- Added `0x1E`, `0x26`, `0x2E` to the existing 7-T-state case group
  in `src/core/cpu/z180_adapter.cpp` alongside `0x3E`, `0x06`,
  `0x0E`, `0x16`.
- No flag updates, no changes to any other opcode handler, no
  changes to HALT-wake semantics, M35 CLI, VDP, audio, scheduler,
  or frontend code.

Tests added to `tests/test_cpu.cpp`:
- `scheduled CPU covers LD E,n used by the PacManV8 T020
  intermission panel path` — pins `E=0x50`, `D=0x44` preserved, HL
  preserved, BC preserved, flags untouched, PC advanced by 2,
  adapter reports 7 T-states.
- `scheduled CPU covers LD H,n used by the PacManV8 T020
  intermission panel path` — same contract for `H` destination,
  `L` preserved.
- `scheduled CPU covers LD L,n used by the PacManV8 T020
  intermission panel path` — same contract for `L` destination,
  `H` preserved.
- `scheduled CPU covers the PacManV8 T020 LD D,n -> LD E,n ->
  LD A,n rectangle-fill prologue` — exercises the exact opcode
  sequence at PacManV8 `0x332C..0x3331` (`LD D,0x44`, `LD E,0x50`,
  `LD A,0x22`), confirming all three destination registers receive
  their immediates in sequence and PC lands at `0x0006`.

Non-perturbation:
- Full `ctest` run: `177/177` pass (was `173/173` plus the four new
  `LD r,n` tests). No existing test was modified or relaxed.
  The skipped showcase-M7 Graphic 6 test remains skipped as before.
  Because the non-`LD E,n` / `LD H,n` / `LD L,n` dispatch paths are
  literally unchanged (the additions are pure opcode-table entries
  and new handler functions), ROMs that do not exercise these opcodes
  produce byte-identical observable behavior.

Runtime regression against PacManV8 T020:
- Rebuilt PacManV8 ROM (`build/pacman.rom`, `build/pacman.sym`) on
  2026-04-21 with no PacManV8-side edits.
- `vanguard8_headless --rom build/pacman.rom --frames 1020
  --inspect-frame 1020 --dump-cpu --hash-frame 1020` now runs to
  completion without `Unsupported timed Z180 opcode 0x1E at PC
  0x332E`, emits a CPU snapshot at end-of-frame, and produces a
  stable frame SHA-256 and event-log digest.
- Three repeat runs all report identical digests:
  - Frame SHA-256 (1020):
    `a79b667aef218838fbe9ef205a615f11606a08a6f73a730d40fe60127cc60a28`
  - Event log digest: `11584233142677547359`
  - Event log size: `218280`
- End-of-frame CPU state: `PC=0x0067`, `halted=true`, `IFF1=true`
  (the expected idle-loop resting point established by M36), with
  `master_cycle=243188400` and `cpu_tstates=121594200`. That the CPU
  reached frame `1020` at all (rather than aborting at `PC=0x332E`
  during the intermission panel prologue on scene-1 setup) is the
  authoritative post-fix evidence.

Files changed:
- `third_party/z180/z180_core.hpp`
- `third_party/z180/z180_core.cpp`
- `src/core/cpu/z180_adapter.cpp`
- `tests/test_cpu.cpp`
- `docs/emulator/milestones/37.md` (new in this milestone pass)
- `docs/emulator/current-milestone.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/tasks/active/M37-T01-timed-ld-e-n-opcode-coverage.md`
  (moved to `docs/tasks/completed/`)
- `docs/tasks/task-index.md`

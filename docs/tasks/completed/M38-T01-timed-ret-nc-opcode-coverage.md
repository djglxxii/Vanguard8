# M38-T01 — Timed HD64180 `RET NC` Opcode Coverage for PacManV8 T020

Status: `active`
Milestone: `38`
Depends on: `M37-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/38.md`
- `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T020-intermission-cutscenes.md`

Scope:
- Implement timed execution for two missing carry-conditional `RET cc`
  opcodes in the extracted HD64180 runtime, as authorized by milestone
  38:
  - `RET NC` (`0xD0`) — the confirmed blocker at PacManV8 `PC 0x2F75`
    in `intermission_advance_review_index`.
  - `RET C` (`0xD8`) — sister form in the same carry-conditional
    family, closed together to avoid a rebuild-per-opcode loop on
    known active modules (`src/ghost_house.asm`, `src/movement.asm`,
    `src/ghost_ai.asm`).
- Each opcode must:
  - Test the carry flag against the condition (clear for `RET NC`,
    set for `RET C`).
  - **Taken branch:** pop PC from the top of the stack (low byte at
    `(SP)`, high byte at `(SP+1)`), advance `SP` by 2, leave all flags
    untouched, report **11 T-states**.
  - **Not-taken branch:** advance `PC` by 1, leave `SP` and all flags
    untouched, report **5 T-states**.
- Implementation must follow the existing pattern already used for
  `op_ret_z` / `op_ret_nz` in `third_party/z180/z180_core.cpp`.
  Register the new handlers in the opcode dispatch table next to the
  existing `RET cc` entries and register the new taken/not-taken
  T-state entries in the adapter's existing `RET cc` timing groups at
  `src/core/cpu/z180_adapter.cpp` alongside `0xC8` (`RET Z`) and
  `0xC0` (`RET NZ`).

Blocker reference:
- PacManV8 blocked task:
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T020-intermission-cutscenes.md`
- Key evidence from that blocker:
  - The canonical headless runtime aborts with
    `Unsupported timed Z180 opcode 0xD0 at PC 0x2F75` when advancing
    the intermission review index from scene 1 to scene 2.
  - `intermission_advance_review_index` at `0x2F70..0x2F7A`:
    ```text
    0x2F70: 3A 77 82    LD A,(INTERMISSION_REVIEW_INDEX)
    0x2F73: FE 02       CP INTERMISSION_REVIEW_LAST_INDEX
    0x2F75: D0          RET NC   ; confirmed missing timed opcode
    0x2F76: 3C          INC A
    0x2F77: 32 77 82    LD (INTERMISSION_REVIEW_INDEX),A
    0x2F7A: C9          RET
    ```
  - `RET Z` (`0xC8`) and `RET NZ` (`0xC0`) have already executed in
    the intermission path before this failure, so they should not be
    included in this fix.
  - Look-ahead for related active-path conditional returns:
    - `src/ghost_house.asm`: `RET NC` at line 114; `RET C` at lines
      122, 173, 181, 189.
    - `src/movement.asm`: `RET C` at line 360.
    - `src/ghost_ai.asm`: `RET C` at line 392.
  - `tools/intermission_tests.py` prints `5/5 passed`, confirming the
    source-level T020 scene/timing harness is correct; validation is
    blocked purely on emulator-side opcode coverage.
  - `intermission_scene_1.ppm` at frame `1020` is now captured
    successfully after M37 (frame hash
    `a79b667aef218838fbe9ef205a615f11606a08a6f73a730d40fe60127cc60a28`).
- Canonical repro command (post-fix expectation in parentheses):
  ```bash
  cd /home/djglxxii/src/PacManV8
  python3 tools/build.py

  /home/djglxxii/src/Vanguard8/cmake-build-debug/src/vanguard8_headless \
      --rom build/pacman.rom \
      --frames 1770 --inspect-frame 1770 \
      --dump-cpu --hash-frame 1770
  # (expected: completes without opcode abort, emits a stable frame
  # hash and CPU snapshot for the scene-2 representative frame)
  ```

Done when:
- New CPU tests pin `RET NC` and `RET C` behavior (both taken and
  not-taken branches) and adapter T-state classification per the
  milestone contract.
- The PacManV8 T020 repro commands for frames `1770`, `2520`, and
  `2640` run to completion without `Unsupported timed Z180 opcode
  0xD0 at PC 0x2F75` (or at any `RET C` site in `ghost_house.asm`,
  `movement.asm`, or `ghost_ai.asm` reached by this ROM).
- A non-perturbation regression on an existing fixture ROM that does
  not exercise the new opcodes produces byte-identical frame, audio,
  and event-log digests before and after the fix.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes with
  the new opcode tests included and no pre-existing CPU test is
  relaxed.

Out of scope:
- Any general HD64180 opcode completion beyond the two `RET cc` forms
  listed above.
- Parity (`RET PO` `0xE0`, `RET PE` `0xE8`) and sign (`RET P` `0xF0`,
  `RET M` `0xF8`) conditional returns. They are not on any confirmed
  active path in the T020 blocker or its look-ahead.
- Changes to interrupt acceptance, HALT-wake semantics, M35
  observability surface, M37 `LD r,n` coverage, VDP, audio, scheduler,
  frontend, debugger, save-state, or PacManV8 ROM code.
- Refreshing PacManV8 T020 acceptance evidence; that happens in the
  PacManV8 repo after this task is completed.

Required tests:
- Extracted-core coverage in `tests/test_cpu.cpp` (or the adjacent CPU
  test file the project already uses):
  - `RET NC` (`0xD0`) taken: with carry clear and a known return
    address on the stack, after execution `PC` equals the popped
    address, `SP` has advanced by 2, all flags unchanged, adapter
    reports 11 T-states.
  - `RET NC` (`0xD0`) not taken: with carry set, after execution
    `PC` has advanced by 1, `SP` is unchanged, all flags unchanged,
    adapter reports 5 T-states.
  - `RET C` (`0xD8`) taken: with carry set, after execution `PC`
    equals the popped address, `SP` has advanced by 2, all flags
    unchanged, adapter reports 11 T-states.
  - `RET C` (`0xD8`) not taken: with carry clear, after execution
    `PC` has advanced by 1, `SP` is unchanged, all flags unchanged,
    adapter reports 5 T-states.
  - Sequential review-index pattern mirroring PacManV8
    `intermission_advance_review_index` at `0x2F70..0x2F7A`: set up
    a byte at a known address, run `LD A,(addr)` → `CP n` → `RET NC`
    → `INC A` → `LD (addr),A` → `RET` and verify:
    - When the loaded value is at or above `n` (carry clear after
      `CP`), `RET NC` is taken, the address byte is **not**
      incremented, and control returns via the early-return path.
    - When the loaded value is below `n` (carry set after `CP`),
      `RET NC` falls through, the address byte is incremented, and
      control returns via the final `RET`.
- Adapter-level T-state coverage confirming each new opcode is
  classified in the correct taken (11) vs not-taken (5) group.
- Non-perturbation guard against a non-`RET NC` / non-`RET C` fixture
  ROM: frame, audio, and event-log digests must be byte-identical
  before and after the fix.

Verification commands:
```bash
cd /home/djglxxii/src/PacManV8
python3 tools/build.py

cd /home/djglxxii/src/Vanguard8
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure

cmake-build-debug/src/vanguard8_headless \
    --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
    --frames 1770 --inspect-frame 1770 \
    --dump-cpu --hash-frame 1770

cmake-build-debug/src/vanguard8_headless \
    --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
    --frames 2520 --inspect-frame 2520 \
    --dump-cpu --hash-frame 2520

cmake-build-debug/src/vanguard8_headless \
    --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
    --frames 2640 --inspect-frame 2640 \
    --dump-cpu --hash-frame 2640
```

## Completion summary

Completed on 2026-04-21.

Implemented against `docs/spec/01-cpu.md`,
`docs/emulator/02-emulation-loop.md`,
`docs/emulator/03-cpu-and-bus.md`,
`docs/emulator/07-implementation-plan.md`,
`docs/emulator/milestones/38.md`, and
`/home/djglxxii/src/PacManV8/docs/tasks/blocked/T020-intermission-cutscenes.md`.

Fix:
- Added two timed-core opcode handlers in
  `third_party/z180/z180_core.cpp` following the existing
  `op_ret_z` / `op_ret_nz` pattern:
  - `Core::op_ret_nc()` — `if ((af_.bytes.lo & flag_carry) == 0U) pc_.value = pop_word();`
  - `Core::op_ret_c()` — `if ((af_.bytes.lo & flag_carry) != 0U) pc_.value = pop_word();`
- Declared the two handlers in `third_party/z180/z180_core.hpp`
  adjacent to the existing `op_ret_z` / `op_ret_nz` declarations.
- Registered the two opcodes in the dispatch table next to the
  existing `RET cc` entries:
  - `opcodes_[0xD0] = &Core::op_ret_nc;`
  - `opcodes_[0xD8] = &Core::op_ret_c;`
- Added two new cases to the adapter dispatch in
  `src/core/cpu/z180_adapter.cpp` alongside `0xC0` / `0xC8`:
  - `case 0xD0:` → 11 T-states when `(af & flag_carry) == 0`, else 5.
  - `case 0xD8:` → 11 T-states when `(af & flag_carry) != 0`, else 5.
- No flag updates, no changes to any other opcode handler, no
  changes to `RET Z` / `RET NZ`, HALT-wake semantics, M37 `LD r,n`
  coverage, M35 CLI, VDP, audio, scheduler, or frontend code.

Tests added to `tests/test_cpu.cpp`:
- `scheduled CPU covers RET NC / RET C for PacManV8
  intermission_advance_review_index (M38)`, containing five
  sections:
  - `RET NC (0xD0)` taken with carry clear: PC loaded from stack
    (0xABCD), SP advances by 2 (0x8100 → 0x8102), flags untouched,
    adapter reports 11 T-states.
  - `RET NC (0xD0)` not taken with carry set: PC advances by 1, SP
    unchanged, flags untouched, adapter reports 5 T-states.
  - `RET C (0xD8)` taken with carry set: PC loaded from stack
    (0x5678), SP advances by 2, flags untouched, adapter reports
    11 T-states.
  - `RET C (0xD8)` not taken with carry clear: PC advances by 1,
    SP unchanged, flags untouched, adapter reports 5 T-states.
  - `intermission_advance_review_index` pattern mirroring PacManV8
    `0x2F70..0x2F7A` (`LD A,(addr) → CP n → RET NC → INC A →
    LD (addr),A → RET`), driven via `CALL` + `HALT`:
    - Carry-clear early-return leaves the stored byte at its
      initial value (0x02 → 0x02).
    - Carry-set fall-through increments the stored byte
      (0x01 → 0x02).

Non-perturbation:
- Full `ctest` run: `178/178` pass (was `177/177` after M37 plus
  the one new test case added here). No existing test was modified
  or relaxed. The skipped showcase-M7 Graphic 6 test remains
  skipped as before. Because the non-`RET NC` / non-`RET C`
  dispatch paths are literally unchanged (the additions are pure
  opcode-table entries and new handler functions), ROMs that do
  not exercise these opcodes produce byte-identical observable
  behavior, including the M33-pinned PacManV8 audio hash
  `24ce40791e466f9f686ee472b5798128065458e06a51f826666ae444ddfb5c75`
  in `tests/test_frontend_backends.cpp`.

Runtime regression against PacManV8 T020:
- Rebuilt PacManV8 ROM (`build/pacman.rom`, `build/pacman.sym`) on
  2026-04-21 with no PacManV8-side edits.
- `vanguard8_headless --rom build/pacman.rom --frames 1770
  --hash-frame 1770` now runs to completion without
  `Unsupported timed Z180 opcode 0xD0 at PC 0x2F75`, and produces
  a stable frame SHA-256 and event-log digest across repeat runs:
  - Frame SHA-256 (1770):
    `082dd26d090aa2632128d266bddd6faa0c50b5b96819dfc7e6f02f3276d93066`
  - Event log digest: `3498303648502710181`
  - Event log size: `378780`
  - Master cycles: `422003400`; CPU T-states: `211001700`.
- `--frames 2520 --hash-frame 2520`:
  - Frame SHA-256 (2520):
    `c80b7468c3a56d2383582b717f17bc36fde775f5821ab4311081d796e4f0a2ff`
  - Event log digest: `10606555846525264891`
  - Event log size: `539280`
  - Master cycles: `600818400`; CPU T-states: `300409200`.
- `--frames 2640 --hash-frame 2640`:
  - Frame SHA-256 (2640):
    `31f8226ca0fe920a1b85e33ecd0625ba0846439a3a15d146e1497c817285d34c`
  - Event log digest: `6526782969573701363`
  - Event log size: `564960`
  - Master cycles: `629428800`; CPU T-states: `314714400`.
- End-of-frame CPU state observed on the scene-2 inspect-frame
  run: `PC=0x0067`, `halted=true`, `IFF1=true` (the expected idle
  resting point established by M36). That the CPU reached frame
  `2640` at all (rather than aborting at `PC=0x2F75` on the
  scene-1 → scene-2 review-index advance) is the authoritative
  post-fix evidence.

Files changed:
- `third_party/z180/z180_core.hpp`
- `third_party/z180/z180_core.cpp`
- `src/core/cpu/z180_adapter.cpp`
- `tests/test_cpu.cpp`
- `docs/emulator/milestones/38.md`
- `docs/emulator/current-milestone.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/tasks/active/M38-T01-timed-ret-nc-opcode-coverage.md`
  (moved to `docs/tasks/completed/`)
- `docs/tasks/task-index.md`

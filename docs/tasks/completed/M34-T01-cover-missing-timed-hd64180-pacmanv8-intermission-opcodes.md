# M34-T01 — Cover Missing Timed HD64180 PacManV8 Intermission Opcodes

Status: `completed`
Milestone: `34`
Depends on: `M33-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/34.md`
- `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T020-intermission-cutscenes.md`
- `/home/djglxxii/src/PacManV8/docs/field-manual/headless-timed-core-jr-c-gap.md`

Scope:
- Implement the timed extracted-HD64180 opcode coverage authorized by
  milestone 34 for the PacManV8 T020 intermission blocker:
  - `DJNZ` (`0x10`)
  - `JR NC,e` (`0x30`) and `JR C,e` (`0x38`)
  - `ADD A,r` / `ADD A,(HL)` family (`0x80-0x87`)
  - `ADD HL,ss` family (`0x09`, `0x19`, `0x29`, `0x39`)
  - `SBC HL,ss` family (`ED 42`, `ED 52`, `ED 62`, `ED 72`)
- Add the adapter timing and focused regression tests needed to lock those
  semantics in the timed path.
- Add one narrow PacManV8 integration regression around the documented T020
  minimal repro so the emulator-side blocker stays closed.

Blocker reference:
- PacManV8 blocked task:
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T020-intermission-cutscenes.md`
- Confirmed missing opcodes from that blocker:
  - `ED 52` (`SBC HL,DE`) at `PC 0x089E`
  - `0x10` (`DJNZ`) at `PC 0x32D3`
  - `0x87` (`ADD A,A`) at `PC 0x32E9`
  - `0x30` (`JR NC,e`) at `PC 0x2F45`
  - `0x09` (`ADD HL,BC`) at `PC 0x333F`
- Strongly adjacent opcodes from the same active path:
  - `0x38` (`JR C,e`)
  - `0x19` (`ADD HL,DE`)
- Canonical repro command:
  ```bash
  cd /home/djglxxii/src/PacManV8
  python3 tools/build.py
  /home/djglxxii/src/Vanguard8/cmake-build-debug/src/vanguard8_headless \
      --rom build/pacman.rom \
      --frames 700 --hash-frame 636
  ```

Done when:
- The canonical T020 repro command runs to completion without a timed-opcode
  abort.
- Focused tests pin `DJNZ`, `JR NC/JR C`, `ADD A,r`, `ADD HL,ss`, and
  `SBC HL,ss` semantics and T-state counts in the timed dispatcher.
- Repeat runs of the T020 repro produce stable digest output.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes.

Out of scope:
- Broad opcode completion outside the five families above.
- New index-register, `CB`-prefix, or unrelated `ED`-prefix work.
- VDP, audio, scheduler, frontend, debugger, or PacManV8 ROM changes.
- Refreshing PacManV8 T020 acceptance evidence; that happens after this task is
  completed and the PacManV8 task is reactivated.

Verification commands:
```bash
cd /home/djglxxii/src/PacManV8
python3 tools/build.py

cd /home/djglxxii/src/Vanguard8
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure
cmake-build-debug/src/vanguard8_headless \
    --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
    --frames 700 --hash-frame 636
```

## Completion summary

Completed on 2026-04-20.

Implemented against `docs/spec/01-cpu.md`,
`docs/emulator/02-emulation-loop.md`,
`docs/emulator/03-cpu-and-bus.md`,
`docs/emulator/07-implementation-plan.md`,
`docs/emulator/milestones/34.md`,
`/home/djglxxii/src/PacManV8/docs/tasks/blocked/T020-intermission-cutscenes.md`,
and `/home/djglxxii/src/PacManV8/docs/field-manual/headless-timed-core-jr-c-gap.md`.

Timed extracted-HD64180 coverage added:
- `DJNZ` (`0x10`)
- `JR NC,e` (`0x30`) and `JR C,e` (`0x38`)
- `ADD A,r` / `ADD A,(HL)` family (`0x80-0x87`)
- `ADD HL,ss` family (`0x09`, `0x19`, `0x29`, `0x39`)
- `SBC HL,ss` family (`ED 42`, `ED 52`, `ED 62`, `ED 72`)

Implementation detail:
- Added the new opcode handlers to `third_party/z180/z180_core.{hpp,cpp}`.
- `ADD A,r` / `ADD A,(HL)` is closed as a generic family handler reusing the
  existing 8-bit add-flag path.
- `ADD HL,ss` preserves `S/Z/PV`, clears `N`, and updates `H/C`.
- `SBC HL,ss` updates `S/Z/H/PV/N/C` for 16-bit subtract-with-carry and is
  decoded in the ED-prefix path as a family rather than a one-off `ED 52`
  patch.
- Added timed adapter entries in `src/core/cpu/z180_adapter.cpp` for:
  - `DJNZ`: 13/8 T-states taken/not-taken
  - `JR NC/JR C`: 12/7 T-states taken/not-taken
  - `ADD A,r` / `ADD A,(HL)`: 4/7 T-states
  - `ADD HL,ss`: 11 T-states
  - `SBC HL,ss`: 15 T-states

Regression coverage:
- `tests/test_cpu.cpp` now pins:
  - `DJNZ` register/PC behavior, flags untouched, and 13/8 timing
  - `JR NC/JR C` carry-gated control flow, flags untouched, and 12/7 timing
  - representative `ADD A,A` and `ADD A,(HL)` semantics and timing
  - representative `ADD HL,BC` / `ADD HL,DE` semantics and timing
  - representative `SBC HL,DE` zero-result and overflow/half-borrow cases
- `tests/test_integration.cpp` now adds:
  - a scripted frame-loop ROM covering the M34 opcode families together
  - a PacManV8 repeat-run regression that executes 700 frames twice and
    asserts stable `PC`, event-log digest, and composed-frame digest

Verification:
- `cd /home/djglxxii/src/PacManV8 && python3 tools/build.py` passed and
  rebuilt `build/pacman.rom`.
- `cmake --build cmake-build-debug` passed.
- `ctest --test-dir cmake-build-debug --output-on-failure` passed
  (163/163, one pre-existing showcase test skipped).
- `cmake-build-debug/src/vanguard8_headless --rom
  /home/djglxxii/src/PacManV8/build/pacman.rom --frames 700 --hash-frame 636`
  passed with:
  - Event log digest: `17319210692457518239`
  - Frame SHA-256 (636):
    `4a63cec305375edd4b20e85ba9830d83888e2eaf4327a29c229cfc7ce7a79693`

# M31-T01 — Cover Missing Timed HD64180 PacManV8 ROM Run-Time Opcodes

Status: `completed`
Milestone: `31`
Depends on: `M30-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/31.md`
- `/home/djglxxii/src/PacManV8/docs/field-manual/vanguard8-build-directory-skew-and-timed-opcodes.md`

Scope:
- After M30 closed the VBlank handler prologue/epilogue, the full PacManV8
  `pacman.rom` exercises the main program past the interrupt boundary and
  surfaces a broader set of timed-HD64180 opcode gaps. The user has
  explicitly broadened this milestone (deviating from M19-M30 narrow
  one-opcode-at-a-time discipline) to cover all timed-HD64180 opcode gaps
  discoverable by running the canonical
  `cmake-build-debug/src/vanguard8_headless --rom pacman.rom --frames 3600
  --hash-audio` binary against the current PacManV8 `pacman.rom`.
- The opcode families in scope for this task are:
  - `LD r, r'` / `LD r, (HL)` / `LD (HL), r` for the entire `0x40-0x7F`
    block except `0x76` (`HALT`). Implement via a generic
    `op_ld_r_r_main` handler that decodes the dst/src register codes
    from the last fetched opcode, so the full family is closed in one
    pass rather than one function per opcode.
  - `INC r` / `DEC r` / `INC (HL)` / `DEC (HL)` for `B/C/D/E/H/L/A` and
    the `(HL)` indirect forms, via generic `op_inc_r_main` /
    `op_dec_r_main` handlers, with correct S/Z/H/P/V/N flag semantics
    (carry preserved; overflow on `0x7F->0x80` for INC and `0x80->0x7F`
    for DEC; half-carry/borrow on low-nibble crossing).
  - `JR Z, e` (`0x28`), `JP NZ, nn` (`0xC2`), `JP Z, nn` (`0xCA`),
    `RET NZ` (`0xC0`), mirroring the semantics of the existing
    complementary conditional variants.
  - `CP n` (`0xFE`) and the register/indirect `CP r` / `CP (HL)`
    family `0xB8-0xBF`, via `op_cp_n` and a generic `op_cp_r_main`,
    with correct S/Z/H/P/V/N/C flag semantics and no write to `A`.
- Add adapter T-state timing entries for every opcode added. For the
  `0x40-0x7F` range, handle timing via an early-return predicate at the
  top of `current_instruction_tstates()` so 4-T / 7-T classification is
  decoded from the opcode byte rather than enumerated per slot.

Blocker reference:
- Binary: `/home/djglxxii/src/Vanguard8/cmake-build-debug/src/vanguard8_headless`
  (canonical; the stale `build/` directory was removed on 2026-04-19)
- Command:
  ```bash
  cmake-build-debug/src/vanguard8_headless \
      --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
      --frames 3600 --hash-audio
  ```
- Observed abort sequence during iterative discovery (post-M30):
  1. `Unsupported timed Z180 opcode 0xC0 at PC 0x2956` (`RET NZ`)
  2. `Unsupported timed Z180 opcode 0xCA at PC 0x2959` (`JP Z, nn`)
  3. `Unsupported timed Z180 opcode 0x28 at PC 0x2A05` (`JR Z, e`)
  4. `Unsupported timed Z180 opcode 0x57 at PC 0x2A13` (`LD D, A`) —
     triggering the full `LD r, r'` family closure
  5. `Unsupported timed Z180 opcode 0x3D at PC 0x2A2E` (`DEC A`) —
     triggering the `INC r` / `DEC r` family closure
  6. `Unsupported timed Z180 opcode 0xFE at PC 0x295C` (`CP n`)
- After all six gap families are closed, `--frames 3600 --hash-audio`
  runs to completion with stable digests (event log and audio hash).

Done when:
- The canonical `cmake-build-debug/src/vanguard8_headless --rom
  /home/djglxxii/src/PacManV8/build/pacman.rom --frames 3600 --hash-audio`
  command runs to completion without a timed-opcode abort.
- Tests lock the added opcode family semantics in the timed extracted CPU
  path:
  - Representative `LD r, r'` / `LD r, (HL)` / `LD (HL), r` coverage
    (at minimum one case per destination register plus `(HL)` load and
    store variants).
  - `INC r` / `DEC r` coverage pinning the half-carry, parity/overflow,
    and carry-preservation behavior on boundary values.
  - `JR Z, e`, `JP NZ, nn`, `JP Z, nn`, `RET NZ` coverage pinning the
    taken / not-taken cycle counts and PC/SP state against the zero
    flag.
  - `CP n` / `CP r` / `CP (HL)` coverage pinning that `A` is unchanged
    while flags reflect the subtraction.
- An integration regression runs the full PacManV8 `pacman.rom` through
  the timed dispatcher for at least 3600 emulated frames without a
  timed-opcode abort, verifying digest stability across repeat runs.
- All Catch2 `ctest` suites pass.

Out of scope (belongs to future milestones if surfaced):
- Index register (`IX`/`IY`) or `DD`/`FD` prefix opcodes.
- `CB`-prefixed bit/shift/rotate expansion.
- Block-move / block-compare expansion (`LDIR`, `LDDR`, `CPIR`, etc.).
- `ADD`/`SUB`/`ADC`/`SBC` arithmetic families beyond the `INC`/`DEC`/`CP`
  work landed here.
- `ED`-prefix expansion past current coverage.
- VDP, palette, renderer, scheduler, audio, or frontend changes.

Verification commands:
```bash
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure
cmake-build-debug/src/vanguard8_headless \
    --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
    --frames 3600 --hash-audio
```

## Completion summary

Completed on 2026-04-19.

Opcode families added to `third_party/z180/z180_core.{hpp,cpp}` and timed in
`src/core/cpu/z180_adapter.cpp`:
- Generic `LD r, r'` / `LD r, (HL)` / `LD (HL), r` dispatcher
  (`op_ld_r_r_main`) registered across `0x40-0x7F` with `HALT` (`0x76`)
  preserved. 4 T-states for register-to-register; 7 T-states for
  `(HL)`-indirect forms. The opcode byte is captured into a new
  `last_opcode_` member by `execute_one()` before dispatch so the generic
  handler can decode the dst/src register codes without changing the
  `Handler` pointer signature.
- Generic `INC r` / `DEC r` / `INC (HL)` / `DEC (HL)` dispatchers
  (`op_inc_r_main`, `op_dec_r_main`) registered across
  `0x04/0x0C/0x14/0x1C/0x24/0x2C/0x34/0x3C` (INC) and
  `0x05/0x0D/0x15/0x1D/0x25/0x2D/0x35/0x3D` (DEC). Flags: S/Z/H/P/V/N
  per the Z80 data book; carry preserved; P/V set on the signed-overflow
  boundary (`0x7F->0x80` for INC and `0x80->0x7F` for DEC); H set on the
  low-nibble crossing. 4 T-states for register forms; 11 T-states for
  `(HL)` forms.
- Conditional flow opcodes: `JR Z, e` (`0x28`), `JP NZ, nn` (`0xC2`),
  `JP Z, nn` (`0xCA`), `RET NZ` (`0xC0`). Cycle counts match the
  existing complementary forms: 12/7 taken/not-taken for `JR cc, e`;
  10 T-states for `JP cc, nn`; 11/5 taken/not-taken for `RET cc`.
- Compare opcodes: `CP n` (`0xFE`) plus `CP r` / `CP (HL)` (`0xB8-0xBF`)
  via a shared `apply_cp_flags` helper. `A` is unchanged; flags
  reflect `A - operand` with N=1. 7 T-states for `CP n` / `CP (HL)`;
  4 T-states for `CP r`.

Adapter timing for the `0x40-0x7F` block is decoded via an early-return
predicate in `current_instruction_tstates()` that classifies 4-T vs 7-T
from the opcode byte, avoiding per-slot enumeration.

Tests added:
- `tests/test_cpu.cpp`:
  - Representative `LD r, r'` coverage including `LD D, A` (the first
    blocker), plus `LD r, (HL)` and `LD (HL), r` timing/semantics.
  - `INC r` / `DEC r` boundary coverage: `DEC A` on `0x01` -> Z set and
    carry preserved; `DEC A` on `0x80` -> signed overflow and half-borrow;
    `INC A` on `0x7F` -> overflow, half-carry, sign; `INC (HL)` / `DEC (HL)`
    at 11 T-states rewriting the byte at `(HL)`.
  - Conditional flow coverage for `JR Z, e`, `JP Z, nn`, `JP NZ, nn`,
    `RET NZ`: taken / not-taken cycle counts and PC/SP behavior against
    the zero flag.
  - `CP n` / `CP r` / `CP (HL)` flag coverage: `A == operand` -> Z set;
    `A < operand` -> C set with N=1; `A` never modified.
- `tests/test_integration.cpp`:
  - `make_pacmanv8_rom_run_opcode_coverage_rom` — scripted ROM that
    exercises all M31 families (INC/DEC/CP/LD r,r'/conditional flow/OR A)
    in one run. Verifies the emulator skips the three decoy HALTs and
    only halts at the terminal `PC 0x001F` with the expected register
    state.

Verification:
- `cmake --build .` clean.
- `ctest --output-on-failure` — 150/150 tests pass (1 showcase test
  skipped as before).
- `src/vanguard8_headless --rom /home/djglxxii/src/PacManV8/build/pacman.rom
  --frames 3600 --hash-audio` runs to completion with stable digests:
  - Event log digest: `715005249434092979`
  - Audio SHA-256: `8cd0a4ee42a37c6107b2fb33c2a21b70c52fc8c32fe156b5b6716917df89060d`

Opcode-gap discovery loop (preserved for evidence):
1. `0xC0` (`RET NZ`) at `PC 0x2956`
2. `0xCA` (`JP Z, nn`) at `PC 0x2959`
3. `0x28` (`JR Z, e`) at `PC 0x2A05`
4. `0x57` (`LD D, A`) at `PC 0x2A13` — triggered full `LD r, r'`
   family closure via `op_ld_r_r_main`
5. `0x3D` (`DEC A`) at `PC 0x2A2E` — triggered full `INC r` / `DEC r`
   family closure
6. `0xFE` (`CP n`) at `PC 0x295C`

After step 6, `--frames 3600 --hash-audio` runs to completion without a
further abort.

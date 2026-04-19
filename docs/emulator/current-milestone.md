# Current Milestone Lock

- Active milestone: `31`
- Title: `Timed HD64180 PacManV8 ROM Run-Time Opcode Coverage`
- Status: `accepted`
- Locked on plan: `docs/emulator/07-implementation-plan.md`
- Contract file: `docs/emulator/milestones/31.md`

Execution rules:
- Only milestone `31` task files present in `docs/tasks/active/` may be
  executed.
- Milestone `30` is accepted.
- Keep milestone `31` work inside `src/core/`, `third_party/z180/`,
  `tests/`, and `docs/` as allowed by the milestone contract.
- Milestone `31` deviates from the one-opcode-at-a-time discipline of
  M19-M30: the user has explicitly broadened scope to close all
  timed-HD64180 opcode gaps surfaced by running the PacManV8
  `pacman.rom` for `--frames 3600 --hash-audio` against the canonical
  `cmake-build-debug/src/vanguard8_headless` binary.
- The opcode families in scope are those discovered iteratively after
  M30 landed:
  - `LD r, r'` / `LD r, (HL)` / `LD (HL), r` across the `0x40-0x7F`
    block except `0x76` (`HALT`).
  - `INC r` / `DEC r` / `INC (HL)` / `DEC (HL)`.
  - `JR Z, e` (`0x28`), `JP NZ, nn` (`0xC2`), `JP Z, nn` (`0xCA`),
    `RET NZ` (`0xC0`).
  - `CP n` (`0xFE`) and `CP r` / `CP (HL)` (`0xB8-0xBF`).
- Do not broaden this milestone into index-register, `CB`-prefix,
  block-instruction, or general arithmetic (`ADD`/`SUB`/`ADC`/`SBC`)
  work; only opcode families surfaced by the iterative discovery loop
  above may be added. Any further gap that appears later belongs to a
  subsequent narrow compatibility milestone.
- Canonical Vanguard8 headless binary for verification is
  `cmake-build-debug/src/vanguard8_headless`. The stale `build/`
  CMake directory was removed on 2026-04-19; any prior task or
  field-manual note still pointing at `build/src/vanguard8_headless`
  is out of date.

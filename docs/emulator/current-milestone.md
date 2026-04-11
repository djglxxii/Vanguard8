# Current Milestone Lock

- Active milestone: `20`
- Title: `Timed HD64180 Palette VCLK Compatibility`
- Status: `accepted`
- Locked on plan: `docs/emulator/07-implementation-plan.md`
- Contract file: `docs/emulator/milestones/20.md`

Execution rules:
- Only milestone `20` task files present in `docs/tasks/active/` may be
  executed.
- Milestone `19` is accepted.
- Keep milestone `20` work inside `src/core/`, `tests/`, and `docs/` as
  allowed by the milestone contract.
- Milestone `20` is the narrow timed-CPU compatibility pass for the Pac-Man
  palette swatch evidence at
  `/home/djglxxii/src/pacman/vanguard8_port/tests/evidence/T004-palette/`.
- The active blocker is limited to the `VCLK 4000` runtime abort currently
  reported as missing timed opcode `0x05` at `PC 0x02B9`; the same palette
  scene completes with `VCLK: off`, so the next pass stays focused on the
  timed CPU path rather than palette decoding or VDP surface changes.
- Milestone `20` was accepted after human review of the `DEC B` timed CPU
  fix and runtime coverage for the `T004-palette` `VCLK 4000` blocker at
  `PC 0x02B9`.

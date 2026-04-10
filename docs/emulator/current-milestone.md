# Current Milestone Lock

- Active milestone: `19`
- Title: `Timed HD64180 Boot Opcode Compatibility`
- Status: `ready_for_verification`
- Locked on plan: `docs/emulator/07-implementation-plan.md`
- Contract file: `docs/emulator/milestones/19.md`

Execution rules:
- Only milestone `19` task files present in `docs/tasks/active/` may be
  executed.
- Milestone `18` is accepted.
- Keep milestone `19` work inside `src/core/`, `tests/`, and `docs/` as
  allowed by the milestone contract.
- Milestone `19` is the narrow timed-CPU compatibility pass for the Pac-Man
  boot/background bring-up evidence at
  `/home/djglxxii/src/pacman/vanguard8_port/tests/evidence/T002-boot-background/`.
- The active blocker is limited to the missing timed opcode surface currently
  reported as base opcode `0x06` (`LD B,n`) at `PC 0x0140` and ED opcode
  `0x56` (`IM 1`) at `PC 0x0314`.
- Milestone `19` implementation is complete and the contract verification
  commands have passed on the milestone tree; the milestone now awaits human
  verification before being marked `accepted`.

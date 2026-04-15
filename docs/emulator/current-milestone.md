# Current Milestone Lock

- Active milestone: `21`
- Title: `Timed HD64180 Boot Opcode Compatibility (PacManV8)`
- Status: `accepted`
- Locked on plan: `docs/emulator/07-implementation-plan.md`
- Contract file: `docs/emulator/milestones/21.md`

Execution rules:
- Only milestone `21` task files present in `docs/tasks/active/` may be
  executed.
- Milestone `20` is accepted.
- Keep milestone `21` work inside `src/core/`, `tests/`, and `docs/` as
  allowed by the milestone contract.
- Milestone `21` is the narrow timed-CPU compatibility pass for the PacManV8
  minimal boot ROM blocker reported in
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T001-build-system-and-minimal-boot.md`.
- The active blocker was limited to missing timed opcodes `0x0E` (`LD C,n`)
  and `0x16` (`LD D,n`) at `PC 0x012A`; both are now covered in the extracted
  z180 core and the timed instruction timing table.
- Milestone `21` was accepted after human review of the `LD C,n` and `LD D,n`
  timed CPU fixes and all 124 tests passing.

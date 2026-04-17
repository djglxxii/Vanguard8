# Current Milestone Lock

- Active milestone: `29`
- Title: `Timed HD64180 VDP-B Framebuffer Copy RET Z / DEC DE Compatibility (PacManV8)`
- Status: `accepted`
- Locked on plan: `docs/emulator/07-implementation-plan.md`
- Contract file: `docs/emulator/milestones/29.md`

Execution rules:
- Only milestone `29` task files present in `docs/tasks/active/` may be
  executed.
- Milestone `28` is accepted.
- Keep milestone `29` work inside `src/core/`, `tests/`, and `docs/` as
  allowed by the milestone contract.
- Milestone `29` is the narrow timed-CPU compatibility pass for the two
  remaining PacManV8 blockers designated by
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T007-vdp-b-maze-render-and-pellet-display.md`
  after the milestone-28 `OR E` patch:
  - `0xC8` (`RET Z`) at `PC 0x0248`
  - `0x1B` (`DEC DE`) at `PC 0x024D`
- These two opcodes were explicitly named by the T007 blocker's static
  audit as the remaining known gaps on the VDP-B framebuffer copy path;
  they are authorized to ship together so T007 can be unblocked in a
  single runtime-dump retry.
- Do not broaden this milestone into a general CPU completion pass, a
  full conditional-return sweep, or a generic 16-bit-decrement sweep;
  only the opcode coverage proven necessary by the designated blockers
  may be added.

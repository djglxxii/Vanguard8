# SR01-T01 — Implement the Bring-Up Cartridge Loop, MMU/Vector Setup, and First Banked Checkpoint

Status: `completed`
Milestone: `1`
Depends on: `SR00-T01`

Implements against:
- `docs/spec/00-overview.md`
- `docs/spec/01-cpu.md`
- `docs/spec/04-io.md`
- `showcase/docs/showcase-rom-plan.md`
- `showcase/docs/showcase-implementation-plan.md`
- `showcase/docs/milestones/01.md`

Scope:
- Add ROM-side boot/reset code, MMU setup, and interrupt vector setup.
- Add the first deterministic scene loop scaffold.
- Add at least one banked-content transition.
- Add the first checkpoint entry and the notes needed to review it.

Done when:
- The ROM boots through a deterministic loop and crosses a banked transition.
- The first checkpoint can be reproduced with the documented run command.
- The completed task summary explains the current loop structure and any
  remaining milestone-1 limitations.

Completion summary:
- Replaced the milestone-0 hello ROM with a milestone-1 bring-up cartridge in
  `showcase/src/showcase.asm` that now performs the documented MMU boot setup
  (`CBAR=0x48`, `CBR=0xF0`, `BBR=0x04`), programs the SRAM-backed vectored
  interrupt table entries for `INT1`, `PRT0`, and `PRT1`, probes two bank pages
  through the shared logical address `0x4001`, and then loops forever by
  calling the current bank-window entry at `0x4000`.
- Added two bank pages inside the ROM image so the fixed-ROM loop repeatedly
  crosses the bank window and swaps VDP-A backdrop color between palette entry
  `2` (bank 0) and palette entry `1` (bank 1). The first 32 traced
  instructions now include the bank probe itself, making the bank transition
  reviewable with the stable wrapper defaults.
- Added the first checkpoint manifest at
  `showcase/tests/milestone-01-checkpoints.md` and updated showcase README/test
  notes so the milestone-1 review command and expected frame hash are
  documented in-tree.
- Verification commands passed:
  `python3 showcase/tools/package/build_showcase.py`
  `python3 showcase/tools/package/run_showcase_headless.py --frames 240 --trace build/showcase/m1.trace --hash-frame 239 --symbols build/showcase/showcase.sym`
- Current milestone-1 limitation: the ROM sets up the documented vectored
  interrupt table but does not enable live INT1-driven pacing yet, because
  toggling `/VCLK` from inside the running ROM currently exposes a separate
  emulator runtime issue outside showcase milestone-1 scope.

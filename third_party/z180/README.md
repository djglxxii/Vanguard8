# Z180 Extraction

This directory now contains the milestone-2 Vanguard 8 CPU extraction:

- Source basis: MAME Z180 core from `src/devices/cpu/z180/`
- Source snapshot: `mamedev/mame` commit `a6342949c0f5bdf0812b29074cd5c34701255b8e`
- Extracted into: `z180_core.hpp` and `z180_core.cpp`

Scope of this extraction:

- Reset-state handling derived from MAME's `device_reset()`, with Vanguard 8
  spec-preserving overrides where the repo spec is authoritative.
- MMU translation and `CBR`/`BBR`/`CBAR` register behavior derived from MAME's
  `z180_mmu()` path and adapted to the repo's documented flat-reset boot state.
- INT0 IM1 and INT1 vectored service logic derived from MAME's interrupt path
  and adapted to the repo's documented `IL & 0xF8` vector-pointer rule.
- Opcode handlers extracted for the milestone-2 boot and register tests:
  `NOP`, `LD HL,nn`, `LD (nn),HL`, `LD SP,nn`, `LD (nn),A`, `LD A,(nn)`,
  `LD A,n`, `HALT`, `JP nn`, `RET`, `CALL nn`, `DI`, `EI`, `OUT0 (n),A`,
  `LD I,A`, and `RETI`.

Non-goals for this milestone extraction:

- Full MAME device-framework integration
- ASCI/CSIO subdevices
- DMA and timer execution
- Full opcode coverage beyond the milestone-2 test surface

The repository spec remains authoritative. Where MAME behavior and the repo
spec disagree, the Vanguard 8 spec wins and the extraction is adapted to it.

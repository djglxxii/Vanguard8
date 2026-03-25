# M02-T03 — Wire Interrupt Lines and Prove the Boot ROM MMU Sequence

Status: `active`
Milestone: `2`
Depends on: `M02-T02`

Implements against:
- `docs/spec/00-overview.md`
- `docs/spec/01-cpu.md`
- `docs/emulator/03-cpu-and-bus.md`

Scope:
- Wire INT0 and INT1 into the CPU adapter.
- Lock INT0 IM1 behavior and INT1 vectored behavior.
- Prove boot ROM reconfiguration to `CBAR=0x48`, `CBR=0xF0`, `BBR=0x04`.

Done when:
- A test ROM can boot, switch banks, and read/write SRAM correctly.
- Interrupt-path tests cover INT0 and INT1 semantics independently.

Completion summary:
- Pending.

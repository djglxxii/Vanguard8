# M08-T01 — Implement DMA Transfer Paths

Status: `completed`
Milestone: `8`
Depends on: `M07-T03`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/emulator/03-cpu-and-bus.md`

Scope:
- Implement DMA channel 0 memory-to-memory transfers.
- Implement DMA channel 1 memory-to-I/O transfers.
- Reuse the normal bus and port dispatch paths.

Done when:
- DMA follows documented source/destination rules.
- Tests prove DMA-to-I/O uses the same port route as CPU `OUT`.

Completion summary:
- Implemented against `docs/spec/01-cpu.md` and
  `docs/emulator/03-cpu-and-bus.md`.
- Added the milestone-8 DMA register surface to
  `src/core/cpu/z180_adapter.hpp` and `src/core/cpu/z180_adapter.cpp`,
  covering `SAR0`/`DAR0`/`BCR0`, `MAR1`/`IAR1`/`BCR1`, and the documented
  `DSTAT`/`DMODE`/`DCNTL` register bytes.
- Added explicit DMA execution paths for channel 0 physical memory-to-memory
  copies and channel 1 physical memory-to-I/O writes, with channel 1 reusing
  the normal `Bus::write_port()` dispatch path exactly as the CPU `OUT` path
  does.
- Kept unspecified HD64180 mode/control behavior out of scope: nonzero
  `DMODE`/`DCNTL` values now warn and skip execution rather than inventing
  undocumented transfer semantics.
- Added CPU coverage in `tests/test_cpu.cpp` for channel 0 copy routing,
  channel 1 bus-port reuse, and unsupported DMA mode/control guardrails.

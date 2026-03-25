# M08-T01 — Implement DMA Transfer Paths

Status: `planned`
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
- Pending.

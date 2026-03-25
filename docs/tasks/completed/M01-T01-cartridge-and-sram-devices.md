# M01-T01 — Implement Cartridge and SRAM Devices

Status: `completed`
Milestone: `1`
Depends on: `M00-T02`

Implements against:
- `docs/spec/00-overview.md`
- `docs/emulator/03-cpu-and-bus.md`

Scope:
- Add cartridge image storage for the fixed region and 59 switchable 16 KB
  banks.
- Add the 32 KB system SRAM device at physical `0xF0000-0xF7FFF`.
- Enforce the 960 KB cartridge ROM limit.

Done when:
- Physical memory devices exist independently of CPU/MMU logic.
- Tests lock ROM size validation and base device ranges.

Completion summary:
- Implemented against `docs/spec/00-overview.md` and
  `docs/emulator/03-cpu-and-bus.md`.
- Added `CartridgeSlot` with 16 KB page validation, 960 KB size enforcement,
  fixed-region reads, switchable-bank reads, and `0xFF` behavior beyond the
  loaded ROM image.
- Added 32 KB `Sram` mapped to physical `0xF0000-0xF7FFF`.

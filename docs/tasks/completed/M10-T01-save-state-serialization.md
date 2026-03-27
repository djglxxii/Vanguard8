# M10-T01 — Implement Save-State Serialization

Status: `completed`
Milestone: `10`
Depends on: `M09-T03`

Implements against:
- `docs/emulator/00-overview.md`
- `docs/emulator/07-implementation-plan.md`

Scope:
- Add a versioned save-state format.
- Cover CPU, MMU, scheduler, VRAM, palettes, audio state, and other core
  machine state.
- Keep the format explicit and testable.

Done when:
- Save/load round-trips preserve the documented emulator state surface.
- Tests lock versioned serialization behavior.

Completion summary:
- Added a versioned `V8ST` save-state format with `SaveState::serialize()`
  and `SaveState::load()` entry points over explicit emulator, bus, CPU,
  scheduler, VDP, memory, and audio snapshot surfaces.
- Extended the core state model so CPU/MMU/DMA registers, scheduler queues,
  SRAM/ROM contents, controller state, VDP VRAM/register/palette/command
  state, and audio/mixer state can be captured and restored deterministically.
- Added round-trip coverage proving save/load preserves the documented core
  state surface and that restored emulation continues with matching frame,
  event-log, and audio-output digests.
- Hardened AY-3-8910 restore to rebind the configured DAC table pointer after
  deserialization so replayed chip state does not depend on stale process
  addresses.

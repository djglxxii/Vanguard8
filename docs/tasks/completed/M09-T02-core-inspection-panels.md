# M09-T02 — Add CPU, Memory, and VDP Inspection Panels

Status: `completed`
Milestone: `9`
Depends on: `M09-T01`

Implements against:
- `docs/emulator/06-debugger.md`

Scope:
- Add CPU register/disassembly controls.
- Add memory views for SRAM, ROM, and VRAM.
- Add VDP register and palette inspection.

Done when:
- Core state can be inspected without ad hoc logging.
- Step/run/pause actions stop at the intended boundaries.

Completion summary:
- Strengthened the milestone-9 contract to allow the narrow read-only core and
  CPU accessors required by the inspection panels.
- Added CPU register/timer/DMA snapshots plus a forward disassembly view and a
  minimal debugger command queue for `pause`, `resume`, and `step_frame`.
- Added memory-region views for SRAM, fixed ROM, banked ROM, logical CPU
  space, and both VDP VRAMs.
- Added VDP register/status/palette inspection snapshots for VDP-A and VDP-B.
- Extended tests to cover CPU/disassembly inspection, control-boundary
  behavior, and memory/VDP panel snapshots.

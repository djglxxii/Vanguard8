# M14-T02 — Add Symbol Loading and Basic Annotation Support

Status: `completed`
Milestone: `14`
Depends on: `M14-T01`

Implements against:
- `docs/emulator/06-debugger.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/11-pre-gui-audit.md`
- `docs/emulator/milestones/14.md`

Scope:
- Implement `.sym` loading.
- Add symbol lookup/annotation support to the covered disassembly, memory, and
  trace surfaces where model-layer seams already exist.

Done when:
- Covered debugger/model surfaces can show symbol-aware annotations without
  requiring a full desktop debugger UI.

Completion summary:
- Added a concrete `src/core/symbols.*` loader for flat `.sym` files with
  duplicate validation, exact-name lookup, and nearest-label formatting over
  the documented 16-bit logical address space.
- Wired symbol-aware annotations into the covered debugger/model seams in
  `src/debugger/cpu_panel.*`, `src/debugger/memory_panel.*`, and
  `src/debugger/trace_panel.*` so decoded disassembly, logical memory snapshots,
  and trace-to-file output can surface labels without a live desktop debugger UI.
- Extended `src/frontend/headless.cpp` so trace mode accepts `--symbols` and
  auto-loads a same-base `.sym` file next to the ROM when present, then added
  regression coverage in `tests/test_symbols.cpp` and updated the current
  debugger/overview docs to match the implemented milestone-14 surface.

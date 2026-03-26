# M09-T01 — Add the ImGui Shell and Docking Layout

Status: `completed`
Milestone: `9`
Depends on: `M08-T03`

Implements against:
- `docs/emulator/06-debugger.md`

Scope:
- Add the debugger shell and docking layout.
- Wire the debugger into the frontend without mutating emulation state.
- Keep the first pass limited to the foundation.

Done when:
- The debugger can attach and render without changing core behavior.
- Tests prove attach/no-attach equivalence where applicable.

Completion summary:
- Advanced the lock to milestone 9 and promoted the milestone-9 task queue.
- Added a concrete `DebuggerShell` foundation with default dock regions, panel
  visibility state, emulator/display attachment, and non-mutating render
  snapshots in `src/debugger/`.
- Wired the frontend CLI path to instantiate and attach the debugger shell and
  expose the foundation state through a `--debugger` runtime flag.
- Added debugger tests proving the default docking layout and that a visible
  render snapshot does not mutate emulator or display state.

# M14-T01 — Add Trace-to-File Support and Lightweight Runtime Debug Output

Status: `completed`
Milestone: `14`
Depends on: `M13-T03`

Implements against:
- `docs/emulator/00-overview.md`
- `docs/emulator/06-debugger.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/14.md`

Scope:
- Implement trace-to-file over the existing trace surface.
- Add only the lightweight runtime/debug output that materially helps bring-up,
  such as narrow status/error or perf reporting.
- Keep the work explicitly below the threshold of a full live debugger UI.

Done when:
- A developer can persist execution traces to disk and inspect narrow runtime
  status without needing an ImGui debugger shell.

Completion summary:
- Added a covered trace-to-file path in `src/debugger/trace_panel.*` and
  `src/frontend/headless.cpp` so the headless CLI can write decoded
  instruction history to disk with `--trace` and `--trace-instructions`
  without needing a live debugger UI.
- Added a shared narrow runtime-summary formatter and used it from the new
  trace path plus the desktop frontend `F10` status output, keeping milestone-14
  debug/status reporting lightweight and CLI-visible.
- Added regression coverage in `tests/test_trace.cpp`, updated build wiring for
  the debugger trace model, and documented the current repo-level trace
  precision boundary in `docs/emulator/00-overview.md` and
  `docs/emulator/06-debugger.md`.

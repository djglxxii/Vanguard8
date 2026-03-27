# M09-T03 — Add Breakpoints, Watchpoints, and Core Logs

Status: `completed`
Milestone: `9`
Depends on: `M09-T02`

Implements against:
- `docs/emulator/06-debugger.md`

Scope:
- Add interrupt log and bank tracker views.
- Add basic breakpoints and watchpoints.
- Keep behavior deterministic under step/run control.

Done when:
- Breakpoints trigger deterministically.
- Interrupt and bank history can be inspected from the debugger.

Completion summary:
- Added scheduler-derived interrupt-log snapshots and BBR bank-switch history
  views through the debugger panels.
- Added basic deterministic breakpoint/watchpoint support to the extracted
  Z180 adapter for PC, memory-read, memory-write, I/O-read, and I/O-write
  matches, with hit history and run-until-breakpoint control.
- Routed CPU-executed logical memory and internal `IN0`/`OUT0` accesses through
  shared debug observation callbacks so watchpoints and bank logs reflect real
  instruction execution rather than only external helper calls.
- Extended the debugger tests to cover deterministic breakpoint hits, interrupt
  log classification, and bank-tracker legality decoding.

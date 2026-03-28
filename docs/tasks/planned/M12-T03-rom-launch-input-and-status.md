# M12-T03 — Wire Command-Line ROM Launch, Minimal Live Input, and Runtime Status Closure

Status: `planned`
Milestone: `12`
Depends on: `M12-T02`

Implements against:
- `docs/spec/04-io.md`
- `docs/emulator/00-overview.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/10-desktop-gui-audit.md`
- `docs/emulator/milestones/12.md`

Scope:
- Launch a ROM directly into the live desktop session from the command line.
- Add the narrow live keyboard/gamepad handling needed for covered controller
  input and quit/fullscreen bring-up paths.
- Close basic runtime status/error/debug output for the first playable desktop
  build without adding a full desktop debugger UI.

Done when:
- `vanguard8_frontend --rom <path>` can load a ROM, present live video, emit
  audible audio, respond to the covered input path, and surface runtime
  failures without relying on terminal-only reporting.

Completion summary:

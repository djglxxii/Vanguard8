# M12-T02 — Add the Real Display and Audio Backends plus Deterministic Desktop Seams

Status: `active`
Milestone: `12`
Depends on: `M12-T01`

Implements against:
- `docs/emulator/04-video.md`
- `docs/emulator/05-audio.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/10-desktop-gui-audit.md`
- `docs/emulator/milestones/12.md`

Scope:
- Add the desktop display backend for real texture upload/present.
- Connect the existing deterministic mixer/resampler path to a live SDL audio
  device through testable frontend seams.
- Preserve deterministic readback/testing support so desktop and headless paths
  can still be compared safely.

Done when:
- A known composited frame can be presented through the desktop backend and
  verified through a deterministic readback seam.
- Generated emulator audio can reach a live desktop audio device through a
  covered queue/ring-buffer path with clean startup and shutdown behavior.

Completion summary:

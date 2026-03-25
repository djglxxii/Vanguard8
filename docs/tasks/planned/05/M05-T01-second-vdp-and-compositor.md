# M05-T01 — Add the Second VDP and the Compositor

Status: `planned`
Milestone: `5`
Depends on: `M04-T03`

Implements against:
- `docs/spec/00-overview.md`
- `docs/spec/02-video.md`
- `docs/emulator/04-video.md`

Scope:
- Add a second V9938 instance.
- Implement the VDP-A-over-VDP-B mux with VDP-A palette index `0`
  transparency.
- Add layer toggles usable by tests and later debugger UI.

Done when:
- Compositor tests prove transparency and nonzero override behavior.
- The dual-VDP path stays synchronized per frame.

Completion summary:
- Pending.

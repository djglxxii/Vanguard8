# M11-T01 — Expand Documented VDP Mode Coverage

Status: `completed`
Milestone: `11`
Depends on: `M10-T03`

Implements against:
- `docs/spec/02-video.md`
- `docs/emulator/04-video.md`

Scope:
- Add VDP modes beyond the initial Graphic 4 path where the spec is already
  sufficient.
- Add mixed-mode validation only where the repo spec is explicit.
- Keep deferred or unspecified video behavior out of scope.

Done when:
- Newly implemented modes are locked by dedicated rendering tests.
- Mixed-mode support is only added where the written spec supports it.

Completion summary:
- Added explicit V9938 mode-bit decoding to the repo docs and renderer so mode
  selection no longer relies on the old implicit "always Graphic 4" behavior.
- Implemented the documented fixed-layout Graphic 3 background and Sprite Mode
  2 path, with dedicated rendering coverage and the explicit mixed-mode case of
  VDP-A Graphic 3 composited over VDP-B Graphic 4.
- Locked the remaining Graphic 3 LN=1 ambiguity in the spec instead of
  inventing the extra 20-line fetch rule or Graphic 3 `R#23` scroll behavior.

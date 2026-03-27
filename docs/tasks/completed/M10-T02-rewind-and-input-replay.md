# M10-T02 — Add Rewind and Deterministic Input Replay

Status: `completed`
Milestone: `10`
Depends on: `M10-T01`

Implements against:
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/07-implementation-plan.md`

Scope:
- Add the rewind ring buffer.
- Add deterministic input recording and replay.
- Keep both systems rooted in save-state correctness.

Done when:
- Rewind restores prior states without AV desynchronization.
- Replay produces stable state progression across runs.

Completion summary:
- Added a save-state-backed rewind ring buffer that captures explicit snapshot
  bytes, restores prior snapshots deterministically, and discards future
  history cleanly when execution resumes from a rewound point.
- Implemented versioned `.v8r` recording and playback with ROM SHA-256
  anchoring, contiguous per-frame controller-port records, and embedded
  save-state anchors for replay sessions that start from a non-power-on state.
- Added replay and rewind coverage proving prior snapshots restore without
  frame/audio digest drift and that embedded-save-state replay reproduces the
  same controller progression and runtime state as the original run.

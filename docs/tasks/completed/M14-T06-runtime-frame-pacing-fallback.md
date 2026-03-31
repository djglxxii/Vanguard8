# M14-T06 — Restore Documented Desktop Frame Pacing Fallback in the Runtime Loop

Status: `completed`
Milestone: `14`
Depends on: `M12-T01`

Implements against:
- `docs/spec/04-io.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/14.md`

Scope:
- Reconnect the existing frontend `frame_pacing` option to the live desktop
  runtime loop.
- Apply only the documented narrow behavior: keep normal runtime near 59.94 Hz
  when the host is not otherwise pacing presentation.
- Add deterministic frontend-runtime tests so the pacing toggle stays visible
  in the repo.

Done when:
- The persistent desktop runtime no longer spins uncapped when
  `frame_pacing = true`.
- The runtime can still run uncapped when `frame_pacing = false`.
- The change is recorded in milestone-14 task history instead of living only as
  an incidental frontend diff.

Completion summary:
- Restored the software frame-limiter fallback in `src/frontend/runtime.*`
  using the existing `frame_pacing` config/CLI surface, targeting the
  documented 59.94 Hz desktop cadence when the host is not already pacing
  presentation.
- Extended `tests/test_frontend_runtime.cpp` with deterministic fake-clock
  coverage for both enabled and disabled pacing, so the runtime loop no longer
  silently regresses to a tight uncapped loop.
- Updated `docs/emulator/02-emulation-loop.md` and the milestone task index to
  keep the closure visible under milestone 14 rather than burying it in
  unrelated frontend notes.

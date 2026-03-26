# M08-T03 — Implement VDP Command Engine and CE Timing

Status: `completed`
Milestone: `8`
Depends on: `M08-T02`

Implements against:
- `docs/spec/02-video.md`
- `docs/emulator/04-video.md`

Scope:
- Implement the documented VDP command set.
- Expose `S#2.CE` while commands are in progress.
- Clear `CE` at the correct cycle boundary.

Done when:
- CPU polling distinguishes in-progress versus complete states.
- Tests lock command completion timing.

Completion summary:
- Implemented against `docs/spec/02-video.md` and `docs/emulator/04-video.md`.
- Filled the missing repo command-engine details first: added `R#45` argument
  bit layout, `S#2.TR` / `S#2.BD` / `S#2.CE`, `POINT` result in `S#7`, and
  the current milestone-8 Graphic-4 command-packing boundary to the video
  docs.
- Added asynchronous command state to `src/core/video/v9938.cpp` /
  `src/core/video/v9938.hpp`, including `CE` tracking, `TR` handling for
  CPU-streamed commands, and command advancement on the master-cycle timeline.
- Wired command advancement into `src/core/emulator.cpp` so VDP command
  progress moves with the same master-cycle accounting as CPU/audio/timer
  state.
- Implemented the documented opcode surface on the covered Graphic-4 path:
  `STOP`, `POINT`, `PSET`, `SRCH`, `LINE`, `LMMV`, `LMMM`, `LMMC`, `HMMV`,
  `HMMM`, `YMMM`, and `HMMC`.
- Locked exact `CE` timing for `HMMM` using the documented
  `64 + (NX × NY × 3)` master-cycle formula; other covered command opcodes
  remain asynchronous with a narrower documented timing contract until the repo
  spec grows source-backed per-command timing formulas.
- Added video tests in `tests/test_video.cpp` covering `HMMM` copy plus CE
  completion timing, `PSET`/`POINT`, `HMMV`, `SRCH`, `LINE`, `HMMC`, `LMMV`,
  `LMMM`, `YMMM`, and `LMMC`.

# M03-T01 — Implement Master-Cycle Counter and Scheduler Core

Status: `completed`
Milestone: `3`
Depends on: `M02-T03`

Implements against:
- `docs/spec/00-overview.md`
- `docs/emulator/02-emulation-loop.md`

Scope:
- Add the master-cycle counter and event queue.
- Define the scheduler interface used by the core runtime.
- Keep timing rooted in the 14.31818 MHz master clock.

Done when:
- Scheduled events execute in cycle order deterministically.
- CPU cycle accounting stays monotonic through scheduler activity.

Completion summary:
- Implemented against `docs/spec/00-overview.md`,
  `docs/emulator/02-emulation-loop.md`, and `docs/emulator/milestones/03.md`.
- Added a deterministic scheduler in `src/core/scheduler.*` with master-cycle
  event ordering, frame-timing constants rooted in the 14.31818 MHz clock, and
  monotonic CPU T-state accounting at `master ÷ 2`.
- Expanded `src/core/emulator.*` from a no-op shell into the milestone-3
  headless runtime model that owns frame counters, event logs, and scheduler
  population.
- Verified with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`,
  `cmake --build build`, and `ctest --test-dir build --output-on-failure`.

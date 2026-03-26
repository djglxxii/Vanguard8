# M03-T02 — Add Blanking and /VCLK Event Timing

Status: `completed`
Milestone: `3`
Depends on: `M03-T01`

Implements against:
- `docs/spec/00-overview.md`
- `docs/spec/03-audio.md`
- `docs/emulator/02-emulation-loop.md`

Scope:
- Schedule H-blank, V-blank, V-blank end, and MSM5205 `/VCLK`.
- Expose stable assertion timing for INT0 and INT1 sources.
- Record deterministic event positions for tests.

Done when:
- Repeated runs produce identical event timing.
- Tests cover the documented cycle positions for the scheduled events.

Completion summary:
- Implemented against `docs/spec/00-overview.md`, `docs/spec/03-audio.md`,
  `docs/emulator/02-emulation-loop.md`, and `docs/emulator/milestones/03.md`.
- Added scheduled H-blank, V-blank, V-blank-end, and MSM5205 `/VCLK` events to
  the runtime, with deterministic timestamps recorded in the event log for
  repeated-run verification.
- Locked milestone-3 timing coverage in `tests/test_scheduler.cpp`, including
  documented H-blank/V-blank positions, stable repeated-run timing, and
  monotonic CPU accounting through scheduled events.
- Verified with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`,
  `cmake --build build`, and `ctest --test-dir build --output-on-failure`.

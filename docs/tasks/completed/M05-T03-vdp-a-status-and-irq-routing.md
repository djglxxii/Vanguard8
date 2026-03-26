# M05-T03 — Implement VDP-A Status Semantics and IRQ Routing

Status: `completed`
Milestone: `5`
Depends on: `M05-T02`

Implements against:
- `docs/spec/00-overview.md`
- `docs/spec/02-video.md`
- `docs/spec/01-cpu.md`

Scope:
- Implement VDP-A V-blank and H-blank CPU-visible status behavior.
- Clear `S#0` and `S#1` flags on the documented reads.
- Keep VDP-B interrupt output isolated from the CPU.

Done when:
- VDP-A IRQs reach INT0 correctly.
- Tests prove that VDP-B never asserts a CPU interrupt.

Completion summary:
- Implemented against `docs/spec/00-overview.md`, `docs/spec/02-video.md`,
  `docs/spec/01-cpu.md`, `docs/emulator/04-video.md`, and
  `docs/emulator/milestones/05.md`.
- Added VDP-visible `S#0`/`S#1` flag semantics in `src/core/video/v9938.*`:
  V-blank and H-blank assertion, documented clear-on-read behavior, and
  `V9938::int_pending()`/`vblank_irq_pending()`/`hblank_irq_pending()` to model
  the VDP `/INT` output.
- Attached that interrupt surface into the real milestone-5 runtime path by
  routing VDP-A/VDP-B ports through `src/core/bus.*` and driving VDP timing
  from `src/core/emulator.*`; only VDP-A is forwarded onto CPU `INT0`, while
  VDP-B remains isolated exactly as the spec requires.
- Added status-register and interrupt-semantic tests in `tests/test_video.cpp`,
  `tests/test_bus.cpp`, `tests/test_cpu.cpp`, and `tests/test_scheduler.cpp`
  covering `S#0` clear, `S#1` clear, VDP-B isolation, CPU-visible `INT0`
  service, and scheduler-driven VDP-A interrupt assertion.
- Verified with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`,
  `cmake --build build`, and `ctest --test-dir build --output-on-failure`.

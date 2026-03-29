# M14-T04 — Execute CPU Instructions Inside the Scheduled Frame Loop for ROM Bring-Up

Status: `completed`
Milestone: `14`
Depends on: `M14-T03`

Implements against:
- `docs/spec/00-overview.md`
- `docs/spec/01-cpu.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/11-pre-gui-audit.md`
- `docs/emulator/milestones/14.md`

Scope:
- Replace the current time-only frame-loop CPU advancement with instruction-
  stepped execution inside the scheduled event loop.
- Keep audio, VDP command advancement, and interrupt timing aligned with the
  existing master-cycle scheduler.
- Add the narrow regression coverage needed to prove a loaded ROM can visibly
  affect emulated video state under `run_frames()`.

Done when:
- `Emulator::run_frames()` executes ROM instructions between event boundaries
  instead of only advancing aggregate T-state counters.
- A regression test proves a minimal ROM can change CPU-observable or
  VDP-observable state through the normal frame loop.
- Desktop/headless ROM bring-up no longer depends on the fixture path to
  present a distinct ROM-driven state.

Completion summary:
- Completed against `docs/spec/01-cpu.md` and `docs/spec/02-video.md` by
  splitting scheduled frame execution into two coordinated paths: wall-clock
  T-state advancement remains derived from master cycles, while a separate
  persisted execution budget now steps full HD64180 instructions between event
  boundaries.
- Added the narrow extracted-opcode support required for ROM bring-up (`JR`,
  `XOR A`, and `OUT (n),A`) plus debugger decode coverage, and extended CPU and
  integration tests to prove the normal frame loop can drive VDP-visible state.
- Preserved deterministic restore behavior by versioning save states to include
  the new execution-budget counter.
- Verification completed with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`,
  `cmake --build build`, and `ctest --test-dir build --output-on-failure`.
- Post-fix sanity check:
  `python3 showcase/tools/package/build_showcase.py` and
  `python3 showcase/tools/package/run_showcase_headless.py --frames 1 --hash-frame 1`
  now produce a ROM-driven frame hash distinct from the no-ROM fixture path.

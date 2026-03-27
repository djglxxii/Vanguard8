# M10-T03 — Add Headless Regression CI

Status: `completed`
Milestone: `10`
Depends on: `M10-T02`

Implements against:
- `docs/emulator/01-build.md`
- `docs/emulator/07-implementation-plan.md`

Scope:
- Add headless frame/audio hashing.
- Add replay-backed `ctest` integration.
- Keep the regression path deterministic and scriptable.

Done when:
- CI can run replay assets and compare stable hashes.
- Regression failures are visible through normal test execution.

Completion summary:
- Extended `vanguard8_headless` with replay-file loading, ROM-hash validation,
  frame SHA-256 calculation, audio SHA-256 calculation, and expected-hash exit
  behavior so regression failures surface through normal CLI and `ctest`
  status codes.
- Exposed the mixed stereo output byte stream from the core audio mixer so the
  headless frontend can hash real generated audio rather than a synthetic
  summary digest.
- Added committed replay fixture assets under `tests/replays/` and registered
  a replay-backed `ctest` entry that runs the headless binary against known
  frame/audio hashes under a deterministic config path.

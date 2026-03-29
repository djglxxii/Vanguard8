# M14-T05 — Reject Unexpected Positional CLI Arguments in the Desktop Frontend

Status: `completed`
Milestone: `14`
Depends on: `M14-T04`

Implements against:
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/11-pre-gui-audit.md`
- `docs/emulator/milestones/14.md`

Scope:
- Make `vanguard8_frontend` fail fast when a user supplies unsupported
  positional arguments instead of silently falling back to fixture mode.
- Keep the accepted `--rom` and related documented options unchanged.
- Add the narrow regression coverage needed to pin the CLI behavior.

Done when:
- `vanguard8_frontend build/showcase/showcase.rom` exits with a clear usage
  error instead of opening fixture mode.
- `vanguard8_frontend --rom build/showcase/showcase.rom` still works.
- Tests cover the parser behavior.

Completion summary:
- Completed by making the desktop frontend CLI reject unknown options,
  missing option values, and unexpected positional arguments instead of
  silently falling back to fixture mode.
- Exposed the frontend parser through `src/frontend/app.hpp` and added
  regression coverage proving `vanguard8_frontend build/showcase/showcase.rom`
  throws a clear usage error while `--rom build/showcase/showcase.rom` still
  parses successfully.
- Narrow UX follow-up: frontend argument errors now stay terminal-only and
  print usage, rather than attempting to open a GUI error dialog for a CLI
  misuse case.
- Manual sanity check:
  `./build/src/vanguard8_frontend build/showcase/showcase.rom` now exits with
  code `2` and prints `Use --rom <path> to launch a ROM.` instead of opening
  fixture mode.
- Verification completed with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`,
  `cmake --build build`, and `ctest --test-dir build --output-on-failure`.

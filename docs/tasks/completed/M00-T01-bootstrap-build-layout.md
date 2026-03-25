# M00-T01 — Bootstrap Build Layout and Dependency Manifests

Status: `completed`
Milestone: `0`
Depends on: `none`

Implements against:
- `docs/emulator/01-build.md`
- `docs/emulator/07-implementation-plan.md`

Scope:
- Add the top-level `CMakeLists.txt`.
- Add `vcpkg.json` and `vcpkg-configuration.json`.
- Create the documented `src/`, `tests/`, `third_party/`, and `shaders/`
  layout with stub `CMakeLists.txt` files.

Done when:
- The repository configures with the documented vcpkg toolchain path.
- Target names for `vanguard8_core`, `vanguard8_frontend`, and
  `vanguard8_headless` exist in the build graph.
- No emulator behavior beyond no-op scaffolding is introduced.

Completion summary:
- Implemented against `docs/emulator/01-build.md` and
  `docs/emulator/07-implementation-plan.md`.
- Added the top-level CMake entry, vcpkg manifest/config files, the documented
  `src/`, `tests/`, `third_party/`, and `shaders/` tree, and stub CMake files
  for the milestone 0 target layout.

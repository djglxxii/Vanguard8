# M02-T01 — Integrate Z180 Adapter and Reset Defaults

Status: `completed`
Milestone: `2`
Depends on: `M01-T03`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/emulator/03-cpu-and-bus.md`

Scope:
- Bring in the Z180 core through the emulator adapter.
- Connect memory and I/O callbacks through the `Bus`.
- Implement documented reset-time CPU defaults.

Done when:
- The CPU core can fetch and execute through the emulator bus.
- Reset behavior matches the documented HD64180 defaults.

Completion summary:
- Implemented against `docs/spec/01-cpu.md`,
  `docs/emulator/03-cpu-and-bus.md`, and `docs/emulator/milestones/02.md`.
- Added a standalone milestone-2 Z180 extraction in `third_party/z180/`
  derived from MAME and wired `src/core/cpu/z180_adapter.cpp` to it through
  bus callbacks.
- Reset behavior now comes from the extracted core with Vanguard 8
  spec-preserving defaults for `PC=0x0000`, `CBAR=0xFF`, `CBR=0x00`, and
  `BBR=0x00`.
- Verified with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`,
  `cmake --build build`, and `ctest --test-dir build --output-on-failure`.

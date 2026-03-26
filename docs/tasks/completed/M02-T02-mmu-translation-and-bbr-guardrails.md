# M02-T02 — Implement MMU Translation and Illegal BBR Guardrails

Status: `completed`
Milestone: `2`
Depends on: `M02-T01`

Implements against:
- `docs/spec/00-overview.md`
- `docs/spec/01-cpu.md`
- `docs/emulator/03-cpu-and-bus.md`

Scope:
- Implement translation through `CBAR`, `CBR`, and `BBR`.
- Handle `OUT0` writes to the internal MMU registers.
- Warn when `BBR >= 0xF0`.

Done when:
- MMU translation matches the Vanguard 8 boot mapping.
- Tests lock the illegal `BBR` warning path.

Completion summary:
- Implemented against `docs/spec/00-overview.md`, `docs/spec/01-cpu.md`,
  `docs/emulator/03-cpu-and-bus.md`, and `docs/emulator/milestones/02.md`.
- Moved MMU translation and internal `OUT0` register handling into the
  extracted `third_party/z180` core and kept the Vanguard 8 flat-reset boot
  mapping required by the repo spec.
- Illegal `BBR >= 0xF0` writes now warn from the extracted core while still
  applying the register value, so the bank window aliases into SRAM exactly as
  the spec requires.
- Verified with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`,
  `cmake --build build`, and `ctest --test-dir build --output-on-failure`.

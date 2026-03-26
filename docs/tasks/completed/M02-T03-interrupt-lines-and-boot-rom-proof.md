# M02-T03 — Wire Interrupt Lines and Prove the Boot ROM MMU Sequence

Status: `completed`
Milestone: `2`
Depends on: `M02-T02`

Implements against:
- `docs/spec/00-overview.md`
- `docs/spec/01-cpu.md`
- `docs/emulator/03-cpu-and-bus.md`

Scope:
- Wire INT0 and INT1 into the CPU adapter.
- Lock INT0 IM1 behavior and INT1 vectored behavior.
- Prove boot ROM reconfiguration to `CBAR=0x48`, `CBR=0xF0`, `BBR=0x04`.

Done when:
- A test ROM can boot, switch banks, and read/write SRAM correctly.
- Interrupt-path tests cover INT0 and INT1 semantics independently.

Completion summary:
- Implemented against `docs/spec/00-overview.md`, `docs/spec/01-cpu.md`,
  `docs/emulator/03-cpu-and-bus.md`, and `docs/emulator/milestones/02.md`.
- INT0 IM1 service and INT1 vectored service now execute through the extracted
  core, with INT1 acknowledging through the bus callback and following the
  repo's documented `IL & 0xF8` vector-pointer rule.
- The boot test ROM now runs through the extracted milestone-2 opcode surface
  in `third_party/z180/`, proving MMU reconfiguration to `CBAR=0x48`,
  `CBR=0xF0`, and `BBR=0x04` before bank switching and SRAM access.
- Verified with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`,
  `cmake --build build`, and `ctest --test-dir build --output-on-failure`.

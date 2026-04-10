# M17-T01 — Fix Full-Range VDP CPU VRAM Addressing for 64 KB Compatibility

Status: `completed`
Milestone: `17`
Depends on: `M16-T01`

Implements against:
- `docs/spec/02-video.md`
- `docs/spec/04-io.md`
- `docs/emulator/04-video.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/08-compatibility-audit.md`
- `docs/emulator/milestones/17.md`

Scope:
- Correct the CPU-visible VDP VRAM address path so writes and reads above
  `0x3FFF` reach the intended 64 KB VRAM locations.
- Add direct and runtime regressions that expose the current wrap-around bug.
- Keep the change narrow to the existing covered VDP read/write surface.

Done when:
- High-address VRAM writes and reads are deterministic and do not alias into
  low VRAM.
- The completion summary explains the runtime impact on the mixed-mode HUD
  compatibility path.

Completion summary:
- Re-read `docs/spec/02-video.md`, `docs/spec/04-io.md`,
  `docs/emulator/04-video.md`, `docs/emulator/07-implementation-plan.md`,
  `docs/emulator/08-compatibility-audit.md`, and the milestone-17 contract
  before implementation, then kept the change inside the existing VDP
  data/control-port surface.
- Updated `src/core/video/v9938.*` so CPU-visible VRAM addressing combines the
  latched low 14 address bits with `R#14` high bits, and data-port
  auto-increment now advances the full CPU-visible VRAM pointer instead of
  wrapping at `0x3FFF`.
- Kept the address register state coherent by synchronizing `R#14` with the
  internal port-visible VRAM pointer, including sequential writes that cross
  `0x3FFF -> 0x4000`.
- Added direct regressions in `tests/test_video.cpp` for high-address CPU
  reads, high-address CPU writes, and the boundary-crossing auto-increment
  case.
- Added a runtime integration ROM in `tests/test_integration.cpp` that programs
  `VDP-A` into `Graphic 6`, writes a lower-screen HUD byte through the CPU I/O
  path at VRAM address `0x6400`, and proves the composed frame shows the
  intended late-line overlay pixel without low-window aliasing.
- Updated `docs/spec/02-video.md` and `docs/emulator/04-video.md` to document
  the now-covered `R#14` high-bit role in CPU-visible VRAM port access, so the
  spec and emulator docs match the verified implementation.
- Verified on the final tree with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`
  `cmake --build build`
  `ctest --test-dir build --output-on-failure`

# M04-T01 — Add V9938 Ports, VRAM, and Palette Storage

Status: `completed`
Milestone: `4`
Depends on: `M03-T03`

Implements against:
- `docs/spec/02-video.md`
- `docs/emulator/04-video.md`

Scope:
- Implement the single-VDP port interface.
- Add VRAM storage and palette registers/state.
- Keep rendering limited to the single-VDP path.

Done when:
- VDP port writes mutate VRAM and palette state correctly.
- Tests cover port-visible state changes.

Completion summary:
- Implemented against `docs/spec/02-video.md`,
  `docs/emulator/04-video.md`, and `docs/emulator/milestones/04.md`.
- Added a milestone-4 single-VDP `V9938` model in `src/core/video/v9938.*`
  covering port-visible VRAM, status, register, and palette state for the
  documented Graphic 4 bring-up path.
- Locked port and palette behavior with `tests/test_video.cpp`, including VRAM
  mutation through the data port and palette decoding against the spec's 3-bit
  channel format.
- Verified with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`,
  `cmake --build build`, and `ctest --test-dir build --output-on-failure`.

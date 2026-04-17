# M24-T01 — Cover Missing Timed HD64180 VDP-B VRAM Seek LD A,B Opcode for PacManV8

Status: `completed`
Milestone: `24`
Depends on: `M23-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/spec/02-video.md`
- `docs/spec/04-io.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/24.md`
- `docs/emulator/15-second-pass-audit.md`

Scope:
- Reproduce the PacManV8 VDP-B VRAM seek `LD A,B` blocker reported in
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T007-vdp-b-maze-render-and-pellet-display.md`
  after the milestone-23 `LD A,C` patch.
- Add only the timed opcode coverage needed for base opcode `0x78`
  (`LD A,B`) at `PC 0x023E` on the VDP-B VRAM seek path.
- Add regressions that pin the direct instruction behavior (A loaded from
  B, PC advanced by one, 4 T-states, flags untouched) and the ROM-driven
  timed path without broadening into a general `LD r,r'` table or full
  opcode completion.

Blocker reference:
- Binary: `/home/djglxxii/src/Vanguard8/build/src/vanguard8_headless`
- Command:
  ```bash
  python3 tools/build.py
  /home/djglxxii/src/Vanguard8/build/src/vanguard8_headless \
    --rom build/pacman.rom \
    --frames 60 \
    --dump-frame tests/evidence/T007-vdp-b-maze-render/frame_060.ppm \
    --hash-frame 60
  ```
- Observed abort: `Unsupported timed Z180 opcode 0x78 at PC 0x23E`

Done when:
- The evidence-backed PacManV8 runtime frame-dump path no longer throws on
  the blocked `PC 0x023E`.
- Tests lock `LD A,B` copy behavior (A <- B, PC advance by one, 4 T-states,
  flag preservation) in the timed extracted CPU path.
- A runtime/integration case exercises the previously failing `PC 0x023E` and
  proves the frame loop survives past it.
- Remaining timed-opcode gaps stay explicit and documented; no broader CPU
  completion or generic `LD r,r'` surface is attempted.

Verification:
```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
ctest --test-dir build --output-on-failure
```

Completion summary:
- Re-read `docs/spec/01-cpu.md`, `docs/spec/02-video.md`, `docs/spec/04-io.md`,
  `docs/emulator/03-cpu-and-bus.md`, `docs/emulator/07-implementation-plan.md`,
  `docs/emulator/15-second-pass-audit.md`, and the milestone-24 contract
  before implementation, then kept the change inside the extracted HD64180
  runtime, tests, and workflow docs.
- Extended the extracted z180 core with `op_ld_a_b` and registered it at
  `opcodes_[0x78]` (declaration in `third_party/z180/z180_core.hpp`,
  implementation in `third_party/z180/z180_core.cpp` as
  `af_.bytes.hi = bc_.bytes.hi`), so the timed runtime no longer hits
  `op_unimplemented` for `LD A,B`.
- Added `case 0x78` to the 4-T-state group in
  `src/core/cpu/z180_adapter.cpp::current_instruction_tstates()` alongside
  `NOP`, `DEC B`, `RRCA`, `LD A,C`, `XOR A`, `DI`, and `EI`, matching the
  documented `LD r,r'` timing.
- Added focused scheduled-path CPU coverage in `tests/test_cpu.cpp` that
  pre-seeds A, B/C, and all AF flag bits, asserts `A == B`, BC unchanged,
  `pc == 0x0001`, 4 T-states, and flag preservation.
- Added runtime integration coverage in `tests/test_integration.cpp`
  (`make_pacmanv8_vdp_b_vram_seek_ld_a_b_blocker_rom`) that jumps to the
  evidence-backed failing `PC 0x023E`, loads BC via the M22-covered
  `LD BC,nn`, executes `LD A,B`, HALTs, and proves the frame loop no longer
  aborts on `0x78` at `0x023E`.
- Kept the milestone strictly narrow: only the `LD A,B` opcode was added, no
  broader `LD r,r'` sweep, no VDP-B, palette, or compositor changes. Any
  later opcode gaps surfaced by running the full PacManV8 ROM are explicitly
  out of M24 scope and must be addressed by a future narrow compatibility
  milestone.
- Verified on the final tree with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`
  `cmake --build build`
  `ctest --test-dir build --output-on-failure`
  All 130 tests pass (128 prior + the 2 added by this task).

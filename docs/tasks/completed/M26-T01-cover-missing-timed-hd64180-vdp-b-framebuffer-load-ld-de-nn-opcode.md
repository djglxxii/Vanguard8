# M26-T01 — Cover Missing Timed HD64180 VDP-B Framebuffer Load LD DE,nn Opcode for PacManV8

Status: `completed`
Milestone: `26`
Depends on: `M25-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/spec/02-video.md`
- `docs/spec/04-io.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/26.md`
- `docs/emulator/15-second-pass-audit.md`

Scope:
- Reproduce the PacManV8 VDP-B framebuffer-load `LD DE,nn` blocker reported
  in
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T007-vdp-b-maze-render-and-pellet-display.md`
  after the milestone-25 `OR n` patch.
- Add only the timed opcode coverage needed for base opcode `0x11`
  (`LD DE,nn`) at `PC 0x020B` on the VDP-B framebuffer-load setup path.
- Add regressions that pin the direct instruction behavior (DE loaded from
  the two-byte little-endian immediate, PC advanced by three, 10 T-states,
  flags untouched) and the ROM-driven timed path without broadening into
  general opcode completion.

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
- Observed abort: `Unsupported timed Z180 opcode 0x11 at PC 0x20B`

Done when:
- The evidence-backed PacManV8 runtime frame-dump path no longer throws on
  the blocked `PC 0x020B`.
- Tests lock `LD DE,nn` immediate-load behavior (DE, PC advance by three,
  10 T-states, flag preservation) in the timed extracted CPU path.
- A runtime/integration case exercises the previously failing `PC 0x020B`
  and proves the frame loop survives past it.
- Remaining timed-opcode gaps stay explicit and documented; no broader CPU
  completion or generic `LD rr,nn` sweep is attempted.

Verification:
```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
ctest --test-dir build --output-on-failure
```

Completion summary:
- Re-read `docs/spec/01-cpu.md`, `docs/spec/02-video.md`, `docs/spec/04-io.md`,
  `docs/emulator/03-cpu-and-bus.md`, `docs/emulator/07-implementation-plan.md`,
  `docs/emulator/15-second-pass-audit.md`, and the milestone-26 contract
  before implementation, then kept the change inside the extracted HD64180
  runtime, tests, and workflow docs.
- Extended the extracted z180 core with `op_ld_de_nn` and registered it at
  `opcodes_[0x11]` (declaration in `third_party/z180/z180_core.hpp`,
  implementation in `third_party/z180/z180_core.cpp` mirroring the existing
  `op_ld_bc_nn` / `op_ld_hl_nn` / `op_ld_sp_nn` 16-bit immediate-load
  helpers: `de_.value = fetch_word();`).
- Added `case 0x11` to the 10-T-state group in
  `src/core/cpu/z180_adapter.cpp::current_instruction_tstates()` alongside
  `LD BC,nn` (`0x01`), `LD HL,nn` (`0x21`), `LD HL,(nn)` (`0x2A`),
  `LD SP,nn` (`0x31`), and `JP nn` (`0xC3`), matching the documented
  immediate 16-bit load timing.
- Added focused scheduled-path CPU coverage in `tests/test_cpu.cpp`
  (`scheduled CPU covers LD DE,nn used by the PacManV8 VDP-B framebuffer-load
  path`) that pins the documented semantics: DE loaded from the two-byte
  little-endian immediate, 10 T-states, PC advances by three, and all
  flags are preserved.
- Added runtime integration coverage in `tests/test_integration.cpp`
  (`make_pacmanv8_vdp_b_framebuffer_load_ld_de_nn_blocker_rom`) that jumps
  into the framebuffer-load setup via `JP 0x020B`, executes
  `LD DE,0x4000` at the evidence-backed failing `PC 0x020B`, HALTs, and
  proves the frame loop no longer aborts on `0x11` at `0x020B`.
- Kept the milestone strictly narrow: only the `LD DE,nn` opcode was added,
  no broader `LD rr,nn` sweep beyond what earlier milestones already
  covered, and no VDP-B, palette, or compositor changes. Any later opcode
  gaps surfaced by running the full PacManV8 ROM are explicitly out of M26
  scope and must be addressed by a future narrow compatibility milestone.
- Verified on the final tree with:
  `cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`
  `cmake --build build`
  `ctest --test-dir build --output-on-failure`
  All 134 tests pass (132 prior + the 2 added by this task).

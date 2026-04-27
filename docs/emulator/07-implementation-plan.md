# Vanguard 8 Emulator — Incremental Implementation Plan

## Purpose

This plan breaks emulator development into ordered milestones that can be
completed, tested, and stabilized incrementally. It is based on the repository
specification in `docs/spec/` and the emulator design docs in `docs/emulator/`.

Traceability:
- Hardware contract: `docs/spec/00-overview.md` through `docs/spec/04-io.md`
- Emulator architecture: `docs/emulator/00-overview.md` through `docs/emulator/06-debugger.md`
- Desktop GUI gap audit: `docs/emulator/10-desktop-gui-audit.md`
- Pre-GUI compatibility audit: `docs/emulator/11-pre-gui-audit.md`

## Planning Rules

- The spec remains authoritative. If code and spec disagree, update the spec or
  stop the implementation.
- Every milestone must leave the repo in a testable state.
- Prefer headless verification first, then SDL/OpenGL/ImGui integration.
- Land the minimum documented behavior before optional features.
- Do not implement currently unspecified areas without a spec update.
- Milestone 11 closed the core emulator baseline, not the live desktop GUI
  target described in older frontend/debugger docs. Milestone 12 and later
  reopen that work explicitly.
- Post-milestone-11 work is intentionally prioritized toward the shortest path
  to a usable desktop ROM bring-up: visible window, live video, live audio,
  minimal input, and lightweight debug/status output before broader polish.

## Explicit Deferrals

These areas should stay out of scope until the spec is expanded or verified:

- Horizontal scroll behavior via V9938 `R#26`/`R#27`
- Per-line horizontal parallax built on unverified scroll behavior
- 4-player expansion hardware
- Battery-backed cartridge SRAM banking details
- Any behavior that depends on unverified V9938 text-mode compositing details

## Milestone Control Files

Use the following files to keep implementation work locked to a single
milestone at a time:

- `docs/emulator/current-milestone.md`
  Declares the only milestone currently allowed to be implemented.
- `docs/emulator/milestones/NN.md`
  Per-milestone contract files with allowed scope, explicit non-goals, allowed
  paths, required tests, exit criteria, forbidden extras, and verification
  commands.
- `docs/process/milestone-acceptance-checklist.md`
  Human review checklist used before a milestone is accepted.
- Milestone contract `Verification Commands`
  These are the authoritative verification steps for the active milestone until
  a shared verifier script is added.

Workflow:

1. Lock work to the milestone named in `docs/emulator/current-milestone.md`.
2. Implement only the deliverables and tests described by that milestone
   contract.
3. Mark the milestone `ready_for_verification` only after the declared scope is
   implemented and the milestone's verification commands pass.
4. Mark the milestone `accepted` only after the milestone's verification
   commands have been rerun and the acceptance checklist is satisfied.
5. Advance the lock to the next milestone only after acceptance.

## Milestone Summary

| Milestone | Goal |
|-----------|------|
| 0 | Freeze project scaffolding and test harness |
| 1 | Bring up memory, cartridge, and bus routing |
| 2 | Integrate CPU core and boot-time MMU behavior |
| 3 | Establish the event scheduler and frame loop |
| 4 | Render the first visible frame in single-VDP Graphic 4 |
| 5 | Add dual-VDP compositing, sprites, and VDP interrupt behavior |
| 6 | Add controller input, ROM loading, and usable frontend flow |
| 7 | Integrate audio chips and INT1-driven ADPCM timing |
| 8 | Fill in HD64180 DMA/timers and VDP command execution |
| 9 | Add debugger foundation and core developer panels |
| 10 | Add save states, rewind, replay, and headless regression tooling |
| 11 | Expand format coverage, compatibility audits, and polish to 1.0 |
| 12 | Deliver a playable desktop frontend baseline for ROM bring-up |
| 13 | Close critical pre-GUI audit items that block real ROM testing |
| 14 | Add lightweight bring-up tooling and selected non-essential closure |
| 15 | Patch the timed HD64180 ROM path for INT1-driven audio bring-up |
| 16 | Add Graphic 6 renderer and mixed-mode HUD compatibility |
| 17 | Fix full-range VDP CPU VRAM addressing compatibility |
| 18 | Make headless runtime frame dumps match compositor output |
| 19 | Cover the timed HD64180 boot opcode gap exposed by Pac-Man |
| 20 | Cover the timed HD64180 palette VCLK opcode gap exposed by Pac-Man |
| 21 | Cover the timed HD64180 boot opcode gap exposed by PacManV8 |
| 22 | Cover the timed HD64180 VDP-B bring-up opcode gap exposed by PacManV8 |
| 23 | Cover the timed HD64180 VDP-B VRAM seek opcode gap exposed by PacManV8 |
| 24 | Cover the timed HD64180 VDP-B VRAM seek LD A,B gap exposed by PacManV8 |
| 25 | Cover the timed HD64180 VDP-B VRAM seek OR n gap exposed by PacManV8 |
| 26 | Cover the timed HD64180 VDP-B framebuffer-load LD DE,nn gap exposed by PacManV8 |
| 27 | Cover the timed HD64180 VDP-B framebuffer copy LD A,D gap exposed by PacManV8 |
| 28 | Cover the timed HD64180 VDP-B framebuffer copy OR E gap exposed by PacManV8 |
| 32 | Frontend live audio playback for interactive verification |
| 33 | Fix instruction-granular audio timing for YM2151 busy polls |
| 34 | Cover the timed HD64180 intermission opcode gaps exposed by PacManV8 T020 |
| 35 | Add headless agent observability primitives for ROM verification |
| 36 | Fix HD64180 HALT resume after interrupt |
| 37 | Cover the timed HD64180 `LD E,n` opcode gap exposed by PacManV8 T020 |
| 38 | Cover the timed HD64180 `RET NC` opcode gap exposed by PacManV8 T020 |
| 39 | Cover timed HD64180 pair INC/DEC and narrow CB-prefix gaps exposed by PacManV8 T021 |
| 40 | Cover timed HD64180 `EX DE,HL` and `OR (HL)` gaps exposed by PacManV8 T021 |
| 41 | Cover timed HD64180 CB-prefix `SRL H` and `RR L` gaps exposed by PacManV8 T021 |
| 42 | Cover timed HD64180 `SCF` gap exposed by PacManV8 T021 |
| 43 | Fix the logical peek-row prefix mismatch exposed by PacManV8 T021 |
| 44 | Finish the remaining Vanguard8-side PacManV8 T021 blockers in one milestone |
| 45 | Cover timed HD64180 `AND r` / `AND (HL)` gaps exposed by PacManV8 T021 |
| 46 | Cover timed HD64180 ALU register/immediate tail (`ADC`/`SUB`/`SBC`/`XOR`) gaps exposed by PacManV8 T021 |
| 47 | Import the full MAME HD64180 / Z180 core and retire the per-opcode CPU milestones |
| 48 | Re-pin the PacManV8 T017 300-frame audio digest after the M47 timing shift |

## Milestones

### Milestone 0 — Project Skeleton and Verification Harness

Objective:
- Create the CMake/vcpkg layout, target structure, base source tree, and test
  tree described in `docs/emulator/01-build.md`.

Deliverables:
- Top-level `CMakeLists.txt`
- `src/`, `tests/`, `third_party/`, and `shaders/` directory structure
- `vanguard8_core`, `vanguard8_frontend`, and `vanguard8_headless` targets
- Catch2 test binary wired into `ctest`
- Logging, config loading, and a no-op emulator shell

Exit criteria:
- A clean checkout configures and builds
- Tests run in CI even if most are still placeholders
- Headless binary can start, print version/build info, and exit cleanly

### Milestone 1 — Memory Map, Cartridge, SRAM, and Bus

Objective:
- Implement the fixed hardware memory map and external I/O routing before CPU
  execution enters the picture.

Deliverables:
- `Bus` with physical memory routing
- `CartridgeSlot` for fixed ROM and 59 switchable 16 KB banks
- 32 KB system SRAM model
- Open-bus handling and warning logging for unmapped memory/ports
- Stubbed peripheral endpoints for all documented I/O ports

Tests:
- Fixed ROM region maps `0x00000–0x03FFF`
- Banked ROM region maps `0x04000–0xEFFFF`
- SRAM maps `0xF0000–0xF7FFF`
- Unmapped memory and I/O return `0xFF`

Exit criteria:
- Bus routing matches `docs/spec/00-overview.md`
- Cartridge size validation enforces the 960 KB limit

### Milestone 2 — CPU Core, MMU, Interrupt Lines, and Boot Sequence

Objective:
- Get the MAME Z180 core integrated through an adapter and prove the emulator
  can execute boot ROM code against the real Vanguard 8 MMU layout.

Deliverables:
- Z180 adapter connected to bus memory/I/O callbacks
- Reset behavior matching documented HD64180 defaults
- MMU translation through `CBAR`, `CBR`, and `BBR`
- INT0 and INT1 line plumbing
- Illegal `BBR >= 0xF0` warning behavior

Tests:
- MMU boot mapping and bank-window translation
- `OUT0` writes to internal MMU registers affect memory translation correctly
- INT0 follows IM1 behavior
- INT1 uses the HD64180 vectored `I`/`IL` path, independent of IM mode
- Boot ROM can reconfigure `CBAR=0x48`, `CBR=0xF0`, `BBR=0x04`

Exit criteria:
- A small test ROM can boot, switch ROM banks, and read/write SRAM correctly

Note:
- Before calling this milestone complete, audit the required HD64180-specific
  behaviors called out in `docs/emulator/03-cpu-and-bus.md`: `IN0`, `OUT0`,
  `MLT`, and INT1/INT2 vector handling.
- The milestone-2 extraction may be narrower than a full device-framework port
  as long as the implemented opcode surface is documented and it satisfies the
  milestone tests and exit criteria.

### Milestone 3 — Master-Cycle Scheduler and Main Emulation Loop

Objective:
- Establish the event-driven runtime model that all remaining subsystems depend
  on.

Deliverables:
- Master-cycle counter
- Event scheduler for H-blank, V-blank, V-blank end, and MSM5205 `/VCLK`
- `run_cpu_and_audio_until()` loop shape
- Frame accounting and scanline accounting
- Pause, run, and single-frame stepping in headless form

Tests:
- Scheduled H-blank and V-blank events occur at deterministic cycle positions
- INT0/INT1 assertion timing is stable across repeated runs
- CPU cycle accounting remains monotonic through event boundaries

Exit criteria:
- A headless test can run for N frames deterministically with stable event logs

### Milestone 4 — First Video Bring-Up: Single VDP, Graphic 4, No Compositing Yet

Objective:
- Get pixels on screen with the simplest documented useful rendering path:
  one V9938 in Graphic 4, 256x212, palette-based output.

Deliverables:
- V9938 port interface
- VRAM and palette storage
- Graphic 4 scanline renderer
- Vertical scroll via `R#23`
- SDL2 window and OpenGL upload path
- Headless frame dump for regression images

Tests:
- VRAM writes through VDP ports affect rendered output
- Palette writes decode to correct RGB values
- Graphic 4 framebuffer addressing is correct for multiple scanlines
- `R#23` vertical scroll wraps correctly

Exit criteria:
- A test ROM or fixture pattern renders the expected 256x212 frame identically
  in headless and SDL/OpenGL modes

Out of scope:
- Horizontal scroll via `R#26`/`R#27`
- Full command engine
- Mixed-mode support

### Milestone 5 — Dual VDPs, Compositing, Sprites, and VDP-A Interrupt Semantics

Objective:
- Reach the first Vanguard 8-specific video milestone: two synchronized VDPs
  composited with VDP-A transparency and correct CPU-visible interrupt behavior.

Deliverables:
- Second V9938 instance
- 74LS257-style compositor
- Sprite Mode 2 path needed by Graphic 4 / Graphic 3 usage
- VDP-A V-blank and H-blank flag behavior
- VDP-B interrupt isolation
- Layer toggles usable by tests and later debugger UI

Tests:
- VDP-A color index 0 transparency reveals VDP-B
- Nonzero VDP-A pixels override VDP-B
- VDP-B never asserts CPU interrupts
- Reading VDP-A `S#0` clears V-blank
- Reading VDP-A `S#1` clears H-blank
- Sprite overflow and collision flags behave for covered cases

Exit criteria:
- Emulator can display a composited dual-layer frame and service VDP-A IRQs
  through INT0 without false VDP-B interrupts

### Milestone 6 — Input, ROM Workflow, and Minimal User-Facing Frontend

Objective:
- Make the emulator practically usable for loading ROMs and interacting with
  software, while keeping functionality tightly scoped.

Deliverables:
- SDL2 keyboard and gamepad mapping to controller ports `0x00` and `0x01`
- Active-low controller decoding
- Config file creation/loading
- ROM open flow, drag-and-drop, and recent ROM list
- Fullscreen toggle, scale modes, and frame pacing

Tests:
- Controller reads are active low
- Player 1 and Player 2 ports stay independent
- ROM loading rejects oversized images and invalid formats cleanly

Exit criteria:
- A user can load a ROM, get video output, and control software through either
  keyboard or gamepad

### Milestone 7 — Audio Chips, Mixer, and Dedicated ADPCM Interrupt Flow

Objective:
- Add the complete documented audio surface with correct timing relationships
  to the CPU and frame loop.

Deliverables:
- YM2151 integration via Nuked-OPM
- AY-3-8910 integration via Ayumi
- MSM5205 integration and port control
- Stereo mixer and resampling pipeline
- YM2151 IRQ contribution to INT0
- MSM5205 `/VCLK` contribution to INT1

Tests:
- YM2151 status/busy handling
- AY register latch/read/write behavior
- MSM5205 control register changes update `/VCLK` cadence correctly
- INT1 fires at 4/6/8 kHz-equivalent master-cycle intervals
- Audio output stays deterministic in headless hash runs

Exit criteria:
- Audio is audible, synchronized, and does not break determinism in headless
  regression mode

### Milestone 8 — HD64180 DMA/Timers and VDP Command Engine

Objective:
- Move from basic software execution to the hardware acceleration and internal
  peripheral behavior games will depend on.

Deliverables:
- DMA channel 0 memory-to-memory path
- DMA channel 1 memory-to-I/O path
- HD64180 timer/counter exposure and validation
- V9938 command engine execution with `S#2.CE` timing
- Implemented command set listed in `docs/emulator/04-video.md`

Tests:
- DMA copies follow documented source/destination rules
- DMA to I/O reuses the same port dispatch path as CPU `OUT`
- Timer behavior is stable enough for interrupt-driven code paths
- VDP command completion timing clears `CE` at the right cycle boundary
- CPU polling of `CE` sees in-progress versus complete states correctly

Exit criteria:
- ROMs can use DMA, timers, and VDP blits without falling back to emulator-only
  shortcuts

### Milestone 9 — Debugger Foundation

Objective:
- Add the developer tooling needed to diagnose timing, mapping, and rendering
  bugs before broader software compatibility work begins.

Deliverables:
- Dear ImGui shell and docking layout
- CPU panel with registers, disassembly, and execution controls
- Memory panel with SRAM/ROM/VRAM views
- VDP panel with register and palette inspection
- Interrupt log and bank tracker
- Basic breakpoints and watchpoints

Tests:
- Debugger can attach without mutating emulation state unexpectedly
- Step/run/pause actions stop at correct CPU or scheduler boundaries
- Breakpoints trigger deterministically

Exit criteria:
- Core emulator issues can be diagnosed without adding ad hoc logging to the
  runtime

### Milestone 10 — Save States, Rewind, Replay, and Headless CI

Objective:
- Lock in reproducibility and regression protection so later compatibility work
  is safe to iterate on.

Deliverables:
- Versioned save-state format
- Quicksave/quickload slots
- Rewind ring buffer
- Deterministic input recording/replay
- `vanguard8_headless` verification mode with frame/audio hashing
- Replay-based `ctest` integration

Tests:
- Save/load round-trips preserve CPU, MMU, VRAM, palettes, audio state, and
  scheduler position
- Replay runs produce stable frame and audio hashes
- Rewind restores prior states without desynchronizing audio/video timing

Exit criteria:
- CI can catch regressions with deterministic replay assets

### Milestone 11 — Coverage Expansion, Audit Closure, and 1.0 Readiness

Objective:
- Finish the documented emulator surface, close explicit assumptions, and reach
  a releasable baseline.

Deliverables:
- Additional V9938 modes beyond initial Graphic 4 bring-up
- Mixed-mode rendering validation where the spec is already sufficient
- Remaining debugger panels
- Performance tuning and correctness fixes from real ROM testing
- Compatibility audit notes for the Z180/HD64180 assumption
- Documentation updates for any newly clarified behavior

Tests:
- Mode-specific rendering tests for each newly implemented VDP mode
- Cross-subsystem integration tests covering bank switching, interrupts, audio,
  and compositing together
- Stress tests for long-running determinism and save/replay compatibility

Exit criteria:
- The emulator covers the documented Vanguard 8 hardware surface except for
  explicitly deferred spec gaps
- Remaining limitations are documented, narrow, and intentional

### Milestone 12 — Playable Desktop Frontend Baseline

Objective:
- Turn the current terminal-only frontend launcher into a minimal but usable
  desktop application that can launch a ROM, present live video, play live
  audio, accept basic input, and expose simple bring-up/debug status.

Deliverables:
- Persistent SDL frontend runtime loop
- SDL window creation and lifecycle management
- OpenGL context creation and swap/present path
- Display backend that uploads composited `256x212` frames to a real texture
- Backend-neutral readback/test seam so desktop output can still be checked
  against headless output deterministically
- SDL audio device fed from the existing mixer/resampler path
- Command-line ROM launch into the live desktop session
- Minimal SDL keyboard/gamepad handling for covered controller input, quit, and
  the narrow runtime controls needed for bring-up
- Basic runtime status/error/debug output suitable for ROM bring-up without a
  live Dear ImGui debugger

Tests:
- Frontend runtime lifecycle tests using fake window/display/audio hosts
- Presentation/readback tests that prove a known frame can be uploaded and read
  back through the desktop path
- Audio device queue/ring buffer tests covering startup, shutdown, and covered
  underrun handling
- ROM startup and backend/ROM failure handling tests
- Minimal SDL event mapping tests through frontend host abstractions

Exit criteria:
- Launching `vanguard8_frontend --rom <path>` opens a visible window, keeps
  running until quit, displays live ROM-driven frames, and produces audible
  audio
- Covered input events reach the controller path without regressing headless
  determinism
- Basic runtime/debug output is available without requiring the full desktop
  debugger

### Milestone 13 — Critical Audit Closure for Real ROM Compatibility

Objective:
- Close the smallest set of pre-GUI audit gaps that are most likely to block
  real ROM and showcase-ROM testing after the playable desktop frontend exists.

Deliverables:
- V9938 VRAM read-ahead latch
- VDP screen-disable behavior via `R#1` bit 6
- 16x16 sprite size and sprite magnification
- Register-relative sprite table addressing via `R#5`, `R#6`, and `R#11`
- Sprite Mode 1
- Graphic 1 and Graphic 2 renderers
- Targeted HD64180 audit coverage for `MLT`, `TST`, and broader covered
  `IN0`/`OUT0` behavior
- ROM-driven or representative fixture-driven compatibility tests for the
  covered bring-up cases

Tests:
- VRAM read-ahead coverage that proves the first read after address set matches
  the documented latch behavior
- Screen-disable coverage that locks blanked output to the backdrop color
- Sprite tests covering 16x16 size, magnification, and register-relative table
  bases
- Mode-specific Graphic 1/2 tests with Sprite Mode 1 coverage
- HD64180 instruction tests for the newly audited covered cases

Exit criteria:
- The critical pre-GUI audit items most likely to block first-wave ROM testing
  are implemented and regression-covered
- Remaining compatibility gaps are either lower priority or explicitly
  spec-gated

### Milestone 14 — Lightweight Bring-Up Tooling and Selected Closure

Objective:
- Add only the smaller non-essential items that materially help ROM bring-up
  and debugging, while leaving full heavy debugger/UI work out of scope unless
  it is explicitly promoted later.

Deliverables:
- Trace-to-file support over the existing trace surface
- Symbol file loading and basic annotation support
- Lightweight desktop/runtime status improvements only where they help bring-up
- Selected additional renderer or tooling coverage only where the written spec
  is already sufficient and a target ROM actually needs the feature
- Documentation closure for the remaining intentional deferrals and dropped
  non-goal items

Milestone-14 closure rule:
- If ROM validation does not prove that a lower-priority renderer or tooling
  gap is required, close the milestone with the documentation-backed deferral
  ledger rather than speculative implementation.

Tests:
- Trace file writer coverage
- Symbol load/lookup/annotation coverage
- Mode-specific tests for any additional renderer added in this milestone
- Documentation-backed regression notes for the remaining deferred items

Exit criteria:
- A developer can run a target ROM, see/hear it in the desktop app, and get
  lightweight diagnostic output without needing a full live debugger UI
- Remaining omissions are narrow, deliberate, and documented

### Milestone 15 — Timed HD64180 INT1 Audio Bring-Up Patch

Objective:
- Close the narrow timed-CPU compatibility gap that blocks the first real
  ROM-driven audio scene: vectored `INT1` dispatch during scheduled execution
  plus the minimal additional opcode surface needed for documented YM2151 busy
  polling and MSM5205 nibble-feed state handling.

Deliverables:
- Reproducing regression coverage for the timed `INT1` real-ROM failure path
- A fix for the timed extracted-Z180 vectored `INT1` execution path when ROM
- driven code enables the documented audio flow
- The smallest additional timed opcode coverage proven necessary by the
  failing ROM-driven audio scene
- Documentation tying the fix back to the blocked showcase milestone-5 task
  without implementing the showcase scene inside the emulator milestone

Milestone-15 closure rule:
- Keep this as a compatibility patch milestone, not a general “complete the
  CPU” effort. Add only the interrupt/path corrections and opcode coverage that
  real ROM bring-up proves necessary.

Tests:
- Timed CPU regression coverage reproducing the pre-fix `INT1` vectored failure
- Normal emulator-runtime coverage proving ROM execution survives the first
  audio-window transition without falling into the vector-table pointer
- Focused opcode tests for any newly supported instructions

Exit criteria:
- The emulator no longer aborts on the blocked milestone-5 ROM audio bring-up
  path because of timed `INT1` dispatch or the newly required covered opcode
  surface
- The showcase milestone-5 blocker can be cleared to resume ROM-side audio
  work, with any remaining limitations narrow and documented

### Milestone 16 — Graphic 6 Renderer and Mixed-Mode HUD Compatibility

Objective:
- Add the narrow V9938 `Graphic 6` (`Screen 7`, `512x212`, `4bpp`) rendering
  path needed for a real mixed-mode showcase scene: a fixed high-resolution HUD
  on one VDP composited over a vertically scrolling `Graphic 4` background on
  the other.

Deliverables:
- A `Graphic 6` scanline renderer driven by the documented mode bits and pixel
  packing in `docs/spec/02-video.md`
- Regression coverage for `Graphic 6` framebuffer addressing and palette output
- Mixed-mode runtime coverage proving `VDP-A Graphic 6` compositing over
  `VDP-B Graphic 4` without silent mode fallback
- The smallest display/upload seam needed to expose a `512x212` composed frame
  through the existing runtime and headless readback paths
- Documentation that narrows the supported `Graphic 6` surface to the needs of
  the follow-on showcase milestone

Milestone-16 closure rule:
- Keep this as a renderer-compatibility milestone, not a general
  high-resolution V9938 completion pass. Add only the `Graphic 6` background
  path and the narrow mixed-mode coverage required by the planned showcase HUD
  scene.

Tests:
- Dedicated `Graphic 6` pixel-addressing and palette rendering tests
- A mixed-mode compositor regression for `VDP-A Graphic 6` over
  `VDP-B Graphic 4`
- Display upload/readback coverage for `512x212` composed frames
- Headless/runtime coverage proving unsupported `Graphic 6` use no longer falls
  back to backdrop output for the planned scene shape

Exit criteria:
- The emulator can render a documented `Graphic 6` framebuffer deterministically
  with the correct `512x212` nibble packing
- A ROM can place a fixed `Graphic 6` HUD layer over a scrolling `Graphic 4`
  background without new emulator-side shortcuts
- Remaining `Graphic 5`/`Graphic 7` and mode-specific command-engine gaps stay
  explicitly deferred

### Milestone 17 — Full-Range VDP CPU VRAM Addressing Compatibility

Objective:
- Complete the covered V9938 CPU-side VRAM addressing path across the full
  64 KB VRAM space so high-address `Graphic 4`/`Graphic 6` content written
  through the VDP data/control ports no longer wraps into the low address
  window.

Deliverables:
- Full-range CPU VRAM address handling for the existing VDP data/control-port
  path, including the already covered read/write behavior needed by current
  ROMs
- Focused regression coverage for CPU-visible VRAM access above `0x3FFF`
- A runtime compatibility check proving mixed-mode HUD content that spans the
  later `Graphic 6` address range is stable and does not alias into the top of
  VRAM
- Documentation that narrows the supported 64 KB CPU VRAM-addressing surface
  without broadening into new VDP mode work

Milestone-17 closure rule:
- Keep this as a VDP CPU-addressing compatibility pass, not a general V9938
  reimplementation. Add only the high-address VRAM behavior and tests needed by
  the currently planned ROM content.

Tests:
- Direct VDP CPU write coverage proving addresses below and above `0x3FFF`
  land in distinct VRAM locations
- Direct VDP CPU read coverage for the same high-address path, including the
  existing dummy-read semantics already covered by the repo
- Runtime/integration coverage proving the late mixed-mode HUD scene no longer
  loses lower-screen `Graphic 6` content because of CPU-visible VRAM wrapping

Exit criteria:
- CPU-driven VRAM writes can reach the full documented 64 KB address space
  without aliasing the upper region into low VRAM
- Covered ROM/runtime paths that depend on high-address `Graphic 6` content
  render deterministically through the existing compositor
- Remaining unimplemented VDP features stay explicitly deferred rather than
  being bundled into the same milestone

### Milestone 18 — Headless Runtime Frame-Dump Parity

Objective:
- Make the headless frame-dump path reflect actual runtime output so desktop
  and headless inspection of mixed-width scenes matches the compositor state
  being hashed and tested.

Deliverables:
- A headless runtime frame-dump path that captures the current composed ROM
  frame after the requested frame count instead of the built-in fixture image
- Regression coverage proving dumped frame dimensions and pixels match the
  runtime compositor output for both `256x212` and `512x212` cases
- Documentation for the supported dump workflow and any preserved fixture-only
  path if one remains necessary

Milestone-18 closure rule:
- Keep this as a runtime-inspection correctness milestone. Do not broaden it
  into general frontend refactoring, new capture formats, or debugger feature
  work.

Tests:
- Headless regression coverage proving `--dump-frame` with a ROM and frame
  count emits the actual runtime frame rather than the fixture
- Coverage proving the dumped PPM dimensions match the composed frame width for
  both standard and mixed-width scenes
- A narrow parity check between the dumped frame bytes and the compositor frame
  used for hashing in the same runtime

Exit criteria:
- Headless frame dumps from a running ROM are trustworthy inspection artifacts
- Mixed-width scenes dump at `512x212` when the compositor resolves to the
  wider grid
- Any preserved fixture-dump behavior is explicit and no longer confused with
  runtime capture

### Milestone 19 — Timed HD64180 Boot Opcode Compatibility

Objective:
- Close the next narrow timed-CPU compatibility gap exposed by the Pac-Man
  boot/background bring-up: the extracted timed HD64180 runtime is still
  missing `LD B,n` (`0x06`) and `IM 1` (`ED 0x56`).

Deliverables:
- Reproducing regression coverage for the Pac-Man boot/background blocker
  captured under
  `/home/djglxxii/src/pacman/vanguard8_port/tests/evidence/T002-boot-background/`
- Timed opcode coverage for `LD B,n` and `IM 1` in the existing extracted
  HD64180 runtime path
- Focused direct CPU tests that pin the documented semantics of the newly
  covered instructions without broadening into full opcode completion
- Documentation tying the fix to the observed ROM bring-up PCs `0x0140` and
  `0x0314`

Milestone-19 closure rule:
- Keep this as a compatibility patch milestone, not a general “finish the Z80”
  effort. Add only the timed opcode support and the narrow interrupt-mode
  handling proven necessary by the blocked boot/background path.

Tests:
- Focused timed CPU coverage for `LD B,n`
- Focused timed CPU coverage for `IM 1`, including the already documented
  split between `INT0` mode handling and mode-independent `INT1`
- Runtime/integration coverage proving the blocked Pac-Man boot/background path
  no longer aborts on the observed missing-opcode PCs

Exit criteria:
- The emulator no longer aborts on the Pac-Man boot/background path because the
  timed core is missing `0x06` or `ED 0x56`
- The newly supported opcode surface is documented and regression-covered
- Remaining CPU opcode gaps stay narrow and explicitly deferred

### Milestone 20 — Timed HD64180 Palette VCLK Compatibility

Objective:
- Close the next narrow timed-CPU compatibility gap exposed by the Pac-Man
  palette swatch bring-up: the runtime succeeds with `VCLK: off` but the
  `VCLK 4000` path still aborts on missing timed opcode `0x05` at `PC 0x02B9`.

Deliverables:
- Reproducing regression coverage for the Pac-Man palette evidence captured
  under
  `/home/djglxxii/src/pacman/vanguard8_port/tests/evidence/T004-palette/`
- Timed opcode coverage for the proven missing base opcode `0x05` in the
  existing extracted HD64180 runtime path
- Focused direct CPU tests that pin the documented semantics of the newly
  covered opcode without broadening into general Z80 completion
- Runtime/integration coverage proving the palette swatch path survives with
  active `VCLK 4000` timing rather than only in the `VCLK: off` case

Milestone-20 closure rule:
- Keep this as a compatibility patch milestone, not a broad palette or VDP
  rewrite. Add only the timed opcode support and runtime coverage proven
  necessary by the `T004-palette` evidence.

Tests:
- Focused timed CPU coverage for opcode `0x05`
- Runtime/integration coverage proving the Pac-Man palette swatch path no
  longer aborts when `VCLK 4000` is enabled
- Coverage proving the passing `VCLK: off` palette path remains stable

Exit criteria:
- The emulator no longer aborts on the evidence-backed Pac-Man palette path
  because the timed core is missing opcode `0x05`
- The newly supported timed opcode surface is documented and regression-covered
- Remaining opcode gaps stay narrow and explicitly deferred

### Milestone 21 — Timed HD64180 Boot Opcode Compatibility (PacManV8)

Objective:
- Close the next narrow timed-CPU compatibility gap exposed by the PacManV8
  minimal boot ROM: the extracted timed HD64180 runtime aborts on `LD C,n`
  (`0x0E`) and `LD D,n` (`0x16`) during the boot/init path at `PC 0x012A`.

Deliverables:
- Reproducing regression coverage for the PacManV8 boot blocker reported in
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T001-build-system-and-minimal-boot.md`
- Timed opcode coverage for `LD C,n` and `LD D,n` in the existing extracted
  HD64180 runtime path
- Focused direct CPU tests that pin the documented semantics of the newly
  covered instructions without broadening into full opcode completion
- Documentation tying the fix to the observed ROM bring-up PC `0x012A`

Milestone-21 closure rule:
- Keep this as a compatibility patch milestone, not a general "finish the Z80"
  effort. Add only the timed opcode support proven necessary by the blocked
  PacManV8 boot path.

Tests:
- Focused timed CPU coverage for `LD C,n` (`0x0E`)
- Focused timed CPU coverage for `LD D,n` (`0x16`)
- Runtime/integration coverage proving the blocked PacManV8 boot path no
  longer aborts on the observed missing-opcode PC

Exit criteria:
- The emulator no longer aborts on the PacManV8 boot path because the timed
  core is missing `0x0E` or `0x16`
- The newly supported opcode surface is documented and regression-covered
- Remaining CPU opcode gaps stay narrow and explicitly deferred

### Milestone 22 — Timed HD64180 VDP-B Bring-Up Opcode Compatibility (PacManV8)

Objective:
- Close the next narrow timed-CPU compatibility gap exposed by the PacManV8
  VDP-B maze render ROM: the extracted timed HD64180 runtime aborts on
  `LD BC,nn` (`0x01`) during the VDP-B bring-up path at `PC 0x01FD`.

Deliverables:
- Reproducing regression coverage for the PacManV8 VDP-B bring-up blocker
  reported in
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T007-vdp-b-maze-render-and-pellet-display.md`
- Timed opcode coverage for `LD BC,nn` (`0x01`) in the existing extracted
  HD64180 runtime path
- A focused direct CPU test that pins the documented 16-bit immediate load
  semantics without broadening into full opcode completion
- Documentation tying the fix to the observed PacManV8 VDP-B bring-up
  `PC 0x01FD`

Milestone-22 closure rule:
- Keep this as a compatibility patch milestone, not a general "finish the Z80"
  effort. Add only the timed opcode support proven necessary by the blocked
  PacManV8 VDP-B bring-up path.

Tests:
- Focused timed CPU coverage for `LD BC,nn` (`0x01`)
- Runtime/integration coverage proving the blocked PacManV8 VDP-B bring-up
  path no longer aborts on the observed missing-opcode PC

Exit criteria:
- The emulator no longer aborts on the PacManV8 VDP-B bring-up path because
  the timed core is missing `0x01`
- The newly supported opcode surface is documented and regression-covered
- Remaining CPU opcode gaps stay narrow and explicitly deferred

### Milestone 23 — Timed HD64180 VDP-B VRAM Seek Opcode Compatibility (PacManV8)

Objective:
- Close the next narrow timed-CPU compatibility gap exposed by the PacManV8
  VDP-B maze render ROM: after the milestone-22 patch, the extracted timed
  HD64180 runtime now aborts on `LD A,C` (`0x79`) in the VDP-B VRAM seek
  helper at `PC 0x023B`.

Deliverables:
- Reproducing regression coverage for the PacManV8 VDP-B VRAM seek blocker
  reported in
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T007-vdp-b-maze-render-and-pellet-display.md`
- Timed opcode coverage for `LD A,C` (`0x79`) in the existing extracted
  HD64180 runtime path
- A focused direct CPU test that pins the documented `LD r,r'` copy semantics
  (A <- C, 4 T-states, flags untouched) without broadening into a full
  `LD r,r'` sweep
- Documentation tying the fix to the observed PacManV8 VDP-B VRAM seek
  `PC 0x023B`

Milestone-23 closure rule:
- Keep this as a compatibility patch milestone, not a general "finish the Z80"
  effort. Add only the timed opcode support proven necessary by the blocked
  PacManV8 VDP-B VRAM seek path.

Tests:
- Focused timed CPU coverage for `LD A,C` (`0x79`)
- Runtime/integration coverage proving the blocked PacManV8 VDP-B VRAM seek
  path no longer aborts on the observed missing-opcode PC

Exit criteria:
- The emulator no longer aborts on the PacManV8 VDP-B VRAM seek path because
  the timed core is missing `0x79`
- The newly supported opcode surface is documented and regression-covered
- Remaining CPU opcode gaps stay narrow and explicitly deferred

### Milestone 24 — Timed HD64180 VDP-B VRAM Seek LD A,B Compatibility (PacManV8)

Objective:
- Close the next narrow timed-CPU compatibility gap exposed by the PacManV8
  VDP-B maze render ROM: after the milestone-23 `LD A,C` patch, the extracted
  timed HD64180 runtime now aborts on `LD A,B` (`0x78`) in the same VDP-B
  VRAM seek helper at `PC 0x023E`.

Deliverables:
- Reproducing regression coverage for the PacManV8 VDP-B VRAM seek `LD A,B`
  blocker reported in
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T007-vdp-b-maze-render-and-pellet-display.md`
- Timed opcode coverage for `LD A,B` (`0x78`) in the existing extracted
  HD64180 runtime path
- A focused direct CPU test that pins the documented `LD r,r'` copy semantics
  (A <- B, 4 T-states, flags untouched) without broadening into a full
  `LD r,r'` sweep
- Documentation tying the fix to the observed PacManV8 VDP-B VRAM seek
  `PC 0x023E`

Milestone-24 closure rule:
- Keep this as a compatibility patch milestone, not a general "finish the Z80"
  effort. Add only the timed opcode support proven necessary by the blocked
  PacManV8 VDP-B VRAM seek path.

Tests:
- Focused timed CPU coverage for `LD A,B` (`0x78`)
- Runtime/integration coverage proving the blocked PacManV8 VDP-B VRAM seek
  path no longer aborts on the observed missing-opcode PC

Exit criteria:
- The emulator no longer aborts on the PacManV8 VDP-B VRAM seek path because
  the timed core is missing `0x78`
- The newly supported opcode surface is documented and regression-covered
- Remaining CPU opcode gaps stay narrow and explicitly deferred

### Milestone 25 — Timed HD64180 VDP-B VRAM Seek OR n Compatibility (PacManV8)

Objective:
- Close the next narrow timed-CPU compatibility gap exposed by the PacManV8
  VDP-B maze render ROM: after the milestone-24 `LD A,B` patch, the extracted
  timed HD64180 runtime now aborts on `OR n` (`0xF6`) in the same VDP-B
  VRAM seek helper at `PC 0x0241`.

Deliverables:
- Reproducing regression coverage for the PacManV8 VDP-B VRAM seek `OR n`
  blocker reported in
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T007-vdp-b-maze-render-and-pellet-display.md`
- Timed opcode coverage for `OR n` (`0xF6`) in the existing extracted
  HD64180 runtime path
- A focused direct CPU test that pins the documented `OR n` semantics (A,
  PC advance by two, 7 T-states) and Z80-standard flag result (S, Z, H=0,
  P/V parity, N=0, C=0) without broadening into a general logical-op sweep
- Documentation tying the fix to the observed PacManV8 VDP-B VRAM seek
  `PC 0x0241`

Milestone-25 closure rule:
- Keep this as a compatibility patch milestone, not a general "finish the Z80"
  effort. Add only the timed opcode support proven necessary by the blocked
  PacManV8 VDP-B VRAM seek path.

Tests:
- Focused timed CPU coverage for `OR n` (`0xF6`) including flag semantics
- Runtime/integration coverage proving the blocked PacManV8 VDP-B VRAM seek
  path no longer aborts on the observed missing-opcode PC

Exit criteria:
- The emulator no longer aborts on the PacManV8 VDP-B VRAM seek path because
  the timed core is missing `0xF6`
- The newly supported opcode surface is documented and regression-covered
- Remaining CPU opcode gaps stay narrow and explicitly deferred

### Milestone 26 — Timed HD64180 VDP-B Framebuffer Load LD DE,nn Compatibility (PacManV8)

Objective:
- Close the next narrow timed-CPU compatibility gap exposed by the PacManV8
  VDP-B maze render ROM: after the milestone-25 `OR n` patch, the extracted
  timed HD64180 runtime now aborts on `LD DE,nn` (`0x11`) in the VDP-B
  framebuffer-load setup at `PC 0x020B`.

Deliverables:
- Reproducing regression coverage for the PacManV8 VDP-B framebuffer-load
  `LD DE,nn` blocker reported in
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T007-vdp-b-maze-render-and-pellet-display.md`
- Timed opcode coverage for `LD DE,nn` (`0x11`) in the existing extracted
  HD64180 runtime path
- A focused direct CPU test that pins the documented 16-bit immediate load
  semantics (DE <- two-byte little-endian immediate, PC +3, 10 T-states,
  flags untouched) without broadening into full opcode completion
- Documentation tying the fix to the observed PacManV8 VDP-B framebuffer-load
  `PC 0x020B`

Milestone-26 closure rule:
- Keep this as a compatibility patch milestone, not a general "finish the Z80"
  effort. Add only the timed opcode support proven necessary by the blocked
  PacManV8 VDP-B framebuffer-load path.

Tests:
- Focused timed CPU coverage for `LD DE,nn` (`0x11`)
- Runtime/integration coverage proving the blocked PacManV8 VDP-B
  framebuffer-load path no longer aborts on the observed missing-opcode PC

Exit criteria:
- The emulator no longer aborts on the PacManV8 VDP-B framebuffer-load path
  because the timed core is missing `0x11`
- The newly supported opcode surface is documented and regression-covered
- Remaining CPU opcode gaps stay narrow and explicitly deferred

### Milestone 27 — Timed HD64180 VDP-B Framebuffer Copy LD A,D Compatibility (PacManV8)

Objective:
- Close the next narrow timed-CPU compatibility gap exposed by the PacManV8
  VDP-B maze render ROM: after the milestone-26 `LD DE,nn` patch, the
  extracted timed HD64180 runtime now aborts on `LD A,D` (`0x7A`) in the
  VDP-B framebuffer copy loop at `PC 0x0246`.

Deliverables:
- Reproducing regression coverage for the PacManV8 VDP-B framebuffer copy
  `LD A,D` blocker reported in
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T007-vdp-b-maze-render-and-pellet-display.md`
- Timed opcode coverage for `LD A,D` (`0x7A`) in the existing extracted
  HD64180 runtime path
- A focused direct CPU test that pins the documented register-copy
  semantics (A <- D, PC +1, 4 T-states, flags untouched) without
  broadening into full opcode completion
- Documentation tying the fix to the observed PacManV8 VDP-B framebuffer
  copy `PC 0x0246`

Milestone-27 closure rule:
- Keep this as a compatibility patch milestone, not a general "finish the Z80"
  effort. Add only the timed opcode support proven necessary by the blocked
  PacManV8 VDP-B framebuffer copy path.

Tests:
- Focused timed CPU coverage for `LD A,D` (`0x7A`)
- Runtime/integration coverage proving the blocked PacManV8 VDP-B
  framebuffer copy path no longer aborts on the observed missing-opcode PC

Exit criteria:
- The emulator no longer aborts on the PacManV8 VDP-B framebuffer copy path
  because the timed core is missing `0x7A`
- The newly supported opcode surface is documented and regression-covered
- Remaining CPU opcode gaps stay narrow and explicitly deferred

### Milestone 28 — Timed HD64180 VDP-B Framebuffer Copy OR E Compatibility (PacManV8)

Objective:
- Close the next narrow timed-CPU compatibility gap exposed by the PacManV8
  VDP-B maze render ROM: after the milestone-27 `LD A,D` patch, the
  extracted timed HD64180 runtime now aborts on `OR E` (`0xB3`) in the
  VDP-B framebuffer copy loop at `PC 0x0247`.

Deliverables:
- Reproducing regression coverage for the PacManV8 VDP-B framebuffer copy
  `OR E` blocker reported in
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T007-vdp-b-maze-render-and-pellet-display.md`
- Timed opcode coverage for `OR E` (`0xB3`) in the existing extracted
  HD64180 runtime path
- A focused direct CPU test that pins the documented logical-OR
  semantics (A <- A | E; S from bit 7, Z from zero result, H=0, P/V
  from even parity, N=0, C=0; PC +1, 4 T-states) without broadening into
  full opcode completion
- Documentation tying the fix to the observed PacManV8 VDP-B framebuffer
  copy `PC 0x0247`

Milestone-28 closure rule:
- Keep this as a compatibility patch milestone, not a general "finish the Z80"
  effort. Add only the timed opcode support proven necessary by the blocked
  PacManV8 VDP-B framebuffer copy path.

Tests:
- Focused timed CPU coverage for `OR E` (`0xB3`)
- Runtime/integration coverage proving the blocked PacManV8 VDP-B
  framebuffer copy path no longer aborts on the observed missing-opcode PC

Exit criteria:
- The emulator no longer aborts on the PacManV8 VDP-B framebuffer copy path
  because the timed core is missing `0xB3`
- The newly supported opcode surface is documented and regression-covered
- Remaining CPU opcode gaps stay narrow and explicitly deferred

### Milestone 29 — Timed HD64180 VDP-B Framebuffer Copy RET Z / DEC DE Compatibility (PacManV8)

Objective:
- Close the remaining narrow timed-CPU compatibility gaps designated by
  the PacManV8 VDP-B maze render blocked task after the milestone-28
  `OR E` patch. The blocked T007 static audit identifies exactly two
  remaining missing timed opcodes on the VDP-B framebuffer copy loop
  path:
  - `0xC8` at `PC 0x0248` — `RET Z`
  - `0x1B` at `PC 0x024D` — `DEC DE`
- Implement both together so the documented PacManV8 headless runtime
  frame-dump path can clear the two designated gaps in one pass and
  PacManV8 T007 becomes actionable for runtime frame capture.

Deliverables:
- Reproducing regression coverage for the PacManV8 VDP-B framebuffer
  copy `RET Z` and `DEC DE` blockers reported in
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T007-vdp-b-maze-render-and-pellet-display.md`
- Timed opcode coverage for `RET Z` (`0xC8`) and `DEC DE` (`0x1B`) in
  the existing extracted HD64180 runtime path
- Focused direct CPU tests that pin the documented semantics:
  - `RET Z`: taken path (Z=1) pops PC from SP, SP += 2, 11 T-states;
    not-taken path (Z=0) advances PC by one, 5 T-states; flags
    untouched on both paths
  - `DEC DE`: DE <- DE - 1 with wrap at 0x0000; PC += 1; 6 T-states;
    flags untouched
- Documentation tying the fix to the observed PacManV8 VDP-B framebuffer
  copy `PC 0x0248` and `PC 0x024D`

Milestone-29 closure rule:
- Keep this as a compatibility patch milestone, not a general "finish
  the Z80" effort. Add only the two timed opcodes explicitly designated
  by the T007 blocker's static audit.

Tests:
- Focused timed CPU coverage for `RET Z` (`0xC8`), including both the
  taken and not-taken timing branches
- Focused timed CPU coverage for `DEC DE` (`0x1B`)
- Runtime/integration coverage proving the blocked PacManV8 VDP-B
  framebuffer copy path no longer aborts on the two observed
  missing-opcode PCs

Exit criteria:
- The emulator no longer aborts on the PacManV8 VDP-B framebuffer copy
  path because the timed core is missing `0xC8` or `0x1B`
- The newly supported opcode surface is documented and
  regression-covered
- Remaining CPU opcode gaps stay narrow and explicitly deferred

### Milestone 30 — Timed HD64180 PacManV8 VBlank Handler PUSH/POP rr Compatibility

Objective:
- Close the narrow timed-CPU compatibility gap blocking the PacManV8 T016
  audio bring-up ROM against the current Vanguard8 timed HD64180 runtime.
  The PacManV8 VBlank handler (see
  `/home/djglxxii/src/PacManV8/docs/field-manual/vanguard8-build-directory-skew-and-timed-opcodes.md`)
  was widened to save more register pairs around `audio_update_frame`.
  This exposes six register-pair stack opcodes that the timed path does
  not yet cover:
  - `0xC5` (`PUSH BC`) at `PC 0x0039`
  - `0xD5` (`PUSH DE`) at `PC 0x003A`
  - `0xE5` (`PUSH HL`) at `PC 0x003B`
  - `0xE1` (`POP HL`)  at `PC 0x0040`
  - `0xD1` (`POP DE`)  at `PC 0x0041`
  - `0xC1` (`POP BC`)  at `PC 0x0042`
- Implement all six together so the PacManV8 T016 regression command
  clears the VBlank handler prologue and epilogue in a single pass,
  rather than triggering six separate one-opcode compatibility
  milestones.

Deliverables:
- Reproducing regression coverage for the PacManV8 VBlank handler
  register-pair PUSH/POP blockers reported in
  `/home/djglxxii/src/PacManV8/docs/field-manual/vanguard8-build-directory-skew-and-timed-opcodes.md`
- Timed opcode coverage for `PUSH BC/DE/HL` and `POP BC/DE/HL` in the
  existing extracted HD64180 runtime path
- Focused direct CPU tests that pin the documented semantics:
  - `PUSH rr` (BC/DE/HL): SP <- SP - 2; memory[SP+1] <- high(rr);
    memory[SP] <- low(rr); PC += 1; 11 T-states; flags untouched
  - `POP rr`  (BC/DE/HL): low(rr) <- memory[SP]; high(rr) <- memory[SP+1];
    SP <- SP + 2; PC += 1; 10 T-states; flags untouched
- Runtime/integration coverage exercising a PacManV8-shaped VBlank
  prologue/epilogue around an `audio_update_frame` stand-in
- Documentation tying the fix to the observed PacManV8 VBlank handler
  PCs (`0x0038..0x0044`)

Milestone-30 closure rule:
- Keep this as a compatibility patch milestone, not a general "finish
  the Z80" effort. Add only the six register-pair stack opcodes
  explicitly designated by the PacManV8 field manual. Do not extend
  into IX/IY PUSH/POP forms, `EX (SP),HL`, or `LD SP,rr`.

Tests:
- Focused timed CPU coverage for each of `PUSH BC` (`0xC5`),
  `PUSH DE` (`0xD5`), `PUSH HL` (`0xE5`), `POP BC` (`0xC1`),
  `POP DE` (`0xD1`), and `POP HL` (`0xE1`)
- Regression coverage that `PUSH AF` (`0xF5`) and `POP AF` (`0xF1`)
  remain covered in the timed path after the new opcodes are added
- Runtime/integration coverage proving the blocked PacManV8 VBlank
  handler prologue and epilogue no longer abort on any of the six
  observed missing-opcode PCs

Exit criteria:
- The emulator no longer aborts on the PacManV8 VBlank handler path
  because the timed core is missing any of `0xC5`, `0xD5`, `0xE5`,
  `0xC1`, `0xD1`, or `0xE1`
- The newly supported opcode surface is documented and
  regression-covered
- Remaining CPU opcode gaps stay narrow and explicitly deferred

### Milestone 31 — Timed HD64180 PacManV8 ROM Run-Time Opcode Coverage

Objective:
- After M30 closed the VBlank handler prologue/epilogue, running the full
  PacManV8 `pacman.rom` against the timed HD64180 dispatcher still aborts
  on a series of general-purpose opcodes used by the main program past the
  interrupt boundary. The user has explicitly broadened this milestone
  (deviating from the M19-M30 one-opcode-at-a-time discipline) to close
  all timed-HD64180 opcode gaps discoverable by running the canonical
  `cmake-build-debug/src/vanguard8_headless --rom pacman.rom --frames 3600
  --hash-audio` binary against the current PacManV8 `pacman.rom`.

Deliverables:
- Reproducing regression coverage for the full PacManV8 `pacman.rom`
  against the timed dispatcher for at least 3600 emulated frames without
  a timed-opcode abort
- Timed opcode coverage for the following families in the existing
  extracted HD64180 runtime:
  - `LD r, r'` / `LD r, (HL)` / `LD (HL), r` for `0x40-0x7F` except
    `0x76` (`HALT`)
  - `INC r` / `DEC r` / `INC (HL)` / `DEC (HL)` with correct
    S/Z/H/P/V/N flags and preserved carry
  - `JR Z, e` (`0x28`), `JP NZ, nn` (`0xC2`), `JP Z, nn` (`0xCA`),
    `RET NZ` (`0xC0`)
  - `CP n` (`0xFE`) and `CP r` / `CP (HL)` (`0xB8-0xBF`)
- Focused direct CPU tests pinning the cycle counts and flag semantics
  for each newly added family
- Integration coverage running the full `pacman.rom` against the timed
  dispatcher for at least 3600 frames with stable digests

Milestone-31 closure rule:
- The milestone covers only opcode families surfaced by the iterative
  discovery loop described above. Do not extend into index-register
  (`IX`/`IY`), `CB`-prefix, block-instruction, or `ADD`/`SUB`/`ADC`/`SBC`
  families; any new gap surfaced later belongs to a subsequent narrow
  compatibility milestone.

Tests:
- Representative `LD r, r'` / `LD r, (HL)` / `LD (HL), r` coverage
- `INC r` / `DEC r` coverage pinning half-carry, parity/overflow, and
  carry-preservation on boundary values
- `JR Z, e`, `JP NZ, nn`, `JP Z, nn`, `RET NZ` coverage pinning taken
  / not-taken cycle counts and PC/SP state against the zero flag
- `CP n` / `CP r` / `CP (HL)` coverage pinning that `A` is unchanged
  while flags reflect the subtraction
- Runtime/integration coverage running the full PacManV8 `pacman.rom`
  through the timed dispatcher for at least 3600 frames with stable
  audio SHA-256 and event-log digest across repeat runs

Exit criteria:
- The canonical `cmake-build-debug/src/vanguard8_headless --rom
  /path/to/pacman.rom --frames 3600 --hash-audio` command runs to
  completion without a timed-opcode abort
- The newly supported opcode surface is documented and
  regression-covered
- Any further opcode gaps stay narrow and explicitly deferred

### Milestone 32 — Frontend Live Audio Playback for Interactive Verification

Objective:
- Connect the deterministic core audio byte stream to the desktop frontend's
  SDL queue path so interactive ROM review can be performed by ear. This
  milestone was later blocked because its PacManV8 acceptance guard was proven
  to encode deterministic silence rather than audible output.

Deliverables:
- Frontend SDL audio queue path consuming `AudioMixer::consume_output_bytes()`
  as the only PCM source
- Fake-device and SDL dummy-device coverage for open, queue, back-pressure,
  close, error handling, and reopen behavior
- Frontend/headless PCM parity coverage for per-frame mixer consumption
- Documentation of the shipping GUI audio path

Milestone-32 blocker:
- The current PacManV8 T017 ROM remains in the YM2151 busy-poll helper
  (`audio_ym_write_bc`, `PC 0x2B8B`) through at least 3,600 frames because
  the scheduled CPU loop does not advance audio time between individual
  instructions inside a scheduler event slice.
- The required 300-frame audio digest
  `0d634fb059d15b12c4a8faf2412fbe08b85d187a31b1f22278ce3662f3b44390`
  is the SHA-256 of 959,128 zero bytes. It is not a valid audible-output
  acceptance target.

Exit criteria:
- Blocked pending milestone 33. Do not accept M32 until the core timing
  blocker is fixed and the PacManV8 audible-output evidence is regenerated
  against nonzero PCM.

### Milestone 33 — Instruction-Granular Audio Timing for YM2151 Busy Polls

Objective:
- Fix the core CPU/audio co-scheduling defect exposed by M32. Scheduled CPU
  instruction execution must advance audio hardware time in lockstep closely
  enough that ROM-visible YM2151 busy/status polling ages while code executes.
  PacManV8 must progress past `audio_ym_write_bc` at `PC 0x2B8B` and reach
  nonzero audio/video output.

Deliverables:
- Core emulation-loop/scheduler change that keeps CPU instruction execution,
  audio advancement, VDP command advancement, CPU T-state accounting, and event
  firing on one coherent master-cycle timeline
- Focused YM2151 busy-poll regression proving a ROM-shaped poll loop can
  observe busy clear while executing inside one frame
- Repeatability and sample-count tests proving the timing change remains
  deterministic and does not double-count audio cycles
- PacManV8 blocker regression proving the ROM is no longer stuck at
  `PC 0x2B8B`, produces nonzero PCM, and reaches nonblack video at the selected
  review frame
- Replacement of the invalid M32 all-zero audio digest with post-fix evidence

Milestone-33 closure rule:
- Keep this as a core timing milestone. Do not bundle frontend, debugger UI,
  input, VDP feature work, synthetic audio, recording/export, or broad opcode
  completion. If the corrected timing exposes a new missing opcode, document it
  narrowly and only include it if it is required to reach the M33 exit
  criteria.

Tests:
- YM2151 busy-poll progress under scheduled CPU execution
- Deterministic repeat run of the busy-poll scenario
- Audio sample-count accounting against the master-cycle timeline
- PacManV8 nonzero audio/video regression replacing the old all-zero hash

Exit criteria:
- PacManV8 no longer remains at `PC 0x2B8B` in the review window
- PacManV8 audio bytes are nonzero and the new SHA-256 is recorded
- PacManV8 video is nonblack at the selected review frame, or the exact later
  first-nonblack frame is documented and regression-covered
- `ctest --test-dir cmake-build-debug --output-on-failure` passes

### Milestone 34 — Timed HD64180 PacManV8 Intermission Opcode Coverage

Objective:
- Close the narrow timed-opcode gap that blocked the PacManV8 T020
  intermission owner after M33. The milestone remains CPU-compatibility only:
  it adds the specific opcode families surfaced by the T020 runtime path and
  does not change ROM, VDP, audio, scheduler, or frontend behavior.

Deliverables:
- Timed HD64180 coverage for the observed PacManV8 intermission opcodes
- Focused CPU tests and ROM-shaped runtime regressions for those opcodes
- Documentation tying the fix to the T020 blocker PCs

Exit criteria:
- The canonical headless PacManV8 runtime no longer aborts on the observed
  intermission opcode path
- `ctest --test-dir cmake-build-debug --output-on-failure` passes

### Milestone 35 — Agent Observability Primitives for Headless ROM Verification

Objective:
- Close the headless observability gap that kept PacManV8 T020 from
  distinguishing between "intermission state never reached", "reached but
  crashed before drawing", and "drew but is not visible in the composed
  framebuffer".

Deliverables:
- Headless CLI flags for deterministic inspection: `--inspect`,
  `--inspect-frame`, `--peek-mem`, `--peek-logical`, `--dump-cpu`,
  `--dump-vdp-regs`, `--dump-vram-a`, `--dump-vram-b`, and
  `--run-until-pc`
- A locked text-report format and tests for deterministic report/VRAM output,
  memory peek correctness, non-perturbation of existing hashes/digests, and
  PC probe hit/not-hit outcomes
- Documentation in `docs/emulator/06-debugger.md` describing how agents should
  use the flags for headless ROM failure triage

Closure rule:
- Keep this milestone in `src/frontend/`, `src/headless_main.cpp`, tests, and
  docs only. Do not add new core tracking state, GUI panels, opcode coverage,
  VDP/audio/scheduler behavior, or PacManV8 ROM changes.

Exit criteria:
- `vanguard8_headless --help` lists every observability flag
- Inspection reports and VRAM dumps are deterministic and contain no
  host-specific report body content
- Existing `--hash-frame`, `--hash-audio`, frame dump, fixture dump, replay,
  and trace paths remain behaviorally unchanged with or without inspection
- The PacManV8 smoke command in `docs/emulator/milestones/35.md` produces a
  deterministic report covering CPU, both VDPs, and the requested RAM ranges

### Milestone 36 — Fix HD64180 HALT Resume After Interrupt

Objective:
- Close the runtime CPU defect that re-blocked PacManV8 T020 after M35.
  With M35 observability the blocker pinpointed the cause: when an
  interrupt is accepted while the CPU is halted, the stack-pushed return
  address is the HALT instruction's own address (because `op_halt`
  decrements `pc_` and interrupt servicing pushes `pc_` as-is). After
  `RETI` the CPU re-enters HALT and the foreground frame loop after
  `HALT` never executes.

Deliverables:
- Corrected HALT-resume semantics in the extracted HD64180 runtime:
  interrupt service pushes the address of the instruction *after* `HALT`
  as the return target, across INT0, INT1, PRT0, and PRT1.
- Focused extracted-core CPU tests pinning the HALT-wake contract for
  each interrupt source, plus a repeat-wake regression.
- Non-perturbation regression against an existing non-HALT-wake fixture
  ROM proving frame, audio, and event-log digests are unchanged.
- Runtime regression using only the M35 headless observability surface
  proving the PacManV8 T020 repro now hits `0x0068` and `0x2EE3`.

Closure rule:
- Keep this milestone inside `third_party/z180/`, `src/core/cpu/`,
  `tests/`, and the listed doc/task files only. Do not add opcode
  coverage, interrupt-acceptance rewiring, VDP/audio/scheduler/frontend
  work, M35 CLI changes, or PacManV8 ROM edits.

Exit criteria:
- PacManV8 T020's "What resolution would unblock it" condition is met:
  the canonical headless runtime resumes foreground execution at the
  instruction after `HALT` once the accepted VBlank / IM1 interrupt
  service has completed.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes with
  the new HALT-wake tests included.
- Deterministic digests for ROMs that do not depend on HALT-wake resume
  are unchanged.

### Milestone 37 — Timed HD64180 `LD E,n` Opcode Coverage for PacManV8 T020

Objective:
- Close the next real-ROM timed CPU compatibility gap exposed after M36
  unblocked the HALT-resume path. With foreground game-flow execution
  now resuming every VBlank, the PacManV8 blocked task
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T020-intermission-cutscenes.md`
  now reaches the intermission panel setup path at
  `intermission_fill_panel`, where the canonical headless runtime aborts
  with `Unsupported timed Z180 opcode 0x1E at PC 0x332E`
  (`LD E,n`). The preceding `LD D,n` (`0x16`) at `0x332C` already
  executes, so only the `0x1E` form is missing. Sibling `LD E,n` sites
  on the same active drawing block (`0x330E`, `0x3317`, `0x3321`,
  `0x332E`, `0x3337`, `0x3341`, `0x3355`, `0x335E`, `0x3394`, `0x33A1`,
  `0x33AE`, `0x33BB`) will hit the same handler.

Deliverables:
- Timed-core implementation of `LD E,n` (`0x1E`), plus the remaining
  missing `LD r,n` sister forms on the same immediate-byte family
  (`LD H,n` at `0x26`, `LD L,n` at `0x2E`), closed in one pass to avoid
  a rebuild-per-opcode loop (precedent: milestone 34).
- Adapter T-state timing entries for each new opcode (7 T-states each),
  registered in the existing 7-T-state case group in
  `src/core/cpu/z180_adapter.cpp`.
- Focused extracted-core CPU tests pinning `E`, `H`, `L` destination
  behavior, immediate-byte semantics, PC advancement, flag
  preservation, and a three-opcode sequential test mirroring the
  PacManV8 `LD D,n` → `LD E,n` → `LD A,n` pattern at `0x332C..0x3331`.
- Non-perturbation regression against an existing fixture ROM that
  does not exercise the new opcodes, proving frame, audio, and
  event-log digests are byte-identical before and after.
- Runtime regression proving the canonical PacManV8 T020 repro runs
  past `PC=0x332E` without a timed-opcode abort and produces a stable
  frame hash / CPU snapshot at frame 1020.

Closure rule:
- Keep this milestone inside `third_party/z180/`, `src/core/cpu/`,
  `tests/`, and the listed doc/task files only. No general CPU opcode
  sweep, no `DD`/`FD`/`CB` prefix work, no interrupt-controller,
  scheduler, audio, VDP, or frontend changes, no M35 CLI changes, no
  PacManV8 ROM edits.

Exit criteria:
- The emulator no longer aborts on the documented PacManV8 T020 path
  with `Unsupported timed Z180 opcode 0x1E` at any of the sites listed
  above.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes with
  the new opcode tests included, and no pre-existing CPU test is
  relaxed.
- Deterministic digests for ROMs that do not depend on the new
  opcodes are unchanged.
- PacManV8 T020 becomes actionable again from the emulator side; the
  actual cutscene evidence refresh and acceptance happen in the
  PacManV8 repo.

### Milestone 38 — Timed HD64180 `RET NC` Opcode Coverage for PacManV8 T020

Objective:
- Close the next real-ROM timed CPU compatibility gap exposed after
  M37 unblocked the scene-1 intermission panel setup path. With
  `LD E,n`, `LD H,n`, and `LD L,n` now timed, the PacManV8 blocked
  task
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T020-intermission-cutscenes.md`
  renders the first intermission scene and advances past it. The
  canonical headless runtime then aborts while advancing from scene
  1 to scene 2 with `Unsupported timed Z180 opcode 0xD0 at PC
  0x2F75` (`RET NC`) inside `intermission_advance_review_index`.
  `RET Z` (`0xC8`) and `RET NZ` (`0xC0`) already execute in the
  intermission path, so only the carry-conditional forms are
  missing.

Deliverables:
- Timed-core implementation of `RET NC` (`0xD0`), plus the remaining
  missing carry-conditional sister form `RET C` (`0xD8`), closed in
  one pass to avoid a rebuild-per-opcode loop (precedent:
  milestones 34 and 37). `RET C` is used in other accepted runtime
  modules — `src/ghost_house.asm`, `src/movement.asm`, and
  `src/ghost_ai.asm` — per the T020 blocker's look-ahead.
- Adapter T-state timing entries for each new opcode:
  **11 T-states** for the taken branch, **5 T-states** for the
  not-taken branch, registered in the existing `RET cc` timing
  groups in `src/core/cpu/z180_adapter.cpp` alongside `0xC8`
  (`RET Z`) and `0xC0` (`RET NZ`).
- Focused extracted-core CPU tests pinning taken and not-taken
  semantics for both opcodes (PC/SP behavior, flag preservation,
  cycle classification) and a review-index pattern test mirroring
  the PacManV8 `intermission_advance_review_index` sequence at
  `0x2F70..0x2F7A`.
- Non-perturbation regression against an existing fixture ROM that
  does not exercise the new opcodes, proving frame, audio, and
  event-log digests are byte-identical before and after.
- Runtime regression proving the canonical PacManV8 T020 repro runs
  past `PC=0x2F75` without a timed-opcode abort and produces stable
  frame hashes / CPU snapshots at the scene-2, scene-3, and
  intermission completion frames (`1770`, `2520`, `2640`).

Closure rule:
- Keep this milestone inside `third_party/z180/`, `src/core/cpu/`,
  `tests/`, and the listed doc/task files only. No general CPU
  opcode sweep, no parity (`RET PO` / `RET PE`) or sign (`RET P` /
  `RET M`) conditional returns, no `DD`/`FD`/`CB` prefix work, no
  interrupt-controller, scheduler, audio, VDP, or frontend changes,
  no M35 CLI changes, no PacManV8 ROM edits.

Exit criteria:
- The emulator no longer aborts on the documented PacManV8 T020
  path with `Unsupported timed Z180 opcode 0xD0` (or at any `RET C`
  site in `ghost_house.asm`, `movement.asm`, or `ghost_ai.asm`
  reached by this ROM).
- `ctest --test-dir cmake-build-debug --output-on-failure` passes
  with the new opcode tests included, and no pre-existing CPU test
  is relaxed.
- Deterministic digests for ROMs that do not depend on the new
  opcodes are unchanged.
- PacManV8 T020 becomes actionable past scene 1 from the emulator
  side; the actual cutscene evidence refresh and acceptance happen
  in the PacManV8 repo.

### Milestone 39 — Timed HD64180 16-bit Pair INC/DEC and CB-Prefix Coverage for PacManV8 T021

Objective:
- Close the next real-ROM timed CPU compatibility gap exposed after
  M38 unblocked the carry-conditional `RET cc` path. With the
  M37/M38 timed load and conditional-return surface now landed, the
  PacManV8 blocked task
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`
  reaches its replay-validation entry path and the canonical
  headless runtime aborts with
  `Unsupported timed Z180 opcode 0xB at PC 0x1209` (`DEC BC`)
  inside `collision_init`. The blocker's look-ahead also enumerates
  `INC DE` (`0x13`) in the same loop, `DEC HL` (`0x2B`) in
  `ghost_mode_tick` / score and dot bookkeeping, and the entirely
  missing CB-prefix dispatch surface for `SRL A` (`CB 3F`) and
  `BIT 4..7,A` (`CB 67`/`6F`/`77`/`7F`) used by existing
  movement/collision code and the T021 replay input decoder.

Deliverables:
- Timed-core implementation of `DEC BC` (`0x0B`), plus the look-ahead
  16-bit pair INC/DEC forms `INC DE` (`0x13`) and `DEC HL` (`0x2B`),
  plus the BC-pair sister `INC BC` (`0x03`), closed in one pass to
  avoid a rebuild-per-opcode loop on the same active path (precedent:
  milestones 34, 37, 38).
- Adapter T-state timing entries for each new 16-bit pair INC/DEC
  opcode: **6 T-states**, registered alongside the already-covered
  `0x1B` (`DEC DE`) and `0x23` (`INC HL`) in the existing 6-T-state
  group in `src/core/cpu/z180_adapter.cpp`.
- Narrow CB-prefix dispatch handler registered against
  `opcodes_[0xCB]` in `Core::install_opcode_table()` that fetches the
  next byte and routes only the listed CB sub-opcodes:
  - `SRL A` (`CB 3F`) — shift-right-logical on `A`, bit 0 → C, bit 7
    ← 0, S/Z/PV from result, H and N cleared.
  - `BIT 4,A` (`CB 67`), `BIT 5,A` (`CB 6F`), `BIT 6,A` (`CB 77`),
    `BIT 7,A` (`CB 7F`) — Z set to inverse of tested bit, H set, N
    cleared, S/PV per standard `BIT b,r` semantics for `r=A`, C and
    `A` unchanged.
  Any other CB sub-opcode preserves the existing fail-fast
  `std::runtime_error` contract.
- Adapter T-state entry routing `case 0xCB:` to a new
  `cb_instruction_tstates(subop)` helper that returns **8 T-states**
  for the listed CB register-form sub-opcodes (matching the Z80
  timing convention already used by the adapter for the 16-bit pair
  INC/DEC forms).
- Focused extracted-core CPU tests pinning each new opcode's
  register/PC/flag/cycle behavior, including a `collision_init`-style
  pattern test mirroring the PacManV8 sequence around `PC=0x1209`,
  and explicit negative tests confirming an out-of-scope CB sub-opcode
  (`RES 0,B` (`CB 80`)) and an out-of-scope main-table opcode
  (`INC SP` (`0x33`)) still raise the existing unsupported-opcode
  runtime error.
- Non-perturbation regression against an existing fixture ROM that
  does not exercise the new opcodes, proving frame, audio, and
  event-log digests are byte-identical before and after.
- Runtime regression proving the canonical PacManV8 T021
  `tools/pattern_replay_tests.py` repro runs past `PC=0x1209` without
  a timed-opcode abort and reaches at least the first authored
  replay checkpoint with stable digests across repeat runs.

Closure rule:
- Keep this milestone inside `third_party/z180/`, `src/core/cpu/`,
  `tests/`, and the listed doc/task files only. No general CPU
  opcode sweep, no `INC SP` / `DEC SP`, no broader CB-prefix
  coverage (no `RES`/`SET`, no `BIT b,r` for `r ≠ A`, no `(HL)`
  targets, no rotate/shift forms other than `SRL A`), no
  `DD CB`/`FD CB` indexed CB variants, no `DD`/`FD` prefix work, no
  interrupt-controller, scheduler, audio, VDP, or frontend changes,
  no M35 CLI changes, no PacManV8 ROM edits.

Exit criteria:
- The emulator no longer aborts on the documented PacManV8 T021
  path with `Unsupported timed Z180 opcode 0xB` (or at any
  `INC DE`, `DEC HL`, `INC BC`, `SRL A`, or `BIT 4..7,A` site
  reached by this ROM along the replay validation path).
- `ctest --test-dir cmake-build-debug --output-on-failure` passes
  with the new opcode tests included, and no pre-existing CPU test
  is relaxed.
- Deterministic digests for ROMs that do not depend on the new
  opcodes are unchanged.
- PacManV8 T021 becomes actionable from the emulator side; the
  actual replay-checkpoint evidence and acceptance still happen in
  the PacManV8 repo.

### Milestone 40 — Timed HD64180 EX DE,HL and OR (HL) Coverage for PacManV8 T021

Objective:
- Close the next real-ROM timed CPU compatibility gap exposed after
  M39. The PacManV8 blocked task
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`
  now reports that the T021 replay validation path runs past the M39
  opcode surface and aborts with `Unsupported timed Z180 opcode 0xEB
  at PC 0x11DA` (`EX DE,HL`) inside `collision_init`. The same ROM
  bytes and source context show an immediate follow-on `OR (HL)`
  (`0xB6`) in the same `.set_pellet` path.

Deliverables:
- Timed-core implementation of `EX DE,HL` (`0xEB`): swap the full
  16-bit `DE` and `HL` pairs, leave all flags untouched, advance
  `PC` by 1.
- Adapter T-state timing entry for `EX DE,HL`: **4 T-states**.
- Timed-core implementation of `OR (HL)` (`0xB6`): read the byte at
  logical address `HL`, compute `A <- A | (HL)`, set S/Z/PV from the
  result, clear H/N/C, advance `PC` by 1.
- Adapter T-state timing entry for `OR (HL)`: **7 T-states**,
  matching the existing indirect memory-operand timing convention.
- Focused extracted-core CPU tests pinning both opcodes'
  register/memory/PC/flag/cycle behavior, including a short
  `collision_init`-style sequence around `EX DE,HL -> OR (HL) ->
  LD (HL),A -> EX DE,HL`, plus a negative test proving a nearby
  out-of-scope exchange opcode such as `EX (SP),HL` (`0xE3`) still
  raises the existing unsupported-opcode runtime error.
- Non-perturbation regression against an existing fixture ROM that
  does not exercise the new opcodes, proving frame, audio, and
  event-log digests are byte-identical before and after.
- Runtime regression proving the canonical PacManV8 T021
  `tools/pattern_replay_tests.py` repro runs past `PC=0x11DA` and the
  immediate `OR (HL)` site without a timed-opcode abort, or records a
  later out-of-scope opcode as a new blocker.

Closure rule:
- Keep this milestone inside `third_party/z180/`, `src/core/cpu/`,
  `tests/`, and the listed doc/task files only. No general CPU
  opcode sweep, no broader exchange-family coverage, no broader
  logical/arithmetic coverage, no interrupt-controller, scheduler,
  audio, VDP, frontend, debugger, save-state, observability, or
  PacManV8 ROM changes.

Exit criteria:
- The emulator no longer aborts on the documented PacManV8 T021 path
  with `Unsupported timed Z180 opcode 0xEB at PC 0x11DA` or at the
  immediate `OR (HL)` (`0xB6`) site.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes
  with the new opcode tests included, and no pre-existing CPU test
  is relaxed.
- Deterministic digests for ROMs that do not depend on the new
  opcodes are unchanged.
- PacManV8 T021 becomes actionable from the emulator side; the
  actual replay-checkpoint evidence and acceptance still happen in
  the PacManV8 repo.

### Milestone 41 — Timed HD64180 CB-Prefix `SRL H` and `RR L` Coverage for PacManV8 T021

Objective:
- Close the next real-ROM timed CPU compatibility gap exposed after
  M40. The PacManV8 blocked task
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`
  now reports that the T021 replay validation path runs past the M40
  `EX DE,HL` / `OR (HL)` surface and aborts with `Unsupported timed
  Z180 opcode 0xCB 0x3C at PC 0x1302` (`SRL H`) inside the
  `collision_prepare_tile` divide-by-eight sequence. The same ROM
  bytes and source context show an immediate follow-on `RR L`
  (`0xCB 0x1D`) forming a `srl h -> rr l` pair that repeats three
  times in a row.

Deliverables:
- Timed-core implementation of `SRL H` (`0xCB 0x3C`): shift `H` right
  by one logical bit, bit 7 of new `H` cleared, old bit 0 of `H` →
  carry. Update S (cleared), Z, H (cleared), P/V (parity), N
  (cleared); `PC` advances by 2.
- Timed-core implementation of `RR L` (`0xCB 0x1D`): rotate `L` right
  through carry, new bit 7 = old carry, new carry = old bit 0. Update
  S, Z, H (cleared), P/V (parity), N (cleared); `PC` advances by 2.
- Adapter T-state timing entries for `CB 3C` and `CB 1D`: both
  **8 T-states**, routed through the existing M39
  `cb_instruction_tstates` helper.
- Extension of the existing `Core::op_cb_prefix` dispatch to cover
  the two new sub-opcodes while preserving the current fail-fast
  contract for every still-unsupported CB sub-opcode.
- Focused extracted-core CPU tests pinning both opcodes'
  register/memory/PC/flag/cycle behavior, including a short
  `collision_prepare_tile`-style sequence running `SRL H -> RR L`
  three times to confirm `HL_final == HL_initial >> 3`, plus a
  negative test proving a nearby out-of-scope CB sub-opcode such as
  `SRL L` (`CB 3D`) or `RR H` (`CB 1C`) still raises the existing
  unsupported-opcode runtime error.
- Non-perturbation regression against an existing fixture ROM that
  does not exercise the new opcodes, proving frame, audio, and
  event-log digests are byte-identical before and after.
- Runtime regression proving the canonical PacManV8 T021
  `tools/pattern_replay_tests.py` repro runs past `PC=0x1302` and the
  immediate `RR L` site without a timed-opcode abort, or records a
  later out-of-scope opcode as a new blocker.

Closure rule:
- Keep this milestone inside `third_party/z180/`, `src/core/cpu/`,
  `tests/`, and the listed doc/task files only. No general CPU
  opcode sweep, no broader CB-prefix coverage (no other SRL or RR
  register forms, no `(HL)`-target CB forms, no `RES`/`SET`, no
  additional `BIT b,r`, no index-register CB variants), no
  interrupt-controller, scheduler, audio, VDP, frontend, debugger,
  save-state, observability, or PacManV8 ROM changes.

Exit criteria:
- The emulator no longer aborts on the documented PacManV8 T021 path
  with `Unsupported timed Z180 opcode 0xCB 0x3C at PC 0x1302` or at
  the immediate `RR L` (`0xCB 0x1D`) site.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes
  with the new opcode tests included, and no pre-existing CPU test
  is relaxed.
- Deterministic digests for ROMs that do not depend on the new
  opcodes are unchanged.
- PacManV8 T021 becomes actionable from the emulator side; the
  actual replay-checkpoint evidence and acceptance still happen in
  the PacManV8 repo.

### Milestone 42 — Timed HD64180 `SCF` Coverage for PacManV8 T021

Objective:
- Close the next real-ROM timed CPU compatibility gap exposed after
  M41 added the authorized `SRL H` and `RR L` CB-prefix coverage for
  PacManV8 T021. The PacManV8 T021 replay validation path now runs
  past the M41 CB-prefix surface and aborts with `Unsupported timed
  Z180 opcode 0x37 at PC 0x1315` (`SCF`) immediately after the
  `collision_prepare_tile` divide-by-eight sequence. The idiom is
  `SCF` followed by `RET` — "return with carry set" signaling the
  collision-check outcome.

Deliverables:
- Timed-core implementation of `SCF` (`0x37`): set carry (C = 1),
  clear half-carry (H = 0) and add/subtract (N = 0), leave S/Z/P/V
  and all registers other than F untouched, advance `PC` by 1.
- Adapter T-state timing entry for `SCF`: **4 T-states**.
- Focused extracted-core CPU tests pinning register/flag/PC/cycle
  behavior, including both entry-carry states, explicit S/Z/P/V
  preservation, a register-preservation case across all pairs, a
  short `SCF -> RET` idiom test, plus a negative test proving the
  sister `CCF` (`0x3F`) opcode still raises the existing
  unsupported-opcode runtime error.
- Non-perturbation regression against an existing fixture ROM that
  does not exercise the new opcode, proving frame, audio, and
  event-log digests are byte-identical before and after.
- Runtime regression proving the canonical PacManV8 T021
  `tools/pattern_replay_tests.py` repro runs past `PC=0x1315`
  without a timed-opcode abort, or records a later out-of-scope
  opcode as a new blocker.

Closure rule:
- Keep this milestone inside `third_party/z180/`, `src/core/cpu/`,
  `tests/`, and the listed doc/task files only. No general CPU
  opcode sweep, no broader flag-manipulation coverage (no `CCF`,
  `DAA`, `CPL`), no interrupt-controller, scheduler, audio, VDP,
  frontend, debugger, save-state, observability, or PacManV8 ROM
  changes.

Exit criteria:
- The emulator no longer aborts on the documented PacManV8 T021
  path with `Unsupported timed Z180 opcode 0x37 at PC 0x1315`.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes
  with the new opcode tests included, and no pre-existing CPU test
  is relaxed.
- Deterministic digests for ROMs that do not depend on the new
  opcode are unchanged.
- PacManV8 T021 becomes actionable from the emulator side; the
  actual replay-checkpoint evidence and acceptance still happen in
  the PacManV8 repo.

### Milestone 43 — Logical Peek Row Prefix Format for PacManV8 T021

Status note, 2026-04-23:
- The in-scope Vanguard8 fix is implemented and locally verified:
  logical peek rows now emit `0xHHHH:`, physical peek rows keep
  `0xHHHHH:`, the headless observability golden/assertions are
  refreshed, full `ctest` passes, and a direct PacManV8 smoke run
  shows `logical 0x8270 ...` followed by `  0x8270:`.
- Milestone acceptance remains blocked externally because the
  checked-in PacManV8 harness still defines
  `BYTE_ROW_PATTERN = re.compile(r"^\\s+0x([0-9a-f]{4}):((?: [0-9a-f]{2})+)$")`,
  which matches literal `\s` instead of leading whitespace and
  therefore still raises `inspection report did not contain
  logical 0x8270:13` against the corrected Vanguard8 report.
  Milestone 43 does not authorize PacManV8 edits.

Objective:
- Close the next real-ROM T021 blocker exposed after M42 added
  the authorized `SCF` timed opcode coverage. With SCF running,
  the PacManV8 `pattern_replay_tests.py` harness now reaches the
  inspection-report parse step and fails with `inspection report
  did not contain logical 0x8270:13`. The cause is a width
  mismatch inside Vanguard 8's own inspection output:
  `src/frontend/headless_inspect.cpp::append_byte_row` uses
  `hex20` (5 hex digits) for every row prefix, including on the
  `[peek-logical]` path where logical addresses are 16-bit. The
  harness regex expects 4 hex digits after `0x` and silently
  fails to match. This blocker is not a Z180 opcode gap.

Deliverables:
- Row-prefix width distinction inside
  `src/frontend/headless_inspect.cpp` so the `[peek-logical]`
  path emits `  0xHHHH: ...` (16-bit) while `[peek-mem]` keeps
  `  0xHHHHH: ...` (20-bit). The block header line format is
  unchanged.
- Refreshed `tests/golden/headless_observability_report.txt`
  reflecting the new logical-row width (the physical-row width
  stays as it was).
- Tightened peek-format assertions in `tests/test_headless.cpp`
  requiring both widths explicitly.
- Runtime regression proving the canonical PacManV8 T021
  `tools/pattern_replay_tests.py` command now parses the
  inspection report and either passes outright or records its
  next distinct blocker explicitly.

Closure rule:
- Keep this milestone inside `src/frontend/headless_inspect.*`,
  `tests/`, and the listed doc/task files only. No CPU opcode
  work. No changes to the physical-peek path. No changes to the
  inspection-report header line, VRAM dump format, event log
  format, VDP register dump format, or any other observability
  surface. No PacManV8 harness regex edits.

Exit criteria:
- Logical peek rows emit a 4-digit hex prefix; physical peek
  rows keep their 5-digit hex prefix.
- The headless observability golden comparison passes against
  the refreshed golden file, and the peek-format assertions
  pass.
- `ctest --test-dir cmake-build-debug --output-on-failure`
  passes, and `vanguard8_headless_replay_regression` still
  matches its pinned frame and audio hashes.
- PacManV8 T021 `pattern_replay_tests.py` no longer fails with
  `inspection report did not contain logical 0x8270:13`; any
  remaining T021 failure is a new, distinct blocker and is
  routed to a future milestone.

### Milestone 44 — PacManV8 T021 Remaining Compatibility Closure

Status note, 2026-04-23:
- M39 through M43 each closed one distinct PacManV8 T021 blocker,
  but the latest blocked-task log at
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`
  shows the replay still surfaces additional Vanguard8-side gaps
  after the earlier parser mismatch was cleared on the PacManV8 side.
- The current confirmed Vanguard8 blocker is `Unsupported timed
  Z180 opcode 0x93 at PC 0x0FAE` (`SUB E`) in
  `ghost_update_all_targets`, and the same blocked-task log already
  identifies same-path `SUB C` (`0x91`) sites as the most plausible
  immediate follow-on traps.
- This milestone intentionally replaces the one-blocker-per-milestone
  pattern for the rest of T021. Do not create M45+ micro-milestones
  for each additional opcode or narrow headless/reporting gap unless
  the subsystem scope changes beyond the boundaries below.

Objective:
- Finish the remaining Vanguard8-side compatibility work required for
  PacManV8 T021 to run end-to-end. Use the blocked task above as the
  authoritative runtime blocker ledger and clear the remaining timed
  HD64180 and narrow headless observability/reporting gaps in one
  milestone.

Deliverables:
- Timed extracted-core coverage for every missing opcode directly
  confirmed by the PacManV8 T021 replay path, beginning with
  `SUB E` (`0x93`) and any same-path immediate sister or look-ahead
  forms already evidenced by the blocked-task log.
- Narrow headless replay, inspection, or reporting fixes only when the
  blocked task or a direct repro proves the failure is caused by
  Vanguard8's emitted output or CLI surface rather than PacManV8.
- Focused CPU/headless regression coverage and non-perturbation proof
  for each newly covered behavior.
- A single active task and blocker ledger for the remaining T021 work
  (`M44-T01`) instead of more per-opcode milestones.

Closure rule:
- Keep this milestone inside `third_party/z180/`, `src/core/cpu/`,
  `src/frontend/headless*`, `tests/`, and the listed doc/task files
  only. Do not broaden into a general CPU completion pass, and do not
  edit PacManV8 sources from this repo. Only implement behavior
  directly proven by the T021 replay path or its immediate same-path
  look-ahead.

Exit criteria:
- The canonical PacManV8 T021 `tools/pattern_replay_tests.py`
  command completes end-to-end against the current Vanguard8 tree
  without a Vanguard8-side timed-opcode abort or headless/report-format
  mismatch.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes
  with focused new coverage included, and no existing regression is
  relaxed.
- Deterministic digests for unaffected fixture ROMs remain unchanged
  unless a deliberate change is explicitly recorded.
- Any remaining T021 failure is explicit and proven external to
  Vanguard8 or outside the documented milestone scope, not another
  undiscovered in-scope micro-milestone.

### Milestone 45 — Timed HD64180 `AND r` / `AND (HL)` Coverage for PacManV8 T021

Status note, 2026-04-23:
- M45 completed its in-scope `AND r` / `AND (HL)` timed CPU
  coverage and is blocked on the next distinct out-of-scope T021
  opcode: `Unsupported timed Z180 opcode 0xD6 at PC 0x3EA`
  (`SUB n` in PacManV8 `movement_distance_to_next_center_px`).
  The next change requires a follow-up milestone contract; M45 is not
  broadened beyond the documented `AND` family.
- Milestone 44 cleared the then-current T021 timed-opcode gaps
  (`SUB E`, `SUB C`, `CPL`) and was moved to blocked when the replay
  path exposed a PacManV8-side fidelity issue outside M44's allowed
  Vanguard8 scope. The PacManV8 team has since resolved that fidelity
  issue, and the canonical T021 harness now aborts on a new
  Vanguard8-side timed-opcode gap:

  ```text
  terminate called after throwing an instance of 'std::runtime_error'
    what():  Unsupported timed Z180 opcode 0xA3 at PC 0x125B
  ```

  This is `AND E` (`0xA3`) in `collision_consume_tile`. The
  PacManV8 blocked-task log also identifies `AND (HL)` (`0xA6`) at
  `PC=0x1260` as the strongly suspected immediate follow-on.
- Per M44's philosophy, keep the remaining T021 Vanguard8-side
  closure inside a single milestone per opcode family rather than
  one milestone per opcode. M45 covers the full base `AND r`
  register family plus `AND (HL)` in one pass. If a new out-of-scope
  gap surfaces after M45, open a new milestone contract; do not
  broaden this one.

Objective:
- Close the confirmed `AND E` timed-opcode gap and the suspected
  `AND (HL)` follow-on, and in the same pass cover the rest of the
  base `AND r` register forms so the T021 replay path cannot abort
  again on a sister `AND r` opcode inside the same rebuild cycle.

Deliverables:
- Timed extracted-core coverage for `AND B`/`C`/`D`/`E`/`H`/`L`/`A`
  (`0xA0`–`0xA5`, `0xA7`) and `AND (HL)` (`0xA6`).
- Focused CPU regression tests pinning operand/result, flags (S, Z,
  H=1, P/V, N=0, C=0), `PC` advance, and adapter T-state
  classification for each opcode, including a memory-read exercise
  for `AND (HL)`.
- A negative guard preserving the existing fail-fast contract for a
  still-out-of-scope sister form (for example `OR E` / `0xB3` or
  `XOR E` / `0xAB`).
- Non-perturbation fixture regression proof against an existing
  fixture ROM that does not exercise the newly covered opcodes.
- Task `M45-T01` captures the full opcode list, per-opcode test
  references, verification evidence, and — if a new out-of-scope
  blocker is surfaced — the precise next repro.

Closure rule:
- Keep this milestone inside `third_party/z180/`, `src/core/cpu/`,
  `tests/`, and the listed doc/task files only. Do not broaden into
  `OR r`, `XOR r`, `CP r`, rotate/shift, index-register, or
  ED-prefix forms without a T021 repro. Do not edit PacManV8 sources
  from this repo.

Exit criteria:
- Every opcode in Allowed scope is dispatched by the timed
  extracted core with a matching focused test.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes
  with the new coverage included and no pre-existing regression
  relaxed.
- Deterministic digests for unaffected fixture ROMs remain unchanged
  unless a deliberate change is explicitly recorded.
- The canonical PacManV8 T021 `tools/pattern_replay_tests.py`
  command either completes end-to-end against the current Vanguard8
  tree or stops on a new distinct blocker that is explicitly out of
  M45 scope and is recorded for the next milestone contract.

### Milestone 46 — Timed HD64180 ALU Register/Immediate Tail Coverage for PacManV8 T021

Status note, 2026-04-24:
- M45 closed the in-scope `AND r` / `AND (HL)` timed CPU coverage
  and was moved to blocked when the canonical PacManV8 T021 replay
  harness advanced past the AND sites and reached a new
  out-of-scope timed-opcode gap:

  ```text
  terminate called after throwing an instance of 'std::runtime_error'
    what():  Unsupported timed Z180 opcode 0xD6 at PC 0x3EA
  ```

  `0xD6` is `SUB n` in PacManV8
  `movement_distance_to_next_center_px:326`. The PacManV8
  blocked-task log contains a dedicated
  `### Recommended fix (Vanguard8 repo)` section that authorizes
  closing the entire remaining register/immediate ALU timed-opcode
  tail in a single Vanguard8 patch instead of one milestone per
  sister opcode. M46 adopts that recommendation verbatim as its
  scope contract.

Objective:
- Close the full register-form ALU tail (`ADC A,r`, `SUB r`,
  `SBC A,r`, `XOR r`) and the matching immediate peers (`ADC A,n`,
  `SUB n`, `SBC A,n`, `XOR n`) in a single pass so the PacManV8
  T021 replay path cannot abort again on a sister register-form or
  immediate-form ALU opcode inside the same rebuild cycle.

Deliverables:
- Timed extracted-core coverage in
  `Z180Adapter::current_instruction_tstates()` for:
  - `ADC A,r` (`0x88..0x8F`; `0x8E` = `ADC A,(HL)`)
  - `SUB r`   (`0x90..0x97`; `0x96` = `SUB (HL)`)
  - `SBC A,r` (`0x98..0x9F`; `0x9E` = `SBC A,(HL)`)
  - `XOR r`   (`0xA8..0xAF`; `0xAE` = `XOR (HL)`)
  expressed as range-checks alongside the existing `LD r,r'`,
  `ADD A,r`, and `AND r` range-checks.
- Removal of the now-redundant explicit dispatch entries for
  `0x91` (`SUB C`), `0x93` (`SUB E`), and `0xAF` (`XOR A`) from
  the existing `return 4` switch block.
- Addition of `ADC A,n` (`0xCE`), `SUB n` (`0xD6`), `SBC A,n`
  (`0xDE`), and `XOR n` (`0xEE`) to the existing uniform-7-cycle
  `return 7` case-label group.
- Focused CPU regression tests pinning operand/result, flags, `PC`
  advance, adapter T-state classification (4 for register forms,
  7 for `(HL)` and immediate forms), and the memory-read path for
  each `(HL)` form.
- A negative guard preserving the existing fail-fast contract for a
  still-out-of-scope sister form (for example a rotate/shift family
  base or an ED-prefix opcode).
- Non-perturbation fixture regression proof against an existing
  fixture ROM that does not exercise the newly covered opcodes.
- Task `M46-T01` captures the full opcode list, per-opcode test
  references, verification evidence, and — if a new out-of-scope
  blocker is surfaced — the precise next repro.

Closure rule:
- Keep this milestone inside `third_party/z180/`, `src/core/cpu/`,
  `tests/`, and the listed doc/task files only. Do not broaden into
  rotate/shift, index-register, or ED-prefix forms without a T021
  repro. Do not touch the existing explicit `OR r` (`0xB0..0xB7`)
  or `CP r` (`0xB8..0xBF`) entries — the recommended fix explicitly
  excludes those ranges. Do not edit PacManV8 sources from this
  repo.

Exit criteria:
- Every opcode in Allowed scope is dispatched by the timed
  extracted core with a matching focused test.
- The three redundant explicit entries (`0x91`, `0x93`, `0xAF`)
  have been removed from the `return 4` switch block.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes
  with the new coverage included and no pre-existing regression
  relaxed.
- Deterministic digests for unaffected fixture ROMs remain unchanged
  unless a deliberate change is explicitly recorded.
- The canonical PacManV8 T021 `tools/pattern_replay_tests.py`
  command either completes end-to-end against the current Vanguard8
  tree or stops on a new distinct blocker that is explicitly out of
  M46 scope and is recorded for the next milestone contract.

### Milestone 47 — Full MAME HD64180 Core Import (Retire Per-Opcode CPU Milestones)

Status note, 2026-04-26:
- Milestone 46 closed cleanly and was accepted on 2026-04-24. No
  new T021-driven CPU blocker is open.
- A repository audit
  (`docs/emulator/16-rom-readiness-audit.md`, 2026-04-26)
  documented that the timed Z180 dispatch in
  `third_party/z180/z180_core.{hpp,cpp}` and
  `src/core/cpu/z180_adapter.cpp` is a milestone-2 boot stub that
  has been extended one opcode per milestone since M19. The audit
  enumerated concrete missing surfaces (all `IX` / `IY`
  prefixes, all block transfers, all `RST n`, conditional `CALL`,
  most conditional `JP cc,nn`, `JP (HL)`, indirect `LD A,(BC)` /
  `(DE)` / mirrored, `EX (SP),HL` / `EX AF,AF'` / `EXX` / `LD
  SP,HL`, accumulator rotates, `DAA` / `CCF` / `LD (HL),n`,
  249 of 256 CB sub-opcodes, and most ED instructions) that no
  agentically authored ROM and no standard Z80/HD64180 toolchain
  output can avoid.
- The user has decided on 2026-04-26 to retire the one-opcode-
  per-milestone pattern. The Vanguard 8 *is* an HD64180 system,
  so the natural posture is to import the **full** MAME z180
  core in one pass and shrink `Z180Adapter` to bus glue. Milestone
  47 supersedes the milestone-2 extraction scope contract recorded
  in `third_party/z180/README.md`.

Objective:
- Replace the hand-written Vanguard 8 Z180 stub with the full MAME
  HD64180 / Z180 CPU core (BSD-3, pinned upstream commit), and
  remove the per-opcode timed dispatch tables in `Z180Adapter`
  and every `"Unsupported timed Z180 opcode …"` throw site. After
  M47, every published HD64180 instruction (full Z80 base set
  plus `MLT`, `IN0`, `OUT0`, `TST`, `TSTIO`, `OTIM` / `OTDM` /
  `OTIMR` / `OTDMR`, `SLP`) executes without dispatch
  fallthrough, with cycle timing supplied by the imported core.

Deliverables:
- `third_party/z180/z180_core.{hpp,cpp}` replaced with the
  imported MAME HD64180 / Z180 device sources at a single pinned
  commit. License header carried verbatim. The MAME device-
  framework dependencies stripped or replaced with a small
  in-tree shim that preserves the existing
  `vanguard8::core::cpu::Z180Adapter` public surface.
- `src/core/cpu/z180_adapter.{hpp,cpp}` reduced to bus glue.
  Specifically: `current_instruction_tstates()`,
  `cb_instruction_tstates()`, `ed_instruction_tstates()`, and
  every `throw std::runtime_error("Unsupported timed Z180
  opcode …")` site no longer exist anywhere in `src/core/cpu/`.
  The breakpoint, bank-switch logging, INT1 acknowledgement, and
  observability surfaces remain intact.
- `tests/test_cpu.cpp` rewritten: per-opcode T-state tests
  removed; spec-pinned integration tests retained or restated;
  new behavioral coverage for the previously unsupported
  instruction classes (`LDIR`, `IX`-based prologue, CB-prefix bit
  ops, `JP (HL)` table, `RST n` round-trip, `IM 2` + INT2 vector
  fetch); negative guard asserting no primary opcode 0x00–0xFF
  triggers a throw from the adapter timing API.
- Replay fixture digests in `tests/replays/` re-pinned. A one-
  time uniform delta is expected and acceptable; substantive
  behavior divergence is a defect.
- `third_party/z180/README.md` rewritten to describe the import,
  pin the upstream commit, list the spec-anchored Vanguard 8
  overrides preserved by the import (`CBAR` / `CBR` / `BBR` boot
  defaults, the `IL & 0xE0` rule, PRT reset values, HALT-resume),
  and remove the "full opcode coverage is a non-goal" line.
- `docs/emulator/16-rom-readiness-audit.md` Section 1 status
  updated to "Resolved" with a one-line pointer to M47.

Closure rule:
- Keep this milestone inside `third_party/z180/`,
  `src/core/cpu/`, `tests/test_cpu.cpp`, `tests/replays/`
  (digest re-pin only), and the listed doc/task files. Do not
  bundle unrelated audit closures (Text 1 / Text 2 renderers,
  Graphic 5 / 7, sprite overflow status fields, command-engine
  packing). Do not vendor a Z80-only core and write a local
  HD64180 extension layer — the only authorized path is the full
  MAME HD64180 / Z180 import. After M47, any future ROM trap on
  a CPU dispatch path is a bug in the adapter or in the imported
  core's pin, not an invitation to open M48 for that single
  opcode.

Exit criteria:
- The imported MAME HD64180 / Z180 core is in
  `third_party/z180/`, with the upstream commit SHA and license
  recorded in the README.
- `Z180Adapter::*_instruction_tstates()` and every
  `"Unsupported timed Z180 opcode"` throw site are gone from
  `src/core/cpu/`.
- `cmake --build cmake-build-debug` succeeds and
  `ctest --test-dir cmake-build-debug --output-on-failure`
  passes with the rewritten test surface.
- Replay-fixture digests are byte-identical across three
  post-import runs and recorded in M47-T01.
- The PacManV8 T021 harness completes end-to-end or surfaces a
  non-CPU blocker (recorded in M47-T01 for a follow-up
  milestone).

Closure note, 2026-04-26:
- M47 was accepted on 2026-04-26. The full MAME HD64180 / Z180
  core is imported at pinned upstream commit
  `c331217dffc1f8efde2e5f0e162049e39dd8717d`, the per-opcode
  timing tables and `"Unsupported timed Z180 opcode …"` throw
  sites are gone from `src/core/cpu/`, the spec-pinned CPU
  integration tests pass alongside new behavioral coverage for
  `LDIR`, `IX`-based prologues, CB-prefix bit ops, `JP (HL)`
  jump tables, `RST n` round-trips, and `IM 2` + INT2 vector
  fetch, the replay fixture's frame-4 digest is byte-identical
  to the M46 pin across three repeat runs, and the canonical
  PacManV8 T021 harness passes both replay cases end-to-end.
  `third_party/z180/README.md` and Section 1 of
  `docs/emulator/16-rom-readiness-audit.md` have been updated to
  reflect the new shipped state.
- Per-opcode milestones M19 through M46 are superseded in spirit
  by M47. Their history is kept intact for traceability; the
  closure annotation lives here, not in the older milestone
  bodies. Any future ROM trap on a CPU dispatch path is a defect
  in the adapter shim or in the imported core's pin, not an
  invitation to open a new per-opcode milestone.
- One narrow digest re-pin was deferred to M48 because
  `tests/test_frontend_backends.cpp` was not in M47's allowed
  paths.

### Milestone 48 — Re-pin PacManV8 T017 300-Frame Audio Digest

Status note, 2026-04-26:
- During M47 verification the imported core's more accurate audio
  timing produced a uniform digest shift in exactly one ctest
  entry outside M47's allowed paths: the test
  `PacManV8 T017 audio/video output is nonzero after instruction-
  granular audio timing` at `tests/test_frontend_backends.cpp:
  387`. The pinned hash is the M33-era pre-import value
  `61ca417e…839e7cc`; the post-import value is
  `a765959a…20d1ab27`, deterministic across three repeat runs.
- The structural assertions in the same test still pass post-
  import (audio nonzero, frame nonzero, `pc() != 0x2B8B`). Only
  the pinned digest needs updating; this is the exact "more
  accurate timing → digest shift" outcome M47 anticipated.

Objective:
- Update the pinned 300-frame PacManV8 audio SHA-256 in
  `tests/test_frontend_backends.cpp` to the deterministic
  post-M47 value, restore `ctest` to a clean pass, and record the
  pre/post pair as the M47 timing-shift evidence.

Deliverables:
- A single string-literal edit at
  `tests/test_frontend_backends.cpp:387`, replacing the pre-M47
  hash with the post-M47 hash. No other change to the test body
  and no change to the surrounding structural assertions.
- Three-run reproducibility evidence for the post-M48 hash,
  recorded in `M48-T01`.
- Doc/task updates required to lock the new digest.

Closure rule:
- Keep this milestone inside `tests/test_frontend_backends.cpp`
  (digest re-pin only) and the listed doc/task files. Do not
  bundle other digest re-pins, test refactors, or any source
  changes. Do not re-run M47's import; the import is locked.

Exit criteria:
- The string literal at
  `tests/test_frontend_backends.cpp:387` reads
  `"a765959a62e9a5afe9d075206efb8943e3141ef4274844a07c35f21c20d1ab27"`.
- `ctest --test-dir cmake-build-debug --output-on-failure`
  passes with the usual showcase milestone-7 skip and no other
  failures.
- Three repeat runs of the T017 test produce byte-identical
  post-M48 hashes.
- The canonical PacManV8 T021 harness still passes
  end-to-end (sanity check; the digest re-pin cannot affect T021
  by construction).
- `M48-T01` is moved to `docs/tasks/completed/` with the
  required completion summary.

## Suggested Release Gates

Use these as project-wide checkpoints rather than individual milestone tasks:

### Gate A — First Deterministic Headless Core

Required milestones:
- 0 through 3

Meaning:
- The emulator can boot ROM code, schedule hardware events, and run repeatably
  without frontend dependencies.

### Gate B — First Playable Video Build

Required milestones:
- 0 through 6

Meaning:
- A user can load a ROM, see composited output, and provide controller input.

### Gate C — First Full-System Alpha

Required milestones:
- 0 through 9

Meaning:
- CPU, video, audio, interrupts, DMA, and debugger support are present.

### Gate D — Regression-Protected Beta

Required milestones:
- 0 through 10

Meaning:
- The emulator is safe to evolve because determinism, save states, and replay
  tooling are in place.

### Gate E — 1.0 Baseline

Required milestones:
- 0 through 11

Meaning:
- The documented hardware surface is implemented except for explicit spec
  deferrals, and those deferrals are clearly called out in docs and tests.

### Gate F — First Playable Desktop Frontend

Required milestones:
- 0 through 12

Meaning:
- The frontend is no longer terminal-only; it can launch a ROM in a real window
  with live video, live audio, and minimal input/debug bring-up support.

### Gate G — First ROM-Compatibility Desktop Build

Required milestones:
- 0 through 13

Meaning:
- The highest-value pre-GUI audit blockers for real ROM testing are closed and
  the desktop build is suitable for practical showcase-ROM validation.

### Gate H — Lightweight Desktop Developer Build

Required milestones:
- 0 through 14

Meaning:
- The desktop build includes lightweight diagnostic tooling and any selected
  non-essential closures that proved necessary during ROM bring-up, without
  requiring a full live debugger project.

### Gate I — First Real-ROM Audio Bring-Up Closure

Required milestones:
- 0 through 15

Meaning:
- The emulator can execute the first ROM-driven INT1/audio compatibility path
  needed to resume showcase-ROM audio verification work without falling over in
  the timed CPU core.

### Gate J — First Pac-Man Boot/Background CPU Bring-Up

Required milestones:
- 0 through 19

Meaning:
- The timed CPU runtime can clear the first Pac-Man boot/background bring-up
  blocker without broadening into full Z80/Z180 opcode completion.

### Gate K — First Pac-Man Palette VCLK Bring-Up

Required milestones:
- 0 through 20

Meaning:
- The timed CPU runtime can execute the Pac-Man palette swatch path with active
  `VCLK 4000` timing, not only with `VCLK` disabled.

### Gate L — First PacManV8 Boot CPU Bring-Up

Required milestones:
- 0 through 21

Meaning:
- The timed CPU runtime can execute the PacManV8 minimal boot ROM without
  aborting on missing `LD r,n` opcode variants in the timed extracted path.

## Recommended Order of Work Inside the Repo

1. Build and test scaffolding
2. Core memory/cartridge/bus
3. CPU/MMU/interrupt wiring
4. Scheduler and deterministic headless execution
5. Video bring-up
6. Input/frontend usability
7. Audio timing and mixing
8. DMA/timers/command engine
9. Debugger
10. Save/replay/headless regression
11. Expanded mode coverage and release hardening
12. Playable desktop ROM bring-up baseline
13. Critical real-ROM compatibility closure
14. Lightweight tooling and selected non-essential closure
15. Timed HD64180 INT1 audio compatibility patch
16. Timed HD64180 boot opcode compatibility
17. Timed HD64180 palette VCLK compatibility
18. Timed HD64180 PacManV8 boot opcode compatibility

## Definition of Done for Each Milestone

- Behavior implemented matches current spec docs
- Tests pin the relevant hardware rules
- Headless execution remains deterministic
- Any known limitation is documented in code and docs
- No milestone silently fills in unspecified hardware behavior

# Implementation Task Index

Derived from `docs/emulator/07-implementation-plan.md`.

## Milestone 0

- `M00-T01` Bootstrap build layout and dependency manifests.
- `M00-T02` Wire Catch2, `ctest`, logging/config scaffolding, and no-op
  frontend/headless shells.

## Milestone 1

- `M01-T01` Implement cartridge image loading and 32 KB SRAM devices.
- `M01-T02` Implement the physical `Bus` map and open-bus behavior.
- `M01-T03` Add ROM bank switching, cartridge validation, and stubbed port
  endpoints.

## Milestone 2

- `M02-T01` Integrate the Z180 adapter and reset defaults.
- `M02-T02` Implement `CBAR`/`CBR`/`BBR` MMU translation and illegal `BBR`
  checks.
- `M02-T03` Wire INT0/INT1 behavior and prove the boot ROM MMU sequence.

## Milestone 3

- `M03-T01` Implement the master-cycle counter and scheduler core.
- `M03-T02` Add H-blank, V-blank, V-blank-end, and `/VCLK` event timing.
- `M03-T03` Add deterministic headless run/pause/frame-step control.

## Milestone 4

- `M04-T01` Add the V9938 port interface, VRAM, and palette storage.
- `M04-T02` Implement the Graphic 4 scanline renderer and `R#23` vertical
  scroll.
- `M04-T03` Add SDL/OpenGL upload and headless frame-dump regression support.

## Milestone 5

- `M05-T01` Add VDP-B and the VDP-A-over-VDP-B compositor.
- `M05-T02` Implement the sprite path and covered collision/overflow flags.
- `M05-T03` Implement VDP-A interrupt-visible status behavior and VDP-B IRQ
  isolation.

## Milestone 6

- `M06-T01` Add active-low controller devices and SDL input mapping.
- `M06-T02` Add config creation/loading plus ROM open and validation flow.
- `M06-T03` Add recent-ROM, fullscreen, scale-mode, and frame-pacing support.

## Milestone 7

- `M07-T01` Integrate YM2151 and AY-3-8910 register surfaces.
- `M07-T02` Integrate MSM5205 control and INT1-driven `/VCLK` timing.
- `M07-T03` Add mixer/resampler output and deterministic audio hashing.

## Milestone 8

- `M08-T01` Implement DMA channel 0 and channel 1 transfer paths.
- `M08-T02` Implement HD64180 timer/counter exposure and validation.
- `M08-T03` Implement the documented VDP command engine and `S#2.CE` timing.

## Milestone 9

- `M09-T01` Add the ImGui shell and docking layout.
- `M09-T02` Add CPU, memory, and VDP inspection panels.
- `M09-T03` Add interrupt log, bank tracker, breakpoints, and watchpoints.

## Milestone 10

- `M10-T01` Implement versioned save-state serialization coverage.
- `M10-T02` Add rewind and deterministic input replay.
- `M10-T03` Add headless frame/audio hashing and replay-backed `ctest`
  integration.

## Milestone 11

- `M11-T01` Expand documented VDP mode coverage and mixed-mode validation.
- `M11-T02` Run compatibility/performance closure work against real ROMs.
- `M11-T03` Close documentation gaps and finalize the 1.0 readiness audit.

## Milestone 12

- `M12-T01` Refactor the frontend into a persistent SDL runtime and window host.
- `M12-T02` Add the real display and audio backends plus deterministic
  presentation/readback seams.
- `M12-T03` Wire command-line ROM launch, minimal live input, and runtime
  status closure.

## Milestone 13

- `M13-T01` Implement the V9938 read-path fixes and targeted HD64180 audit
  coverage needed for ROM bring-up.
- `M13-T02` Expand sprite compatibility with 16x16/magnification and
  register-relative table addressing.
- `M13-T03` Add Sprite Mode 1 plus Graphic 1/2 renderer coverage for startup,
  title, and menu-screen ROM paths.

## Milestone 14

- `M14-T01` Add trace-to-file support and lightweight runtime debug output.
- `M14-T02` Add symbol loading and annotation support.
- `M14-T03` Close the remaining selected non-essential gaps and deferral
  ledger.
- `M14-T04` Execute CPU instructions inside the scheduled frame loop for ROM
  bring-up.
- `M14-T05` Reject unexpected positional CLI arguments in the desktop
  frontend.
- `M14-T06` Restore documented desktop frame pacing fallback in the runtime
  loop.

## Milestone 15

- `M15-T01` Fix the timed `INT1` audio bring-up path blocking the showcase
  milestone-5 ROM scene.

## Milestone 16

- `M16-T01` Add the narrow `Graphic 6` renderer and mixed-mode compositor
  coverage needed for a high-resolution HUD overlay scene.

## Milestone 17

- `M17-T01` Fix the full-range VDP CPU VRAM addressing path needed for 64 KB
  high-address `Graphic 4`/`Graphic 6` compatibility.

## Milestone 18

- `M18-T01` Make headless frame dumps match actual runtime compositor output.

## Milestone 19

- `M19-T01` Cover the missing timed HD64180 boot opcodes needed by the
  Pac-Man background bring-up path.

## Milestone 20

- `M20-T01` Cover the missing timed HD64180 opcode needed by the Pac-Man
  palette swatch path under `VCLK 4000`.

## Milestone 32

- `M32-T01` Frontend live audio playback for interactive verification.

## Milestone 33

- `M33-T01` Fix instruction-granular audio timing for YM2151 busy polls.

## Milestone 34

- `M34-T01` Cover the missing timed HD64180 PacManV8 intermission opcodes.

## Milestone 35

- `M35-T01` Add headless agent observability primitives for ROM verification.

## Milestone 36

- `M36-T01` Fix the HD64180 HALT-resume path so foreground code after
  `HALT` actually runs on interrupt return.

## Milestone 37

- `M37-T01` Cover the missing timed HD64180 `LD E,n` opcode (plus sister
  `LD H,n` / `LD L,n` forms) needed by the PacManV8 T020 intermission
  panel setup path.

## Milestone 38

- `M38-T01` Cover the missing timed HD64180 `RET NC` opcode (plus
  carry-family sister `RET C` form) needed by the PacManV8 T020
  intermission review-index advance path.

## Milestone 39

- `M39-T01` Cover the missing timed HD64180 16-bit pair INC/DEC
  opcodes (`DEC BC`, `INC BC`, `INC DE`, `DEC HL`) and the narrow
  CB-prefix dispatch surface (`SRL A`, `BIT 4..7,A`) needed by the
  PacManV8 T021 pattern-replay validation path.

## Milestone 40

- `M40-T01` Cover the missing timed HD64180 `EX DE,HL` opcode and the
  immediate same-path `OR (HL)` look-ahead gap needed by the PacManV8
  T021 pattern-replay validation path.

## Milestone 41

- `M41-T01` Cover the missing timed HD64180 CB-prefix `SRL H` opcode
  and the immediate same-path `RR L` look-ahead gap needed by the
  PacManV8 T021 pattern-replay validation path.

## Milestone 42

- `M42-T01` Cover the missing timed HD64180 `SCF` opcode needed by
  the PacManV8 T021 pattern-replay validation path's "return with
  carry set" idiom after the `collision_prepare_tile`
  divide-by-eight sequence.

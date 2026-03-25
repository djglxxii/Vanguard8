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

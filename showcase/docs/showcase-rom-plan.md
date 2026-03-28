# Vanguard 8 Showcase ROM Plan

## Purpose

Create a demonstration cartridge ROM that serves two roles at once:

1. Show off the strongest documented Vanguard 8 hardware features with
   intentional presentation.
2. Provide a practical ROM-driven validation target for the current emulator.

This plan is written against the current documented and implemented hardware
surface, not against speculative future features.

Traceability:
- Hardware contract: `docs/spec/00-overview.md`
- CPU/MMU/interrupt contract: `docs/spec/01-cpu.md`
- Video contract: `docs/spec/02-video.md`
- Audio contract: `docs/spec/03-audio.md`
- I/O/timing contract: `docs/spec/04-io.md`
- Current emulator compatibility posture: `docs/emulator/08-compatibility-audit.md`
- Milestone-13/14 closure notes:
  `docs/emulator/11-pre-gui-audit.md`,
  `docs/tasks/completed/M13-T03-graphic1-graphic2-and-sprite-mode1.md`,
  `docs/tasks/completed/M14-T03-targeted-closure-and-deferral-ledger.md`

## Current Baseline

The repo is in a good position to support a showcase ROM:

- The emulator already loads raw cartridge images and exposes a usable desktop
  and headless runtime.
- The headless runner can emit frame hashes, audio hashes, traces, and symbol-
  aware annotations.
- The covered graphics surface includes Graphic 1, Graphic 2, Graphic 3, and
  Graphic 4, dual-VDP compositing, Sprite Mode 1, Sprite Mode 2, covered sprite
  compatibility work, and the documented Graphic 4 command-engine paths.
- The covered audio surface includes YM2151, AY-3-8910, MSM5205, and the
  documented INT1-driven ADPCM flow.
- The covered CPU/system surface includes MMU setup, ROM banking, INT0/INT1,
  DMA channel paths, and programmable reload timer exposure.

There is no ROM authoring/build scaffold in the repo yet. That must be added as
part of the showcase effort.

## Goals

- Produce a single reproducible cartridge ROM image plus symbol file.
- Make the ROM usable both as a polished attract-mode showcase and as a
  deterministic validation target.
- Prefer real supplied assets over generated placeholder content.
- Keep the first version strictly within documented and covered hardware
  behavior.
- Build the ROM in a way that supports incremental scene bring-up and headless
  regression capture.

## Non-Goals For Version 1

- Do not depend on unverified or explicitly deferred hardware behavior.
- Do not treat the showcase ROM as a general SDK or game-engine effort.
- Do not broaden emulator behavior just to support a flashy demo scene.
- Do not introduce later-stage content tooling before the minimal ROM pipeline
  exists.

## Explicitly Avoided Features In The First Showcase Pass

The first ROM should not rely on:

- Text 1 or Text 2
- Graphic 5, Graphic 6, or Graphic 7
- Horizontal scroll via `R#26`/`R#27`
- Per-line horizontal parallax based on unverified scroll behavior
- Any invented Graphic 3 LN=1 background fetch behavior for lines `192-211`
- 4-player expansion behavior
- Battery-backed cartridge SRAM mapping details
- Any broader HD64180 behavior outside the repo's current tested execution
  subset unless it is first proven in the ROM bring-up phase

## Recommended Repo Structure

Keep showcase-ROM work in a dedicated top-level workspace:

```text
showcase/
  README.md
  docs/
    showcase-rom-plan.md
    content-brief.md
    asset-requests/
  assets/
    art/
    audio/
    licenses/
  src/
    boot/
    engine/
    scenes/
    data/
  tools/
    convert/
    package/
  tests/
    captures/
    manifests/
```

Rationale:

- ROM source and asset tools are a separate product surface from the emulator
  core.
- It keeps `src/` and `tests/` focused on the emulator implementation.
- It gives the showcase ROM its own iteration loop without implying that ROM
  authoring is part of the emulator runtime.

## Build Pipeline Plan

Use `sjasmplus` as the initial ROM assembler. It is already available in the
local environment and is a simple fit for a banked Z80/HD64180 cartridge image.

Planned outputs:

- `showcase.rom`
- `showcase.sym`
- generated converted asset blobs used to build the ROM

Planned supporting tools:

- Python conversion scripts for:
  - palette packing
  - tile conversion
  - sprite conversion
  - Graphic 4 bitmap packing
  - ADPCM sample packing/prep
- a packaging script or Make/CMake wrapper to assemble the ROM deterministically

## Development Strategy

Build the showcase ROM in two layers.

### Layer 1: Bring-Up Cartridge

The first ROM should be deliberately plain and deterministic. Its job is to
prove that the whole authoring and runtime path works:

- boot/reset path
- MMU setup
- ROM bank switching
- VDP initialization on both chips
- INT0 service
- INT1-driven MSM5205 feed
- scene switching
- symbol generation
- trace readability

Deliverable:

- a minimal ROM that boots, changes banks, initializes audio/video, and cycles
  through a few simple known scenes under deterministic timing

### Layer 2: Polished Showcase

Once the bring-up cartridge is stable, replace fixture-like scenes with
presented showcase scenes using real supplied content.

Deliverable:

- a single attract-style ROM whose scenes still map cleanly to regression
  checkpoints

## Recommended Scene Set

Each scene should be visually distinct and validate a specific hardware path.

### Scene 1: Boot / Identity Screen

Purpose:
- prove basic boot, palette setup, controller polling, and title presentation

Suggested content:
- Vanguard 8 logo
- short YM2151 boot sting
- simple menu prompt or timed auto-advance

Key validation:
- fixed ROM boot path
- palette writes
- controller reads

### Scene 2: Dual-VDP Compositing Demo

Purpose:
- show the system-defining two-layer architecture directly

Suggested content:
- VDP-B background artwork
- VDP-A foreground frame, logo, or cutout overlay
- obvious transparency windows using VDP-A color index `0`

Key validation:
- VDP-A-over-VDP-B priority
- transparency behavior
- independent palette setup per VDP

### Scene 3: Tile-Mode Presentation

Purpose:
- exercise covered tile paths likely to appear in menus and title screens

Suggested content:
- Graphic 1 or Graphic 2 title/menu scene
- Sprite Mode 1 cursor or icon animation

Key validation:
- Graphic 1 or Graphic 2 tile fetch
- Sprite Mode 1 behavior
- 192-line mode handling

### Scene 4: Graphic 3 + Graphic 4 Mixed-Mode Scene

Purpose:
- show mixed-mode layer operation using documented covered paths

Suggested content:
- Graphic 4 bitmap background on one VDP
- Graphic 3 tile/status/ornament layer on the other
- transparent cutouts between layers

Key validation:
- mixed-mode compositing
- Graphic 3 covered fetch behavior
- Scene design must avoid depending on undocumented lines `192-211`

### Scene 5: Sprite Capability Scene

Purpose:
- demonstrate sprite scale and compatibility work clearly

Suggested content:
- side-by-side examples of 8x8, 16x16, and magnified sprites
- overlapping sprites with deliberate priority
- controlled per-scanline load to expose visibility limits cleanly

Key validation:
- Sprite Mode 1 and Mode 2 paths
- 16x16 and magnification behavior
- collision/overflow/status behavior in covered cases

### Scene 6: Graphic 4 Command-Engine Scene

Purpose:
- show off the V9938 command engine rather than only static VRAM writes

Suggested content:
- tile or panel reveals
- blitted logo assembly
- scripted wipes or object copies using covered commands

Key validation:
- Graphic 4 command-path correctness
- `CE`-visible command timing along covered paths

### Scene 7: Audio Feature Scene

Purpose:
- demonstrate all three audio chips distinctly before mixing them together

Suggested content:
- YM2151 music phrase
- AY lead/noise/SFX layer
- MSM5205 short voice or percussion sample
- combined playback pass

Key validation:
- YM2151 register programming
- AY register surface
- MSM5205 nibble feed via INT1
- mixed output determinism

### Scene 8: System Validation Scene

Purpose:
- expose machine behavior useful during bring-up and future regression work

Suggested content:
- controller visualizer
- timer or IRQ activity display
- banked asset/page switch
- optional DMA-fed content update if kept narrow and deterministic

Key validation:
- active-low controller reads
- timers/interrupt flow
- bank switching under live content

## Regression Plan

The showcase ROM should be treated as a ROM-backed regression target, not just
as a presentation artifact.

Planned regression surfaces:

- frame hashes at selected known scene/frame checkpoints
- audio hash across a fixed attract loop window
- optional replay-backed controller inputs for interactive scenes
- trace snapshots around boot, bank switch, and INT1-driven ADPCM playback
- symbol-aware trace verification using the generated `.sym` file

The ROM should expose fixed checkpoints such as:

- boot complete
- first compositing scene
- first audio-only section
- first bank switch
- full-loop return to title

## Proposed Implementation Phases

### Phase 0: Project Setup

- create showcase workspace
- choose ROM assembler conventions
- define output filenames and build entry points
- define symbol-export expectations

### Phase 1: Bring-Up Skeleton

- boot code
- MMU setup
- interrupt table setup
- scene loop scaffold
- one banked content page
- one frame-hash checkpoint

### Phase 2: Video Bring-Up Scenes

- title scene
- compositing scene
- tile-mode scene
- mixed-mode scene
- sprite scene

### Phase 3: Audio Bring-Up Scenes

- YM scene
- AY scene
- MSM scene
- combined music/SFX scene

### Phase 4: Regression Harness

- checkpoint manifest
- headless commands for frame/audio hashes
- replay scripts where needed
- trace capture examples

### Phase 5: Content Polish

- replace placeholder content with supplied real assets
- tighten scene pacing and transitions
- finalize bank layout and ROM size budgeting

## Real Asset Intake Plan

To avoid generated filler, request assets in a small first wave.

Preferred first-wave assets:

- one title/logo image
- one background illustration for the compositing scene
- one tile-friendly image or tileset
- one sprite sheet
- one short music reference or MIDI-equivalent source
- one short mono WAV for MSM5205 preparation

For each asset, capture:

- source file
- ownership/license status
- intended scene
- target conversion format
- any palette restrictions or color priorities

## Open Decisions To Resolve Before Implementation

- final assembler invocation and build wrapper format
- ROM bank layout strategy
- symbol-file format expected from the assembler and any post-processing needed
- whether DMA is included in the first showcase loop or deferred to a later pass
- exact checkpoint list for deterministic regression

## Recommended Immediate Next Steps

1. Create the minimal ROM build scaffold under `showcase/src/` and
   `showcase/tools/`.
2. Build the plain bring-up cartridge before requesting large content drops.
3. Define the first checkpointed scene list and bank budget.
4. Request the first-wave real assets listed above.
5. Add a small documented command set for building the ROM and validating it
   with `vanguard8_headless`.

# Vanguard 8

A hypothetical premium home game console, designed as if launched in mid-1986.
Positioned as "the ultimate 8-bit home arcade experience" — above the NES and
Sega Master System in capability, far below the cost of an arcade cabinet.

This repository contains the Vanguard 8 hardware specification, emulator design,
implementation source tree, milestone contracts, and task queue.

Current repo state:
- The emulator core, tests, headless binary, and a deterministic frontend
  launcher are present and build.
- A dedicated `showcase/` workspace is reserved for a ROM-side demonstration
  cartridge, asset pipeline, and showcase-specific regression work so that ROM
  authoring stays separate from the emulator implementation tree.
- The current `vanguard8_frontend` binary is not yet a live desktop GUI
  application. It prints frontend/debugger status to the terminal and can build
  deterministic frame data, but it does not currently open an SDL/OpenGL window
  or render an interactive Dear ImGui debugger on screen.
- Desktop GUI implementation requirements and the milestone-12+ roadmap now
  live in `docs/emulator/10-desktop-gui-audit.md` and
  `docs/emulator/07-implementation-plan.md`.

**Design constraint:** No custom silicon. Every chip was commercially available
off-the-shelf by 1985–1986. No ASIC NRE, no mask costs, no fab risk.

---

## Key Specifications

| Component     | Part                          | Notes                                        |
|---------------|-------------------------------|----------------------------------------------|
| CPU           | Hitachi HD64180 @ 7.16 MHz   | Z80-compatible, on-chip MMU + DMA            |
| Main RAM      | 32 KB SRAM (62256)           | Fast access, full CPU speed                  |
| VRAM          | 64 KB DRAM × 2               | 64 KB dedicated to each VDP                  |
| VDP (Layer A) | Yamaha V9938                 | Foreground layer + sprites; all V9938 modes   |
| VDP (Layer B) | Yamaha V9938                 | Background layer + sprites; all V9938 modes   |
| FM Audio      | Yamaha YM2151 (OPM)          | 8ch × 4-op FM, stereo pan per channel        |
| PSG Audio     | AY-3-8910                    | 3 square + noise + envelope                  |
| ADPCM Audio   | OKI MSM5205                  | 4-bit ADPCM from cartridge ROM               |
| Media         | Cartridge, up to 960 KB ROM  | 16 KB bank window, HD64180 MMU, no mapper    |

---

## Repository Structure

```
AGENTS.md              Repo operating contract for coding agents
docs/
  spec/                Authoritative hardware reference (source of truth)
    00-overview.md     System overview, memory map, I/O port map
    01-cpu.md          HD64180 CPU reference
    02-video.md        Dual V9938 video system reference
    03-audio.md        YM2151 + AY-3-8910 + MSM5205 audio reference
    04-io.md           Controllers, timing, interrupts

  emulator/            Emulator design documentation
    00-overview.md     Architecture, component graph, technology stack, feature list
    01-build.md        Build system (CMake + vcpkg), dependencies, directory layout
    02-emulation-loop.md  Scanline loop, timing model, interrupt sequencing, save states
    03-cpu-and-bus.md  HD64180/Z180 core integration, MMU, bus routing, interrupt wiring
    04-video.md        V9938 emulation, compositor, OpenGL display pipeline, shaders
    05-audio.md        YM2151/AY-3-8910/MSM5205 integration, mixing, resampling
    06-debugger.md     Dear ImGui debugger panels and developer tooling

  milestones/          Per-milestone implementation contracts
  current-milestone.md Active milestone lock

  process/             Review and acceptance workflow docs
    milestone-acceptance-checklist.md

  tasks/               Active/completed/blocked execution queue
    README.md
    task-template.md
showcase/
  docs/                Showcase-ROM planning and content briefs
  assets/              Source art/audio and provenance notes
  src/                 Showcase-ROM source tree
  tools/               Asset conversion and ROM packaging helpers
  tests/               Showcase-ROM checkpoint manifests and captures
```

---

## Emulator Status

The emulator is a C++20 Linux implementation built for accuracy and developer
ergonomics. The detailed design and implementation roadmap live under
`docs/emulator/`.

Implemented headline properties:

- Scanline-accurate video, instruction-level CPU accounting, and
  sample-accurate audio derived from the 14.31818 MHz master clock.
- Extracted Z180-based HD64180 execution with milestone-11 compatibility audit
  notes documented in `docs/emulator/03-cpu-and-bus.md` and
  `docs/emulator/08-compatibility-audit.md`.
- Dual V9938 rendering with VDP-A-over-VDP-B compositing.
- YM2151, AY-3-8910, and MSM5205 audio via the libraries documented in
  `docs/emulator/05-audio.md`.

Planned-but-not-yet-live desktop features:

- SDL2 desktop window and OpenGL presentation path.
- Interactive Dear ImGui debugger rendering.
- Live audio device playback and runtime desktop UX around ROM loading,
  fullscreen, drag-and-drop, and overlays.

---

## Positioning

| System                        | Era     | Relation to Vanguard 8                              |
|-------------------------------|---------|-----------------------------------------------------|
| Nintendo NES                  | 1985    | Below — 1 BG layer, 8 spr/line, 25 colors, no FM   |
| Sega Master System            | 1986    | Below — 1 BG layer, 8 spr/line, 32 colors          |
| **Vanguard 8**                | **1986**| **2 BG layers, 16 spr/line, 16 colors/layer from 512** |
| Typical mid-80s arcade board  | 1985–87 | Above — custom scaling, 3+ layers, dense sprites    |

The Vanguard 8 sits in the gap: capable of faithful, enhanced arcade ports that
clearly surpass contemporary home consoles, while stopping short of the exotic
scaling, rotation, and multi-CPU power of the arcade originals.

Each V9938 supports all nine V9938 display modes — bitmap (Graphic 4–7),
tile-based (Graphic 1–3), and text (Text 1/2). The two chips can run the same
mode or different modes simultaneously; compositing is mode-agnostic and driven
by VDP-A's YS pin. The primary/default mode is **Graphic 4** (256×212, 4bpp
bitmap); Graphic 3 (tile mode with Sprite Mode 2) is a natural companion layer.
Each chip has an independent 16-entry palette chosen from a 512-color 9-bit RGB
space. Sprites are hardware-rendered with per-row color via Sprite Mode 2 (in
Graphic 3–7) or single-color via Sprite Mode 1 (in Graphic 1–2).

# Vanguard 8 Emulator — Design Overview

## Purpose

This document and the files alongside it define the planned design of the
Vanguard 8 emulator: a cycle-honest, scanline-accurate software implementation
of the Vanguard 8 hardware for Linux, written in C++. The hardware
specification in `docs/spec/` is the authoritative source for all emulated
behavior. When this document and the spec conflict, the spec wins.

---

## Design Principles

- **Spec-first.** No behavior is invented. If the spec does not define it, the
  emulator marks it unimplemented and logs a warning rather than guessing.
- **Authentic timing.** All clocks derive from a single 14.31818 MHz master
  counter. Subsystem clocks are derived by integer division, not independent
  timers.
- **Readable component boundaries.** Each hardware chip maps to one C++ class.
  The bus connects them. No component reaches directly into another.
- **Debugger model always present.** The debugger snapshot/control model is
  compiled into the repo today. Live Dear ImGui presentation remains a desktop
  frontend milestone and must not be implied by the current CLI frontend.
- **No custom silicon to invent.** The hardware uses only off-the-shelf parts.
  The emulator has no undefined internal state to fill in — every register is
  documented.

---

## Technology Stack

| Layer              | Technology                        | Role                                               |
|--------------------|-----------------------------------|----------------------------------------------------|
| Language           | C++20                             | Core emulator                                      |
| Build system       | CMake 3.25+                       | Build and dependency management                    |
| Dependencies       | vcpkg (manifest mode)             | Third-party library acquisition                    |
| Window / input     | SDL2                              | Window creation, keyboard/gamepad input, audio out |
| GPU / rendering    | OpenGL 3.3 core (via SDL2)        | Framebuffer display, scaling, CRT shader           |
| Debug UI           | Dear ImGui (docking branch)       | All developer panels                               |
| CPU core           | MAME Z180 core (extracted)        | HD64180 / Z180 instruction execution and MMU       |
| FM audio           | Nuked-OPM                         | YM2151 cycle-accurate emulation                    |
| PSG audio          | Ayumi                             | AY-3-8910 emulation                                |
| ADPCM audio        | MAME MSM5205 core (extracted)     | OKI MSM5205 ADPCM decode                          |
| Audio resampling   | libsamplerate (SRC)               | Native chip rates → SDL2 output rate              |
| Configuration      | toml++                            | User-readable config file                          |
| Logging            | spdlog                            | Structured logging; debug trace buffer             |
| Testing            | Catch2                            | Unit and integration tests                         |
| File dialogs       | nativefiledialog-extended         | Native OS file picker for ROM load                 |

---

## Current Repo State

The architecture below is still the target desktop shape, not the exact
implemented frontend state.

Implemented today:
- Core emulation, tests, headless runtime, desktop SDL/OpenGL/audio runtime,
  deterministic frame upload/dump buffer, ROM/config plumbing, debugger
  snapshot/control models, basic symbol loading/annotation over the covered
  disassembly/memory/trace surfaces, and CLI trace-to-file support over the
  covered CPU stepping surface.

Not yet implemented as a live desktop path:
- Interactive Dear ImGui rendering

The authoritative gap analysis and implementation requirements for that work
live in `docs/emulator/10-desktop-gui-audit.md`.

---

## Component Graph

```
┌─────────────────────────────────────────────────────────────────────┐
│                          Emulator (top level)                       │
│                                                                     │
│  ┌──────────┐   ┌──────────────────────────────────────────────┐   │
│  │  Config  │   │                    Bus                        │   │
│  └──────────┘   │                                              │   │
│                 │  ┌──────────────┐   ┌─────────────────────┐ │   │
│  ┌──────────┐   │  │  HD64180 CPU │   │   CartridgeSlot      │ │   │
│  │  SDL2    │   │  │  (Z180 core) │   │   (ROM image + bank) │ │   │
│  │  Window  │   │  │  + MMU       │   └─────────────────────┘ │   │
│  │  Input   │◀──│  │  + DMA       │                           │   │
│  │  Audio   │   │  │  + Timers    │   ┌─────────────────────┐ │   │
│  └──────────┘   │  └──────────────┘   │   SRAM (32 KB)      │ │   │
│                 │                     └─────────────────────┘ │   │
│  ┌──────────┐   │  ┌──────────────┐   ┌─────────────────────┐ │   │
│  │ OpenGL   │   │  │   VDP-A      │   │   VDP-B             │ │   │
│  │ Display  │◀──│  │   (V9938)    │   │   (V9938)           │ │   │
│  │ + Shader │   │  │   + 64KB RAM │   │   + 64KB RAM        │ │   │
│  └──────────┘   │  └──────┬───────┘   └────────┬────────────┘ │   │
│                 │         │                     │              │   │
│  ┌──────────┐   │  ┌──────▼─────────────────────▼───────────┐ │   │
│  │  ImGui   │   │  │         Compositor (74LS257 model)      │ │   │
│  │ Debugger │   │  │         YS-pin mux: VDP-A over VDP-B   │ │   │
│  │  Panels  │   │  └────────────────────────────────────────┘ │   │
│  └──────────┘   │                                              │   │
│                 │  ┌──────────────┐   ┌─────────────────────┐ │   │
│                 │  │   YM2151     │   │   AY-3-8910         │ │   │
│                 │  │  (Nuked-OPM) │   │   (Ayumi)           │ │   │
│                 │  └──────────────┘   └─────────────────────┘ │   │
│                 │                                              │   │
│                 │  ┌──────────────┐   ┌─────────────────────┐ │   │
│                 │  │   MSM5205    │   │   AudioMixer        │ │   │
│                 │  │  (MAME core) │   │   + libsamplerate   │ │   │
│                 │  └──────────────┘   └─────────────────────┘ │   │
│                 │                                              │   │
│                 │  ┌──────────────────────────────────────┐   │   │
│                 │  │  ControllerPorts [0x00, 0x01]        │   │   │
│                 │  └──────────────────────────────────────┘   │   │
│                 └──────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
```

Each component owns its own state. The `Bus` owns instances of all components
and is the sole routing layer for memory reads, memory writes, I/O reads, and
I/O writes. The CPU core calls into the `Bus`; the `Bus` dispatches to the
correct component based on address or port number.

---

## Accuracy Model

### Timing unit

All timing is expressed in **master clock cycles** (14.31818 MHz). The main loop
does not use wall-clock time to drive emulation — it uses master-cycle budgets.

### CPU

- Instruction-level execution. Each instruction consumes its documented T-state
  count, which is converted to master clock cycles (1 CPU cycle = 2 master
  cycles).
- The CPU core maintains a cycle counter. The main loop runs the CPU until the
  per-scanline budget is exhausted, then steps to the next phase.

### VDP (both chips)

- Scanline-accurate. Each scanline is processed as a unit: active display,
  H-blank boundary, H-blank interrupt check (VDP-A only), then repeat.
- V-blank begins after scanline 212 and lasts 50 scanlines (262 total).
- Both VDPs are always advanced together since they share the master clock and
  are frame-perfectly synchronized in hardware.

### Audio

- Each chip generates samples at its native rate derived from the master clock.
  The AudioMixer accumulates samples into a ring buffer. libsamplerate converts
  to the SDL2 output rate (typically 48 kHz) before delivery.
- MSM5205 /VCLK events are counted in master cycles, not wall-clock time, so
  ADPCM interrupt timing is accurate relative to the CPU.

### Interrupts

- INT0 (VDP-A V-blank, VDP-A H-blank, YM2151 timer): asserted and cleared at
  the exact master-cycle boundary where the hardware event occurs.
- INT1 (MSM5205 /VCLK): asserted on counted master-cycle boundaries matching
  the selected sample rate. The INT1 vectored response (mode-independent,
  I/IL registers) is implemented exactly as specified in `docs/spec/01-cpu.md`.

---

## Repository Layout (target)

```
Vanguard8/
├── docs/
│   ├── spec/           Hardware specification (source of truth)
│   └── emulator/       This design documentation
├── src/
│   ├── core/           Emulator core (bus, CPU wrapper, VDP, audio, I/O)
│   ├── frontend/       SDL2 window, OpenGL display, ImGui shell
│   ├── debugger/       All ImGui debugger panels
│   └── main.cpp        Entry point
├── third_party/
│   ├── z180/           Extracted MAME Z180 CPU core
│   ├── nuked-opm/      YM2151 emulation
│   ├── ayumi/          AY-3-8910 emulation
│   └── msm5205/        Extracted MAME MSM5205 core
├── tests/
│   ├── cpu/            HD64180 instruction and MMU tests
│   ├── video/          VDP compositing and register tests
│   ├── audio/          Chip integration tests
│   └── integration/    Full-system ROM-based tests
├── shaders/
│   ├── scale.vert      Integer scaling vertex shader
│   ├── scale.frag      Pass-through / nearest-neighbor fragment shader
│   └── crt.frag        Optional CRT scanline/phosphor shader
├── CMakeLists.txt
└── vcpkg.json          Dependency manifest
```

---

## Feature List

### Emulation
- HD64180 CPU with full instruction set (Z80 + HD64180 extensions)
- MMU (CBAR/CBR/BBR) with correct 20-bit physical address translation
- Dual-channel DMA controller
- Dual 16-bit timer/counters
- Dual V9938 VDPs in Graphic 4 / 256×212 mode
- Full VDP command engine (HMMM, LMMM, LMMC, HMMC, LMMV, HMMV, LINE, etc.)
- Dual independent 16-entry 9-bit RGB palettes
- Per-chip sprite rendering (32 sprites / 8 per scanline per chip)
- VDP-A-over-VDP-B compositing with color-0 transparency
- YM2151 8-channel 4-operator FM synthesis
- AY-3-8910 3-channel PSG + noise + envelope
- MSM5205 4-bit ADPCM at 4/6/8 kHz
- Stereo audio mixing (YM2151 per-channel pan; AY + MSM5205 center)
- Two controller ports (8 buttons, active-low)
- ROM cartridge loading with bank switching (up to 960 KB)

### Display
- Integer display scaling (1× through monitor limit)
- Non-integer stretch mode
- Correct NTSC pixel aspect ratio mode (8:7) and square-pixel mode
- Fullscreen toggle
- Optional CRT shader (scanlines, phosphor glow)

### Workflow
- Save states (full snapshot of all component state)
- Quicksave / quickload slots (10 numbered slots, Shift+F1–F9 / F1–F9)
- Save state thumbnails (128×96 PNG) and metadata sidecars per slot
- Rewind (ring buffer, 30-second default at ~30 snapshots/sec, up to 60 sec)
- Deterministic input recording and replay (`.v8r` format, ROM-hash-anchored)
- Speed control (fast-forward, slow motion, frame advance, pause)
- Screenshot (native resolution and scaled)
- Audio recording to WAV
- Auto-resume: reopen last ROM and optionally restore last session on launch
- Recent ROM list and drag-and-drop ROM loading

### Headless / CLI
- `vanguard8_headless` binary: full emulator core without SDL/OpenGL/ImGui
- Run for N frames or until replay ends
- Framebuffer and audio SHA-256 hash verification with nonzero exit on mismatch
- Dump the current runtime frame to binary PPM, with an explicit fixture-only dump path
- `--expect-frame-hash` / `--expect-audio-hash` for CI regression gating
- ctest integration via replay files in `tests/replays/`

### Developer / Debugger
- CPU step / run / pause / run-to-cursor / frame advance
- Scanline and interrupt stepping: run to next H-blank, V-blank, INT0, INT1
- Breakpoints on PC, memory address (read/write), I/O port (read/write)
- Watchpoints with value conditions
- Breakpoints settable by symbol name when a symbol file is loaded
- HD64180-aware disassembler with PC arrow and symbol annotations
- Register file viewer (main + alternate sets, I/R, MMU regs, DMA, timers, ITC/IL)
- Memory map viewer (CA0 / Bank Window / CA1 with active bank indicator)
- Hex editor for RAM, ROM banks, VDP-A VRAM, VDP-B VRAM
- VRAM viewer (palette-decoded display of each chip's VRAM)
- VDP register dump (both chips, all registers including command registers)
- Palette viewer (both 16-entry palettes with 9-bit RGB values)
- Sprite overlay (bounding boxes from SAT for both VDPs)
- Layer toggle (VDP-A bg, VDP-A sprites, VDP-B bg, VDP-B sprites)
- Audio register inspector (live state for all three chips)
- Interrupt log (ring buffer with cycle timestamp and source identification)
- Execution trace (recent PC history with decoded instructions)
- Trace-to-file (current repo: headless CLI writer over the covered CPU
  stepping surface; full live desktop trace panel/file piping remains future UI work)
- ROM bank tracker (BBR write log with timestamps)
- Per-chip audio mute and volume trim
- Performance overlay (emulation speed, frame time, audio buffer fill, host CPU)
- Symbol file support (flat `HEXADDR LABEL` format; auto-loaded alongside ROM)
- Game debug console (I/O port hook for game-side debug print output; dev-only, off by default)

# Vanguard 8 Emulator — Build System and Dependencies

## Build System

This document describes the intended build system and directory layout. The
files and targets below may not exist yet in a fresh documentation-only repo.

CMake 3.25 or later, using **vcpkg manifest mode** for third-party library
acquisition. vcpkg is added as a git submodule at `third_party/vcpkg` and
bootstrapped automatically during the CMake configure step via the vcpkg
toolchain file.

```
cmake -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

A `Debug` build enables verbose logging and disables optimizations. A `Release`
build enables `-O2` and silences trace-level log output. The ImGui debugger is
present in both — it is not a debug-only build feature.

---

## Compiler Requirements

- GCC 12+ or Clang 15+
- C++20 standard (`-std=c++20`)
- Linux only (x86-64)

---

## Third-Party Dependencies

### Managed by vcpkg (`vcpkg.json`)

| Library                     | Version  | Purpose                                          |
|-----------------------------|----------|--------------------------------------------------|
| sdl2                        | 2.28+    | Window, input, audio output                      |
| imgui[docking,sdl2-binding,opengl3-binding] | 1.90+ | Debugger UI              |
| tomlplusplus                | 3.4+     | Config file parsing                              |
| spdlog                      | 1.13+    | Structured logging                               |
| libsamplerate               | 0.2+     | Audio resampling (chip rates → SDL2 output rate) |
| catch2                      | 3.5+     | Test framework                                   |
| nativefiledialog-extended   | 1.1+     | Native OS file picker                            |

### Vendored in `third_party/` (not vcpkg-managed)

These are source-level inclusions, not prebuilt libraries. They are compiled
directly as part of the emulator build.

| Directory              | Source                    | License    | Purpose                     |
|------------------------|---------------------------|------------|-----------------------------|
| `third_party/z180/`    | MAME project              | BSD-3      | HD64180 / Z180 CPU core     |
| `third_party/nuked-opm/` | nukeykt/Nuked-OPM       | LGPL-2.1   | YM2151 FM synthesis          |
| `third_party/ayumi/`   | true-grue/ayumi           | MIT        | AY-3-8910 PSG emulation     |
| `third_party/msm5205/` | MAME project              | BSD-3      | MSM5205 ADPCM decode        |

#### MAME Core Extraction Notes

The Z180 core lives in MAME at `src/devices/cpu/z180/`. The MSM5205 core lives
at `src/devices/sound/msm5205.*`. Both depend on MAME's device framework
(`emu.h`, `device_t`, etc.). Extraction involves:

1. Copying the relevant `.cpp`/`.h` files into `third_party/`.
2. Replacing MAME framework calls (`device_t` lifecycle, `address_space`
   reads/writes, `devcb` callbacks) with a thin adapter layer that calls into
   the emulator's own `Bus` interface.
3. Preserving all timing logic unchanged — the goal is to keep the cycle
   counts and state machine exactly as MAME has them.

This adapter layer lives in `src/core/cpu/z180_adapter.cpp` and
`src/core/audio/msm5205_adapter.cpp` respectively. The extracted chip code
itself is not modified.

---

## CMakeLists Structure

```
CMakeLists.txt                  Top-level: project, toolchain, subdirs
src/CMakeLists.txt              Defines vanguard8_core, vanguard8_frontend, vanguard8_headless targets
src/core/CMakeLists.txt         Core library: CPU, bus, VDP, audio, I/O, replay, save state, symbols
src/frontend/CMakeLists.txt     Frontend: SDL2 window, OpenGL, ImGui shell, headless frontend
src/debugger/CMakeLists.txt     Debugger panels (linked into vanguard8_frontend only)
tests/CMakeLists.txt            Catch2 test executables + ctest replay integration
third_party/CMakeLists.txt      Vendored libraries as INTERFACE/STATIC targets
```

`vanguard8_frontend` links `vanguard8_core` and adds the SDL2/OpenGL/ImGui
frontend. `vanguard8_headless` links `vanguard8_core` with only the headless
frontend (no SDL window, no GL, no ImGui). The test executables link
`vanguard8_core` directly. The replay tests in `tests/replays/` are registered
as ctest entries that invoke `vanguard8_headless`.

---

## Directory Structure (Detailed)

```
Vanguard8/
├── CMakeLists.txt
├── vcpkg.json                        vcpkg dependency manifest
├── vcpkg-configuration.json          vcpkg baseline pinning
│
├── docs/
│   ├── spec/                         Hardware specification (source of truth)
│   └── emulator/                     This design documentation
│
├── src/
│   ├── main.cpp                      Entry point: parse args, init SDL2, run loop
│   │
│   ├── core/                         Platform-independent emulator core
│   │   ├── bus.hpp / bus.cpp         Memory and I/O routing
│   │   ├── emulator.hpp / emulator.cpp  Top-level: owns all components, main tick
│   │   ├── scheduler.hpp             Master-cycle scheduler / event queue
│   │   ├── save_state.hpp / .cpp     Serialise / deserialise all component state
│   │   ├── rewind.hpp / .cpp         Ring buffer of recent save states
│   │   ├── symbols.hpp / .cpp        Symbol table (load, lookup, annotate)
│   │   │
│   │   ├── cpu/
│   │   │   ├── z180_adapter.hpp      Adapter: MAME Z180 ↔ Bus interface
│   │   │   └── z180_adapter.cpp
│   │   │
│   │   ├── memory/
│   │   │   ├── sram.hpp              32 KB system SRAM
│   │   │   └── cartridge.hpp         ROM image + bank switching logic
│   │   │
│   │   ├── video/
│   │   │   ├── v9938.hpp / v9938.cpp  Single V9938 VDP instance
│   │   │   └── compositor.hpp         Per-pixel VDP-A-over-VDP-B mux
│   │   │
│   │   ├── audio/
│   │   │   ├── ym2151.hpp            Nuked-OPM wrapper
│   │   │   ├── ay8910.hpp            Ayumi wrapper
│   │   │   ├── msm5205_adapter.hpp   MAME MSM5205 adapter
│   │   │   └── audio_mixer.hpp       Mix + resample → SDL2 ring buffer
│   │   │
│   │   ├── io/
│   │   │   └── controller.hpp        Two controller ports, active-low decoding
│   │   │
│   │   └── replay/
│   │       ├── recorder.hpp / .cpp   Per-frame input capture → .v8r file
│   │       └── replayer.hpp / .cpp   Per-frame input playback from .v8r file
│   │
│   ├── frontend/
│   │   ├── app.hpp / app.cpp         Top-level frontend: owns window, loop
│   │   ├── sdl_window.hpp            SDL2 window + OpenGL context creation
│   │   ├── display.hpp / display.cpp  OpenGL framebuffer, scaling, shader
│   │   ├── input.hpp / input.cpp     SDL2 event → controller port state (P1+P2, gamepad)
│   │   ├── headless.hpp / headless.cpp  Headless frontend: no window/GL/ImGui
│   │   └── perf_overlay.hpp          Performance overlay (speed, frame time, audio fill)
│   │
│   └── debugger/
│       ├── debugger.hpp / debugger.cpp  ImGui shell, panel manager, toggle
│       ├── cpu_panel.hpp             Registers, disassembler, step controls (incl. scanline/IRQ step)
│       ├── memory_panel.hpp          Hex editor: RAM / ROM / VRAM
│       ├── vdp_panel.hpp             VDP register dump, palette, sprite overlay
│       ├── audio_panel.hpp           Per-chip register state, waveform view
│       ├── interrupt_panel.hpp       Interrupt log ring buffer
│       ├── trace_panel.hpp           Execution trace (recent PC + decoded insns; trace-to-file)
│       └── bank_panel.hpp            ROM bank tracker (BBR write log)
│
├── third_party/
│   ├── CMakeLists.txt
│   ├── z180/                         Extracted MAME Z180 core
│   ├── nuked-opm/                    YM2151 core
│   ├── ayumi/                        AY-3-8910 core
│   ├── msm5205/                      Extracted MAME MSM5205 core
│   └── vcpkg/                        vcpkg submodule
│
├── tests/
│   ├── CMakeLists.txt
│   ├── cpu/
│   │   ├── test_mmu.cpp              MMU boot mapping, illegal BBR, bank switching
│   │   ├── test_instructions.cpp     HD64180-specific instructions (MLT, TST, etc.)
│   │   └── test_interrupts.cpp       INT0 IM1 dispatch, INT1 vectored dispatch
│   ├── video/
│   │   ├── test_compositor.cpp       VDP-A color-0 transparency, mux priority
│   │   └── test_vdp_registers.cpp    Register read/write, status register select
│   ├── audio/
│   │   └── test_msm5205.cpp          ADPCM nibble feed, /VCLK interrupt timing
│   ├── integration/
│   │   └── test_boot.cpp             Power-on MMU reconfiguration sequence
│   └── replays/                      Input replay files for system-level regression tests
│       └── README.md                 Documents each replay file and its expected hash
│
└── shaders/
    ├── scale.vert                    Vertex shader (fullscreen quad)
    ├── scale.frag                    Nearest-neighbor / integer scale fragment
    └── crt.frag                      CRT scanline + phosphor glow (optional)
```

---

## Configuration File

User settings are stored in `~/.config/vanguard8/config.toml`. The emulator
creates it with defaults on first run.

```toml
[display]
scale = 4               # Integer scale factor (1–8)
aspect = "ntsc"         # "ntsc" (8:7 pixel AR) or "square"
fullscreen = false
crt_shader = false

[audio]
sample_rate = 48000     # SDL2 output rate
buffer_ms = 32          # Audio latency target
ym2151_volume = 1.0
ay8910_volume = 1.0
msm5205_volume = 1.0

[input]
# Key bindings use SDL2 scancode names (https://wiki.libsdl.org/SDL_Scancode)
# Player 1 — keyboard
p1_up     = "Up"
p1_down   = "Down"
p1_left   = "Left"
p1_right  = "Right"
p1_a      = "Z"
p1_b      = "X"
p1_select = "RShift"
p1_start  = "Return"

# Player 2 — keyboard (defaults assume numpad / WASD)
p2_up     = "W"
p2_down   = "S"
p2_left   = "A"
p2_right  = "D"
p2_a      = "Q"
p2_b      = "E"
p2_select = "Tab"
p2_start  = "Backspace"

# Gamepad mapping — SDL2 GameController button names
# Applied to any connected gamepad; first gamepad = P1, second = P2
gamepad_a      = "a"        # face button
gamepad_b      = "b"        # face button
gamepad_up     = "dpup"
gamepad_down   = "dpdown"
gamepad_left   = "dpleft"
gamepad_right  = "dpright"
gamepad_select = "back"
gamepad_start  = "start"

# Emulator hotkeys (separate from in-game controls)
hotkey_debugger        = "F1"
hotkey_perf_overlay    = "P"
hotkey_fast_forward    = "Tab"     # hold
hotkey_rewind          = "R"       # hold
hotkey_frame_advance   = "F6"
hotkey_screenshot      = "F12"
hotkey_fullscreen      = "F11"
hotkey_pause           = "Escape"

[save_states]
# Quicksave/quickload — Shift+F1–F9 saves, F1–F9 loads, slot 0 via UI only
thumbnail = true        # save 128×96 PNG alongside each state

[rewind]
enabled       = true
duration_secs = 30      # 10–60
mute_audio    = false

[session]
# Auto-resume on launch
restore_last_rom   = true
restore_last_state = false   # restore save state from previous session

[debugger]
show_on_start = false
font_size = 13

[dev]
# Non-hardware developer features. All default off.
# When disabled these do not alter the emulated I/O surface.
console = false     # port 0xFE debug output (open bus when false)
```

---

## Build Targets

| Target                 | Description                                                          |
|------------------------|----------------------------------------------------------------------|
| `vanguard8_frontend`   | Main emulator executable (SDL2 + OpenGL + ImGui frontend)            |
| `vanguard8_headless`   | Headless emulator binary (no SDL window / OpenGL / ImGui)            |
| `vanguard8_tests`      | Catch2 test runner                                                   |
| `vanguard8_core`       | Static library (core without frontend; linked by both executables)   |

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake

# Build all
cmake --build build -j$(nproc)

# Run tests (includes replay-based regression tests in tests/replays/)
ctest --test-dir build --output-on-failure

# Run emulator
./build/vanguard8_frontend path/to/game.rom

# Run headless for N frames and verify framebuffer hash
./build/vanguard8_headless --rom game.rom --frames 3600 \
  --expect-frame-hash 3600 <known-hash>
```

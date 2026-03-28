# Vanguard 8 Emulator — Debugger

## Overview

Target desktop shape:
- The debugger is a set of Dear ImGui panels that attach directly to emulator
  state and can be toggled at runtime with a key (default: F1).

Current repo state:
- The implemented debugger is a snapshot/control layer (`DebuggerShell`,
  panel snapshots, breakpoint/watchpoint logic, interrupt/bank logs) that does
  not yet create a Dear ImGui context or render visible on-screen panels.
- This document describes the intended live desktop debugger once the desktop
  GUI backend from `docs/emulator/10-desktop-gui-audit.md` is implemented.

The debugger never owns emulator state — it holds const references or raw
pointers to the components it inspects. Writes (e.g., memory edits,
breakpoint configuration) go through a command queue that the main loop
drains between emulation steps, avoiding races.

---

## ImGui Setup

Current limitation:
- The ImGui setup below is not live in the current tree. The repo currently
  stores debugger layout metadata and render snapshots only.

The docking branch of Dear ImGui is used, which allows debugger panels to be
docked into a layout and resized freely. The layout is persisted to
`~/.config/vanguard8/imgui.ini` automatically by ImGui.

ImGui renders in the same OpenGL context as the game output, in the same
frame. The game's composited output is drawn to the lower-left of the window;
ImGui panels occupy the remaining space (or float over the game view if the
user prefers).

```cpp
void Debugger::render(const EmulatorState& state) {
    if (!visible_) return;

    ImGui::NewFrame();
    render_main_menu_bar();
    render_cpu_panel(state.cpu);
    render_memory_panel(state.bus);
    render_vdp_panel(state.vdp_a, state.vdp_b, state.compositor);
    render_audio_panel(state.ym2151, state.ay8910, state.msm5205);
    render_interrupt_panel(state.interrupt_log);
    render_trace_panel(state.trace_buffer);
    render_bank_panel(state.bank_log);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
```

Each panel is a separate `bool visible` flag — panels can be independently
shown or hidden from the main menu bar or with keyboard shortcuts.

---

## CPU Panel

Shows all CPU registers, a disassembler, and execution controls.

### Register Display

```
Main registers:        AF  BC  DE  HL
Alternate registers:   AF' BC' DE' HL'
Index registers:       IX  IY
Special:               SP  PC  I   R

MMU:                   CBAR  CBR  BBR
                       → CA0: 0x0000–0x3FFF
                       → Bank: 0x4000–0x7FFF (BBR=0x04, bank 0)
                       → CA1: 0x8000–0xFFFF

DMA Ch0:               SAR0  DAR0  BCR0  (active/idle)
DMA Ch1:               MAR1  IAR1  BCR1  (active/idle)
Timers:                TCR0  TCR1
INT control:           ITC   IL
Flags (F decoded):     S Z H P/V N C
```

MMU registers are decoded to show the current logical→physical mapping for
each region, and which ROM bank is active. Illegal BBR values are highlighted
in red.

### Disassembler

Shows 20 instructions centered on the current PC. The PC arrow (`→`) marks
the next instruction to execute. Jump targets are shown as both address and
offset. HD64180-specific mnemonics (`MLT`, `IN0`, `TST`, etc.) are used.

Clicking an address in the disassembler sets a PC breakpoint.

### Execution Controls

| Control            | Keyboard   | Action                                                      |
|--------------------|------------|-------------------------------------------------------------|
| Step               | F10        | Execute one instruction                                     |
| Step Over          | F11        | Step, but treat CALL as one unit                            |
| Continue           | F5         | Run until next breakpoint or pause                          |
| Pause              | F5 / Esc   | Halt execution                                              |
| Run to Cursor      | —          | Run until PC reaches clicked disassembler line              |
| Frame Advance      | F6         | Run exactly one frame, then pause                           |
| Step to H-blank    | F7         | Run until the next H-blank event boundary                   |
| Step to V-blank    | Shift+F7   | Run until the next V-blank event boundary                   |
| Run to Next INT0   | F8         | Run until INT0 is asserted (any source)                     |
| Run to Next INT1   | Shift+F8   | Run until INT1 is asserted (MSM5205 /VCLK)                 |

**Scanline / interrupt stepping** is implemented via the event scheduler: the
main loop injects a one-shot synthetic event at the target boundary and runs
until it fires. This means the CPU always stops at the exact master-cycle
boundary, not just at the end of an instruction that crosses it.

"Step to H-blank" and "Step to V-blank" advance to the next occurrence within
the current or next frame. If the emulator is already at or past a boundary in
the current frame it steps to the corresponding boundary in the following frame.

### Breakpoints

Breakpoints are listed in a sub-panel. Each entry shows:
- Type: PC / Mem Read / Mem Write / IO Read / IO Write
- Address (and optional value condition for watchpoints)
- Enabled/disabled toggle

New breakpoints are added by:
- Clicking an address in the disassembler (PC breakpoint)
- Right-clicking an address in the memory hex editor (access breakpoint)
- Typing an address into the breakpoint add dialog

---

## Memory Panel

A hex editor with region selection. Regions available:

| Region         | Size   | Description                           |
|----------------|--------|---------------------------------------|
| SRAM           | 32 KB  | System RAM (physical 0xF0000–0xF7FFF) |
| ROM Fixed      | 16 KB  | Cartridge fixed region (physical 0x00000–0x03FFF) |
| ROM Bank N     | 16 KB  | Any of the 59 switchable banks (select by bank number) |
| VDP-A VRAM     | 64 KB  | VDP-A video RAM                       |
| VDP-B VRAM     | 64 KB  | VDP-B video RAM                       |

The hex editor shows address, hex bytes, and ASCII. Edits are queued as
commands and applied between emulation steps.

A logical address view mode shows the current MMU-mapped view of the 64 KB
logical address space, coloring each region (CA0 / Bank Window / CA1) so the
current mapping is visually clear.

---

## VDP Panel

Three sub-panels: Registers, Palette, and VRAM Viewer. Applies to whichever
VDP is selected (A or B toggle).

### Register Sub-Panel

Displays all V9938 registers in groups:

- **Mode/Control** (R#0–R#9, R#11)
- **Scroll** (R#19, R#23, R#26, R#27) — R#26/R#27 shown with "unverified" warning
- **Command** (R#32–R#46) with decoded field names (SX, SY, DX, DY, NX, NY, CLR, ARG, CMD)
- **Status** (S#0–S#6) decoded: VB flag, collision flag, CE flag, FH flag, collision coords
- **Active command:** when CE bit is set, shows the currently executing command opcode and progress

### Palette Sub-Panel

Displays both VDP-A and VDP-B palettes side-by-side. Each of the 16 entries
shows a color swatch plus the raw 9-bit RGB value (R:G:B in decimal and hex).
The border/backdrop color (R#7) is marked.

### VRAM Viewer Sub-Panel

Renders the selected VDP's VRAM as a palette-decoded image. The display
area region (0x0000–0x69FF) is outlined. The sprite pattern bank and tile
bank regions are labeled. A toggle switches between decoded view and raw
nibble hex view.

### Sprite Overlay

When enabled, the compositor draws axis-aligned rectangles over the game
output showing each sprite's bounding box. Colors distinguish VDP-A sprites
(blue) from VDP-B sprites (orange). Sprite number and pattern index are
shown as a tooltip on hover.

---

## Audio Panel

Three sub-panels, one per chip.

### YM2151 Sub-Panel

- Per-channel summary table: algorithm, feedback, L/R pan, key on/off state
- Per-channel operator detail (expandable): DT1, MUL, TL, KS, AR, D1R, D2R, D1L, RR
- Global: LFO frequency, waveform, noise enable/frequency
- Timer state: Timer A value/period, Timer B value/period, IRQ flags
- Per-channel mute toggle
- Master volume trim slider

### AY-3-8910 Sub-Panel

- Channel A/B/C: period, volume, tone enable, noise enable
- Noise period
- Envelope period, shape, generator state
- Per-channel mute toggle
- Master volume trim slider

### MSM5205 Sub-Panel

- Sample rate selection (current: 4/6/8 kHz or stopped)
- ADPCM decoder state: step index, current predictor value
- /VCLK counter (cycles until next INT1)
- Reset flag
- Mute toggle, volume trim slider

### Waveform Visualizer

A small oscilloscope-style plot (ImGui::PlotLines) showing the last 256
output samples from each chip. Updates every frame. Useful for verifying
that a chip is producing sound and identifying clipping or silence.

---

## Interrupt Panel

A ring buffer showing the last 256 interrupt events. Columns:

| Column       | Content                                                    |
|--------------|------------------------------------------------------------|
| Cycle        | Master cycle count at assertion time                       |
| Frame/Line   | Frame number and scanline at assertion time                |
| Line         | INT0 or INT1                                               |
| Source       | V-blank / H-blank / YM2151-TimerA / YM2151-TimerB / VCLK  |
| Handled      | Whether the handler ran and cleared the flag               |
| Latency      | CPU cycles from assertion to handler entry                 |

A "pause on interrupt" option halts emulation the next time a selected
interrupt source fires.

---

## Execution Trace Panel

A ring buffer of the last 1024 executed instructions. Columns:

| Column    | Content                                    |
|-----------|--------------------------------------------|
| PC        | Logical PC at instruction fetch            |
| Physical  | Physical address after MMU translation     |
| Bank      | Active ROM bank at fetch time (if in bank window) |
| Bytes     | Raw opcode bytes (up to 4)                 |
| Mnemonic  | Decoded HD64180 instruction                |
| Cycles    | T-states consumed                          |

Clicking a row in the trace sets a breakpoint at that PC.

The ring buffer is only populated when the trace panel is open (to avoid
overhead when not in use). A "freeze trace" button pauses the buffer so the
user can scroll through it without it being overwritten.

---

## ROM Bank Tracker Panel

Logs every write to the BBR register (port OUT0 0x39). Columns:

| Column  | Content                                        |
|---------|------------------------------------------------|
| Cycle   | Master cycle of the OUT0 write                 |
| Frame   | Frame number                                   |
| PC      | CPU PC when the bank switch occurred           |
| BBR     | New BBR value                                  |
| Bank    | Decoded bank number (BBR/4 − 1)               |
| Legal   | Warning if BBR ≥ 0xF0                         |

Useful for profiling bank switch frequency and tracking unexpected switches.

---

## Game Debug Console

**This feature is disabled by default.** When disabled, port `0xFE` behaves
as open bus (returns 0xFF, writes are ignored, unrecognized-port warning is
logged) — identical to every other unmapped port. Enabling the debug console
does not change the emulated hardware surface in any spec-defined area.

When explicitly enabled via the `[dev]` config section or the `--dev-console`
command-line flag, writing a byte to port `0xFE` sends it as a character to
the debug console panel. This port has no hardware equivalent on the Vanguard 8.

```toml
[dev]
console = false     # default; set true to enable port 0xFE debug output
```

The console panel shows a scrollable log of received characters with
timestamps. A clear button resets it.

Game code that targets only the emulator can use this port during development.
Because the port is non-functional by default (open bus), a ROM that writes
to it will not break on hardware or on an emulator with the feature disabled —
the write is simply a no-op in both cases. Games intended for the Vanguard 8
hardware should not depend on this port for correctness.

---

## Symbol File Support

The debugger can load a symbol file to annotate the disassembler, memory panel,
and execution trace with human-readable label names.

### File Format

A flat text file, one symbol per line:

```
HEXADDR LABEL
; comment lines begin with semicolons and are ignored
```

Examples:

```
0000 reset_vector
0038 int0_handler
0050 main_loop
4000 bank_start
; ROM data section
8000 sram_base
8100 player_x
8101 player_y
```

- `HEXADDR` is a 4-digit (or shorter) hex address in the CPU's 16-bit logical
  address space. No `0x` prefix.
- `LABEL` is an alphanumeric identifier (underscores allowed; spaces not allowed).
- Bank-qualified symbols are not yet supported in v1 — symbols apply to the
  logical address regardless of which ROM bank is active at that address.

### Loading

Symbols are loaded from the main menu (`Debug → Load Symbol File…`) or
automatically on ROM load if a `.sym` file with the same base name exists in
the same directory as the ROM.

```
game.rom      → auto-loads game.sym if present
```

### Annotations

Once a symbol file is loaded:

- **Disassembler**: jump and call targets show the symbol name alongside the
  address (e.g., `CALL 0x0050  ; main_loop`). Addresses in the listing that
  match a symbol show the label instead of the raw address where space permits.
- **Memory panel**: the SRAM region labels known addresses with their symbol
  names in a side column when the logical address view is active.
- **Execution trace**: the PC column shows `label+offset` when the PC is within
  a known symbol range (falls back to bare address when no symbol matches).
- **Breakpoints**: breakpoints can be set by typing a label name rather than a
  hex address in the add-breakpoint dialog.

Symbols can be unloaded from the same menu. The symbol table is not saved with
save states — it is a debugging aid, not emulator state.

---

## Performance Overlay

A lightweight always-on-top overlay showing real-time emulation metrics.
Toggled independently from the full debugger (default key: `P`).

```
┌─────────────────────────────────────┐
│ Speed:   100.2%   Frame: 16.71 ms   │
│ Audio:   ████████░░  78% filled     │
│ CPU:     host 3.1%                  │
└─────────────────────────────────────┘
```

| Field        | Description                                                        |
|--------------|--------------------------------------------------------------------|
| Speed        | Emulation speed as a percentage of real time (100% = 59.94 Hz)    |
| Frame time   | Wall-clock time taken to run the last emulated frame               |
| Audio fill   | SDL2 ring buffer fill level (low values risk underrun)             |
| Host CPU     | Host process CPU usage (from `/proc/self/stat`, sampled once/sec)  |

During fast-forward the speed figure reflects the uncapped frame rate. During
slow motion it shows the fraction in use (e.g., `25%`).

The overlay is rendered by ImGui in an always-visible, non-dockable window at
a fixed corner of the screen (configurable). It is separate from the main
debugger toggle so it can remain visible during normal play.

---

## Trace-to-File

For long bug hunts where the covered in-memory debugger surfaces are
insufficient, the current repo can write decoded instruction history directly
to a file from the command line without requiring a live ImGui panel.

### Activation

Current repo path:

```bash
vanguard8_headless --rom game.rom --trace trace.log --trace-instructions 256
```

Current precision boundary:
- This milestone-14 path traces the covered CPU stepping surface, not the full
  live desktop runtime loop.
- The file is written synchronously and the command exits after the requested
  instruction budget is captured.
- A live on-screen trace panel and background file-streaming path remain future
  UI work.

### File Format

Plain text, one instruction per line:

```
STEP         PC    PHYS   BANK  BYTES        MNEMONIC
000000000000  0000  00000     F  76           HALT
000000000001  0038  00038     F  ED 56        IM 1
000000000002  003A  0003A     F  FB           EI
...
```

`STEP` is a monotonically increasing instruction index in the captured trace.
`BANK` is `F` for the fixed ROM region, `S` for SRAM-backed logical space, or
the decoded bank number while the PC is in the cartridge bank window.

### Stopping

The command stops after the requested instruction budget is reached or when the
CPU halts. The final line count is printed to stdout.

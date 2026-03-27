# Vanguard 8 Emulator — Video System

## Architecture Summary

The full video design is three objects: `VDP-A`, `VDP-B`, and `Compositor`.
Milestone 4 implemented only the single-VDP subset needed for the first
Graphic 4 frame path. Milestone 5 adds the first Vanguard 8-specific video
behavior: dual-VDP compositing, covered Mode-2 sprite rendering, VDP-A-only
interrupt semantics, and layer toggles.

For the current implementation, `Bus` owns both VDP instances and routes ports
`0x80-0x87` directly to them. `Emulator` drives scanline and V-blank timing
into those VDPs from the scheduler and mirrors only VDP-A's enabled interrupt
sources onto CPU `INT0`.

In the current repo, the display pipeline ends at a deterministic upload buffer
in `src/frontend/display.*` rather than a real SDL/OpenGL binding. The shader
files remain part of the repo asset layout, but the verified milestone-5 path
is:

`VDP-A + VDP-B -> compositor -> RGB888 framebuffer -> upload buffer -> headless PPM dump`

---

## V9938 Instance (`src/core/video/v9938.hpp`)

One instance represents one physical V9938 chip. VDP-A and VDP-B are
identical in capability; they differ only in their position in the compositor
stack and in interrupt wiring.

### State

```cpp
struct V9938 {
    uint8_t  vram[65536];          // 64 KB VRAM
    uint8_t  reg[64];              // R#0–R#46 (command regs use R#32–R#46)
    uint8_t  status[10];           // S#0–S#9
    uint8_t  palette[16][2];       // 16 entries × 2 bytes (9-bit RGB packed)
    uint16_t vram_addr;            // Current VRAM address pointer
    uint8_t  vram_latch;           // Address write latch (two-write protocol)
    bool     addr_latch_full;      // True after first address byte written
    uint8_t  reg_latch;            // Register indirect write latch
    uint8_t  status_reg_select;    // R#15 value (selects which S# to read)
    uint8_t  bg_line_buffer[256];  // Background color indices for current scanline
    uint8_t  sprite_line_buffer[256]; // Sprite color indices; 0xFF = no sprite pixel

    // Command engine
    CommandState cmd;              // Active VDP command state machine

    // Scanline rendering
    uint8_t  line_buffer[256];     // Current scanline, 4bpp color indices
    int      current_scanline;
};
```

### Port Interface

All VDP access goes through I/O ports. The bus calls these methods:

```cpp
uint8_t read_data();             // Port 0x80 / 0x84: VRAM data read
void    write_data(uint8_t);     // Port 0x80 / 0x84: VRAM data write
uint8_t read_status();           // Port 0x81 / 0x85: reads S#(status_reg_select)
void    write_control(uint8_t);  // Port 0x81 / 0x85: address or register command
void    write_palette(uint8_t);  // Port 0x82 / 0x86: palette entry write
void    write_register(uint8_t); // Port 0x83 / 0x87: register indirect write
```

The two-byte control write protocol (address or register):
- First byte written to 0x81: latched
- Second byte written to 0x81:
  - If bit 7 = 0 and bit 6 = 0: VRAM read address (14-bit, bits 13:0)
  - If bit 7 = 0 and bit 6 = 1: VRAM write address
  - If bit 7 = 1: register write (bits 5:0 = register number, then first byte = data)

Reading status (0x81) also resets the address latch (clears `addr_latch_full`).

### Scanline Rendering

`tick_scanline(int line)` is called once per scanline from the main loop.
It renders the line into background and sprite buffers, then combines them into
`line_buffer` using the current VRAM and register state.

#### Mode decode and covered renderers

The milestone-11 renderer now decodes the documented V9938 mode bits directly:

```
R#0 bit 3 = M5
R#0 bit 2 = M4
R#0 bit 1 = M3
R#1 bit 4 = M1
R#1 bit 3 = M2
```

Covered scanline modes:

- **Graphic 4**: default bitmap path
- **Graphic 3**: fixed-layout tile path using the repo's documented
  `0x0000-0x42FF` table placement

Unsupported display modes must not silently fall back to a neighboring mode.
The narrow emulator path fills the background with the backdrop color until the
mode is implemented from spec.

#### Graphic 4 rendering (primary/default mode)

```
Background:
  For each x in 0..255:
    byte_addr = (line + R#23) % 256 * 128 + x / 2
    byte = vram[byte_addr]
    if x is even: pixel = byte >> 4
    else:         pixel = byte & 0x0F
    line_buffer[x] = pixel

Sprites:
  Evaluate up to 8 visible sprites for this scanline (Sprite Mode 2).
  For each visible opaque sprite pixel, overwrite line_buffer[x] with the
  sprite color and leave lower-priority sprites behind it.
  Track 5th-sprite overflow (S#0 bit 6) and same-chip collision (S#0 bit 5,
  S#3-S#6 coordinates) for the covered milestone-5 cases.
```

Vertical scroll (R#23) wraps within the 256-line VRAM page.

Horizontal scroll (R#26/R#27): **not implemented**. Marked as unspecified in
`docs/spec/02-video.md` pending V9938 Technical Data Book verification.
A `TODO(spec)` stub exists in the renderer.

Sprite layout note:
- The covered milestone-5 sprite path uses the recommended Graphic 4 sprite
  layout from `docs/spec/02-video.md`:
  pattern generator at `0x6A00`, color table at `0x7A00`, and SAT at `0x7C00`.
- General register-relocated sprite table addressing is intentionally deferred
  until the repo docs carry the exact alignment detail needed to implement it
  without guessing.

#### Graphic 3 rendering (fixed-layout milestone-11 path)

```
Background:
  If line is 0..191:
    tile_row    = line / 8
    row_in_tile = line % 8
    bank        = tile_row / 8
    name        = VRAM[0x0000 + tile_row * 32 + tile_col]
    pattern     = VRAM[0x0300 + bank * 0x0800 + name * 8 + row_in_tile]
    colors      = VRAM[0x1800 + bank * 0x0800 + name * 8 + row_in_tile]
    fg          = colors >> 4
    bg          = colors & 0x0F
  Else:
    output backdrop color from R#7
```

The Graphic 3 sprite path reuses the covered 8x8 Sprite Mode 2 fetch logic,
but switches to the documented Graphic 3 fixed layout:

- pattern generator at `0x3000`
- color table at `0x4000`
- SAT at `0x4200`

Current precision boundary:

- The documented `32x24` tile map is implemented directly.
- The repo still does not define the extra LN=1 background fetch rule for
  lines `192-211`; those lines intentionally render the backdrop color.
- `R#23` vertical scroll remains locked to the existing Graphic 4 path until
  Graphic 3 scroll behavior is written into the spec without ambiguity.

### VDP Command Engine

VDP commands (R#46) execute asynchronously relative to the CPU. In the
emulator, command progress is advanced on the same master-cycle timeline as
the CPU — not batched to scanline boundaries. This ensures `S#2.CE` is
cleared at the correct cycle, so CPU polling of CE produces accurate results
even for short commands that complete well within a single scanline.

Command execution rate for HMMM (`docs/spec/02-video.md`):
```
cycles = 64 + (NX × NY × 3) master clock cycles
```

For an 8×8 HMMM tile blit: `64 + (4 × 8 × 3)` = 160 master cycles (~22 µs).
That is roughly one-sixth of a scanline. Under a scanline-batched model,
CE would remain asserted for the rest of the scanline after the command
actually completes — the event-scheduled model eliminates that error.

`advance_command(elapsed_master_cycles)` is called from `run_cpu_and_audio_until()`
after each CPU instruction:

```cpp
void V9938::advance_command(int master_cycles) {
    if (!cmd.active) return;
    cmd.cycles_remaining -= master_cycles;
    if (cmd.cycles_remaining <= 0) {
        cmd.active = false;
        status[2] &= ~0x01;   // clear CE in S#2
    }
}
```

The CE flag in S#2 bit 0 reflects whether a command is still in progress.
Games that poll CE before issuing a new command will see accurate timing.

Current precision boundary:

- `HMMM` uses the documented cycle formula above and is the milestone-8 timing
  lock for `CE` polling tests.
- Other covered command opcodes remain asynchronous, but the repo does not yet
  carry per-opcode cycle formulas for them. Their `CE` lifetime must still
  cross a real command-advance boundary; do not invent more precise timings
  until the spec is expanded with source-backed values.

Supported commands: STOP, POINT, PSET, SRCH, LINE, LMMV, LMMM, LMMC, HMMV,
HMMM, YMMM, HMMC.

Relevant register/status details locked by `docs/spec/02-video.md`:

- `R#45` (`ARG`) bits used here are `DIY`, `DIX`, `EQ`, and `MAJ`
- `S#2` bit 7 is `TR` (transfer ready for CPU-streamed commands)
- `S#2` bit 4 is `BD` (SRCH boundary detect)
- `S#7` carries the sampled color result for `POINT`
- `S#8`/`S#9` carry the x-coordinate result for `SRCH`
- `HMMC` and `LMMC` stream CPU data through the normal VDP data port while
  `TR` is asserted

Milestone-8 boundary:

- execute the documented command opcode set
- keep `CE` asynchronous on the master-cycle timeline
- map command coordinates through the already-supported **Graphic 4** packing
  model only

Graphic 5/6/7 command packing remains deferred to the later mode-expansion
milestone. If code would need those layouts, stop and extend the spec first.

### Interrupt Output

```cpp
bool int_pending() const;
bool vblank_irq_pending() const;
bool hblank_irq_pending() const;
```

Per-chip `/INT` asserts when:
- V-blank flag set AND R#1 bit 5 (V-blank IRQ enable) is set
- H-blank flag set AND R#0 bit 4 (H-blank IRQ enable) is set

Reading S#0 clears the V-blank flag. Reading S#1 clears the H-blank flag.
Both reads de-assert INT if no other source remains pending.

System wiring note:
- `Bus` mirrors `VDP-A::vblank_irq_pending()` and
  `VDP-A::hblank_irq_pending()` onto CPU `INT0`.
- `VDP-B` keeps the same internal status/IRQ behavior, but its `/INT` output is
  intentionally not forwarded to the CPU.

---

## Compositor (`src/core/video/compositor.hpp`)

Milestone 5 uses the dual-VDP compositor directly and keeps the single-VDP
path only as a convenience for narrow tests.

```cpp
// Called once per active scanline after both VDPs have been ticked
void composite(
    const uint8_t*  line_a,        // VDP-A line buffer (256 4bpp indices)
    const uint8_t   palette_a[16][2],
    const uint8_t*  line_b,        // VDP-B line buffer
    const uint8_t   palette_b[16][2],
    uint8_t         out_rgb[256][3],   // Output: 24-bit RGB
    const LayerMask& mask              // Debugger layer toggles
);
```

Per-pixel logic:

```cpp
for (int x = 0; x < 256; ++x) {
    uint8_t idx_a = line_a[x];
    uint8_t idx_b = line_b[x];

    // VDP-A color index 0 = transparent (YS pin high → mux selects VDP-B)
    const uint8_t (&entry)[2] = (idx_a == 0 || mask.hide_vdp_a)
                                  ? palette_b[idx_b]
                                  : palette_a[idx_a];

    // Expand 9-bit RGB (3:3:3) to 8-bit per channel
    out_rgb[x][0] = expand3to8((entry[0] >> 4) & 0x7);  // R
    out_rgb[x][1] = expand3to8( entry[0]       & 0x7);  // G
    out_rgb[x][2] = expand3to8( entry[1]       & 0x7);  // B
}
```

`expand3to8(v)` maps a 3-bit value to 8 bits: `(v << 5) | (v << 2) | (v >> 1)`
(replicates high bits into low bits to fill the full 8-bit range cleanly).

### Layer Mask (Debugger)

```cpp
struct LayerMask {
    bool hide_vdp_a_bg      = false;
    bool hide_vdp_a_sprites = false;
    bool hide_vdp_b_bg      = false;
    bool hide_vdp_b_sprites = false;
};
```

When a layer is hidden, the compositor drops that layer from the per-pixel
selection. Hiding `VDP-A` background makes those pixels transparent to
`VDP-B`; hiding sprite layers removes only sprite contribution while leaving
the background layer intact.

---

## Display Pipeline (`src/frontend/display.hpp`)

```
VDP-A background/sprites + VDP-B background/sprites
        │
  Dual-VDP compositor → 256×212 RGB888 framebuffer
        │
  Display upload buffer (`Display::upload_frame`)
        │
  Headless PPM dump / frontend digest output
```

### Milestone-4 Upload Surface

`Display` stores the most recently uploaded 256×212 RGB888 frame, exposes a
stable digest for tests, and can emit a binary PPM dump for regression assets.
This is the current verified stand-in for the eventual SDL/OpenGL upload path.

### GLSL Shaders

Two fragment shaders, selectable at runtime:

**`shaders/scale.frag` (default):**
Integer-scale pass-through. Samples the texture with nearest-neighbor
filtering. The vertex shader maps the texture to a centered quad sized to
the largest integer multiple that fits the window.

**`shaders/crt.frag` (optional):**
Adds scanline darkening (every other pixel row attenuated) and a mild
phosphor glow (bloom pass over bright pixels). Does not attempt to model
dot pitch or convergence. Purely cosmetic.

### Scaling Modes

| Mode           | Behavior                                                   |
|----------------|------------------------------------------------------------|
| Integer scale  | Largest N where 256×N ≤ window width and 212×N ≤ window height |
| Stretch        | Fill window, ignore aspect ratio                           |
| NTSC aspect    | Scale to correct 8:7 pixel AR (~292×212 logical, then scaled) |
| Square pixels  | Integer scale, square pixels (default)                     |

On a 4K display (3840×2160), integer scale 8 gives 2048×1696 — a comfortable
large window. Scale 10 would be 2560×2120 — also fits. The config default is 4.

---

## VRAM Viewer (Debugger)

The VRAM viewer renders the full 64 KB VRAM of either VDP as a decoded image
using the chip's current palette. It uses the same `expand3to8` path as the
compositor. The user can select which VDP to view, and optionally view raw
nibble data in a hex grid.

The sprite overlay draws axis-aligned rectangles over the composited output
at the positions read from each chip's Sprite Attribute Table.

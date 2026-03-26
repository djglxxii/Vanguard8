# Vanguard 8 — Video System Reference

## Architecture Overview

The Vanguard 8 video system uses two **Yamaha V9938** VDP chips running in
lockstep from the same master clock, with their pixel outputs composited
digitally by a bank of **74LS257 multiplexers**.

Each V9938 is a fully independent, fully capable VDP. Neither chip is restricted
in its feature set — both chips expose the complete V9938 mode set, all VRAM
tables, the hardware command engine, and the full interrupt system. The only
inherent difference between them is their position in the compositing priority
stack.

Game developers are free to configure each chip independently. The two chips
may run the same mode or different modes simultaneously. See
[Mixed-Mode Operation](#mixed-mode-operation) for considerations.

```
┌──────────┐    ┌──────────────────────┐
│  VDP-B   │───▶│  74LS257 Mux Bank    │───▶ Video Output (RGB/Composite)
│ (Layer B)│    │                      │
└──────────┘    │  Select: VDP-A YS pin│
                │  0 = VDP-A wins      │
┌──────────┐    │  1 = VDP-B wins      │
│  VDP-A   │───▶│                      │
│ (Layer A)│    └──────────────────────┘
└──────────┘
```

---

## Compositing Model

### Priority Stack (Back to Front)

```
1. VDP-B background layer         (rearmost)
2. VDP-B sprites
3. VDP-A background layer
4. VDP-A sprites                  (frontmost)
```

Sprites on VDP-B always appear behind all VDP-A content. Within each chip,
standard V9938 sprite priority rules apply (lower sprite number = higher
priority).

### Transparency and the YS Pin

The mux decision is driven entirely by VDP-A's **YS status pin**. When VDP-A
outputs a transparent pixel, YS goes high and the mux selects VDP-B's pixel
for that clock cycle. This is fully digital with no analog signal degradation,
and is correct for both RGB and composite output.

**What counts as transparent on VDP-A** depends on the active mode and the
TP control bit:

- In all color modes (Graphic 1–7): color index 0 is transparent when the
  **TP bit (R#8 bit 5)** is set. VDP-A must have TP set for compositing to
  work as intended.
- In Text modes (Text 1, Text 2): the background pixel is the border/backdrop
  color (R#7), not color index 0. YS behavior in text modes is governed by the
  same TP mechanism but the practical compositing result depends on how the
  backdrop interacts with the layer below. **Verify against the V9938 Technical
  Data Book before relying on Text mode compositing.**

VDP-B's TP setting is independent and does not affect the mux; it only controls
whether VDP-B's own backdrop is transparent relative to any VDP-B-internal
layering.

### Mixed-Mode Operation

Because the compositing depends only on VDP-A's YS pin, it is mode-agnostic.
VDP-A and VDP-B can run in different modes simultaneously. Practical
combinations include:

- **VDP-B in a tile mode (Graphic 1–3), VDP-A in a bitmap mode (Graphic 4–7):**
  VDP-B provides a hardware-scrolled tile background; VDP-A provides a bitmap
  overlay with transparency cut-outs.
- **Both chips in Graphic 4:** Maximum bitmap flexibility; software tile blitting
  on both layers via VDP hardware commands.
- **VDP-B in Graphic 4, VDP-A in Graphic 3:** VDP-B provides a bitmap background;
  VDP-A provides a tile-based foreground with Sprite Mode 2.

**Scanline synchronization:** Both VDPs run from the same master clock and
generate identical pixel timing. Configure both chips to the same active-line
count (R#9 LN bit) to keep V-blank timing aligned. Mismatched LN settings will
cause VDP-A and VDP-B to disagree on where the visible frame ends, which may
produce visible artifacts in the border region.

---

## System Display Parameters

These are the fixed hardware display parameters regardless of mode choice.

| Parameter         | Value                                         |
|-------------------|-----------------------------------------------|
| Refresh rate      | 59.94 Hz (NTSC)                               |
| Total scanlines   | 262 (including V-blank)                       |
| Active scanlines  | 212 (LN=1) or 192 (LN=0)                     |
| Output            | RGB, composite, RF                            |
| VRAM per chip     | 64 KB (8 × 4164 64Kbit DRAM)                 |

**212-line operation (R#9 bit 7 = 1, LN=1) is recommended** for all modes that
support it. It maximizes the active display area and is the baseline for the
VRAM layouts in this document.

---

## Supported Display Modes

The V9938 supports nine display modes selectable via mode bits M1–M5 spread
across R#0 and R#1. Both VDP chips support all nine modes equally. Exact mode
bit encoding is in the V9938 Technical Data Book; the table below uses the
mode names and MSX screen number as reference identifiers.

| Mode       | MSX Screen | Type   | Active Pixels   | bpp  | Sprite Mode | 212-line | VRAM (active area) |
|------------|------------|--------|-----------------|------|-------------|----------|--------------------|
| Text 1     | Screen 0   | Text   | 240×192 visible | 2    | None        | No       | ~1.2 KB            |
| Text 2     | Screen 0W  | Text   | 480×192 visible | 4    | None        | No       | ~3 KB              |
| Graphic 1  | Screen 1   | Tile   | 256×192         | 16   | Mode 1      | No       | ~4 KB (name table) |
| Graphic 2  | Screen 2   | Tile   | 256×192         | 16   | Mode 1      | No       | ~16 KB             |
| Graphic 3  | Screen 4   | Tile   | 256×212         | 16   | **Mode 2**  | Yes      | ~16 KB             |
| Graphic 4  | Screen 5   | Bitmap | 256×212         | 4    | Mode 2      | Yes      | 27,136 bytes       |
| Graphic 5  | Screen 6   | Bitmap | 512×212         | 2    | Mode 2      | Yes      | 27,136 bytes       |
| Graphic 6  | Screen 7   | Bitmap | 512×212         | 4    | Mode 2      | Yes      | 54,272 bytes       |
| Graphic 7  | Screen 8   | Bitmap | 256×212         | 8    | Mode 2      | Yes      | 54,272 bytes       |

**Notes:**
- Text modes display 6×8 pixel character cells. Text 1 shows 40 columns;
  Text 2 shows 80 columns. Neither supports sprites.
- Graphic 1 and 2 are 192-line only (LN bit has no effect in these modes).
- Graphic 3 is the tile mode with Sprite Mode 2; it is the natural companion
  for a VDP-A tile layer composited over a VDP-B bitmap.
- Graphic 5 and 6 are 512 pixels wide; these are effectively double-horizontal-
  resolution modes and require attention to horizontal timing.
- Graphic 6 and 7 each require ~54 KB of VRAM for the active framebuffer alone.
  With sprite tables and pattern storage also consuming VRAM, layout planning is
  critical. Both fit within 64 KB but leave limited headroom.
- **VDP hardware commands (R#32–R#46) are available in Graphic 3, 4, 5, 6, and
  7 only.** They are not available in Text 1, Text 2, Graphic 1, or Graphic 2.
- **Scroll registers R#23/R#26/R#27 are available in Graphic 4, 5, 6, and 7.**
  Availability in Graphic 3 and tile modes should be verified against the
  V9938 Technical Data Book.
- **128 KB VRAM features are not available.** Each VDP chip has exactly 64 KB.
  Page-flip operations or any feature that requires a second 64 KB VRAM page
  are not possible.

---

## Text Modes

### Text 1 (Screen 0)

Displays 40 columns × 24 rows of 6×8 pixel characters. Two colors only: the
foreground color and the backdrop color, both set via R#7. No sprites. The
display output is 240 pixels wide, centered within the 256-pixel output frame
with border pixels filling the sides.

VRAM tables (register-controlled base addresses):
- **Pattern Name Table** (R#2): 960 bytes (40×24 character indices)
- **Pattern Generator Table** (R#4): 2,048 bytes (256 patterns × 8 bytes each)

### Text 2 (Screen 0W)

Displays 80 columns × 24 rows of 6×8 pixel characters. Up to four colors via
a blink mechanism (odd/even character attribute bits). No sprites.

Text 2 is the highest character-density mode and is useful for system menus or
status overlays on VDP-A when compositing over a VDP-B game layer.

VRAM requirements for Text 2 (including blink attribute table) should be
verified against the V9938 Technical Data Book before committing to a VRAM
layout for this mode.

---

## Tile Modes (Graphic 1, 2, 3)

Tile modes use a hardware **Pattern Name Table** (the tile map) that indexes
into a **Pattern Generator Table** (the tile pixel data) and a **Color Table**
(per-tile or per-row color assignments). All tile modes support 256×8 pixel
pattern entries (32 columns × 24 rows of 8×8 tiles).

### Sprite Mode in Tile Modes

- **Graphic 1 and Graphic 2:** Sprite Mode 1 — each sprite has a single color
  applied uniformly to all opaque pixels. 32 sprites defined, 4 visible per
  scanline (hardware limit in Mode 1). See [Sprite Mode 1](#sprite-mode-1).
- **Graphic 3:** Sprite Mode 2 — per-row color, same as Graphic 4 and the
  other bitmap modes. 32 sprites, 8 visible per scanline.

### Graphic 1 (Screen 1)

256×192 tile display. Color is assigned per group of 8 patterns (one color
table byte covers 8 consecutive pattern numbers, giving one foreground and one
background color for all tiles sharing that pattern group).

VRAM tables (register-controlled):
- **Pattern Name Table** (R#2): 768 bytes (32×24 tile indices)
- **Pattern Generator Table** (R#4): 2,048 bytes (256 patterns × 8 bytes)
- **Color Table** (R#3): 32 bytes (one byte per 8-pattern group)
- **Sprite Attribute Table** (R#5/R#11): 128 bytes (32 sprites × 4 bytes, Mode 1)
- **Sprite Pattern Generator** (R#6): 2,048 bytes (256 patterns × 8 bytes) or
  512 bytes (64 patterns × 8 bytes) for 8×8 sprites

### Graphic 2 (Screen 2)

256×192 tile display. The 256×192 pixel area is divided into three vertical
banks of 256×64 pixels (8 rows of tiles each). Each bank has its own Pattern
Generator Table and Color Table, enabling per-8-pixel-row color control on
every tile. This effectively allows each 8-pixel-tall row of every tile to have
independent foreground and background colors.

VRAM tables:
- **Pattern Name Table** (R#2): 768 bytes
- **Pattern Generator Table** (R#4): 6,144 bytes (3 banks × 256 patterns × 8 bytes)
- **Color Table** (R#3): 6,144 bytes (3 banks × 256 patterns × 8 bytes color data)
- Sprite tables: same as Graphic 1 (Sprite Mode 1)

### Graphic 3 (Screen 4)

Identical to Graphic 2 in background tile layout, but:
- Supports **212 active lines** (LN bit effective)
- Uses **Sprite Mode 2** (per-row sprite color, 8 sprites per scanline)
- VDP hardware commands are available

Graphic 3 is the natural choice when a tile-mode VDP layer needs Sprite Mode 2
for rich multi-color sprites. It is directly compatible with Graphic 4 as a
companion layer on the other VDP.

VRAM tables: same structure as Graphic 2, plus the Sprite Color Table required
by Sprite Mode 2 (located at SAT base − 512 bytes, same as in Graphic 4).

---

## Bitmap Modes (Graphic 4, 5, 6, 7)

All bitmap modes store the display as a flat framebuffer in VRAM. There is no
hardware tile engine; the VDP hardware command engine (HMMM, LMMM, etc.) serves
as the blitter for software-managed tile rendering. All bitmap modes support
Sprite Mode 2 and the full command engine.

### Graphic 4 (Screen 5) — Primary Bitmap Mode

256×212 pixels at 4 bits per pixel. 2 pixels packed per byte. 128 bytes per
scanline, 27,136 bytes total for the active framebuffer. **This is the
recommended default mode** for most game development on the Vanguard 8. It
offers the best balance of resolution, color depth, VRAM economy, and hardware
command throughput.

#### Bitmap Layout in VRAM

```
Line 0:   VRAM 0x0000 – 0x007F  (128 bytes, pixels 0–255)
Line 1:   VRAM 0x0080 – 0x00FF
...
Line 211: VRAM 0x69C0 – 0x6A3F
```

Pixel address formula:
```
byte_addr = (y × 128) + (x / 2)
nibble    = (x is even) ? high nibble : low nibble
```

### Graphic 5 (Screen 6)

512×212 pixels at 2 bits per pixel. 4 pixels per byte, 128 bytes per scanline
(same byte-per-line count as Graphic 4), 27,136 bytes total framebuffer.

The doubled horizontal resolution is useful for high-detail artwork or text
rendering. Color depth is reduced to 4 colors from the 16-entry palette (only
palette indices 0, 5, 10, 15 are used, mapped to the 2-bit values 0–3).
**Verify exact palette mapping for Graphic 5 against the V9938 Technical Data
Book before implementing.**

Pixel address formula:
```
byte_addr = (y × 128) + (x / 4)
pair      = (x mod 4); bits 7:6 = pixel 0, bits 5:4 = pixel 1,
            bits 3:2 = pixel 2, bits 1:0 = pixel 3 (within byte)
```

### Graphic 6 (Screen 7)

512×212 pixels at 4 bits per pixel. 2 pixels per byte, 256 bytes per scanline,
54,272 bytes total framebuffer. This mode delivers the highest resolution with
full 4bpp color, at the cost of consuming most of the available 64 KB VRAM for
the framebuffer alone.

VRAM layout must be planned carefully. With 54,272 bytes for the framebuffer,
only ~10 KB remains for sprite tables and pattern storage.

Pixel address formula:
```
byte_addr = (y × 256) + (x / 2)
nibble    = (x is even) ? high nibble : low nibble
```

### Graphic 7 (Screen 8)

256×212 pixels at 8 bits per pixel. 1 pixel per byte, 256 bytes per scanline,
54,272 bytes total framebuffer. This is the only mode with direct 8-bit color
per pixel, but it does not use the 9-bit RGB palette. Instead, each byte
encodes color directly in a packed RGB format (3 bits red, 3 bits green, 2 bits
blue). **This mode does not use the 16-entry palette; the palette registers are
ignored. Verify exact color encoding against the V9938 Technical Data Book.**

The 54 KB framebuffer leaves only ~10 KB for other VRAM structures, same
constraint as Graphic 6.

---

## Palette

Each V9938 maintains an independent 16-entry palette used by all color modes
except Graphic 7. Each entry is a 9-bit RGB value (3 bits per channel),
selected from a 512-color master space.

```
Palette entries:  16 (indices 0–15)
Per entry:        9-bit RGB  (R:3, G:3, B:3)
Master space:     512 possible colors

Color index 0:    Transparent on VDP-A when TP bit (R#8 bit 5) is set.
                  VDP-B shows through wherever VDP-A pixel index = 0.
```

The two VDP chips have fully independent palettes. Layer A and Layer B can
use completely different 16-color sets simultaneously, for a combined maximum
of 32 distinct colors visible on screen at once. Within each layer, all 16
colors are freely chosen from the 512-color 9-bit RGB space.

### Writing Palette Entries

Palette is written two bytes per entry via the palette port. Write the palette
index byte first (auto-increments), then two data bytes per entry:

```
Byte 0 (index):  palette entry number | 0x00 (resets auto-increment pointer)
Byte 1 (data):   R (bits 6:4), G (bits 2:0)
Byte 2 (data):   0 (bits 6:4), B (bits 2:0)
```

| Port  | VDP-A | VDP-B | Function          |
|-------|-------|-------|-------------------|
| Pal   | 0x82  | 0x86  | Palette write     |

---

## VRAM Layout Examples

Each VDP has 64 KB of VRAM. The layouts below are suggested starting points,
not hardware requirements. Register-controlled table base addresses (R#2–R#6,
R#11) allow tables to be placed anywhere within VRAM on the required alignment
boundaries.

### Graphic 4 Layout (Recommended Default)

```
0x0000 – 0x69FF   Active display bitmap      (27,136 bytes, 212 lines × 128 bytes)
0x6A00 – 0x79FF   Sprite pattern generator   (4,096 bytes = 128 unique 16×16 patterns)
0x7A00 – 0x7BFF   Sprite Color Table         (512 bytes, Mode 2: 32 sprites × 16 rows)
0x7C00 – 0x7CFF   Sprite Attribute Table     (256 bytes, 32 entries × 8 bytes)
0x7D00 – 0xFFFF   Tile/asset pattern bank    (~33 KB available for blitter source data)
```

**Sprite Color Table placement:** The V9938 hardware defines the Sprite Color
Table base as the Sprite Attribute Table base address minus 512 bytes. With the
SAT at 0x7C00, the Color Table is automatically at 0x7A00. This offset is fixed
by hardware and cannot be independently configured.

The tile/asset bank stores pre-rendered tile bitmaps for use with the HMMM
blitter. Each 8×8 tile at 4bpp occupies 32 bytes (8 rows × 4 bytes per row).
~33 KB supports approximately 1,060 unique 8×8 tile patterns.

### Graphic 3 Layout (Tile Mode with Sprite Mode 2)

```
0x0000 – 0x02FF   Pattern Name Table         (768 bytes, 32×24 tile map)
0x0300 – 0x17FF   Pattern Generator Table    (6,144 bytes, 3 banks × 256 × 8)
0x1800 – 0x2FFF   Color Table                (6,144 bytes, 3 banks × 256 × 8)
0x3000 – 0x3FFF   Sprite pattern generator   (4,096 bytes = 128 16×16 patterns)
0x4000 – 0x41FF   Sprite Color Table         (512 bytes, SAT base − 512)
0x4200 – 0x42FF   Sprite Attribute Table     (256 bytes, 32 entries × 8 bytes)
0x4300 – 0xFFFF   Remaining VRAM             (~47 KB available)
```

The exact alignment requirements for each table base register in Graphic 3
should be verified against the V9938 Technical Data Book (table bases are
typically aligned to fixed block sizes).

### Graphic 6 / 7 Layout Note

With ~54 KB consumed by the framebuffer, only ~10 KB remains. Sprite tables
and pattern storage must fit within that remainder. Sprite pattern generator
(4 KB for 128 16×16 patterns) plus SAT (256 bytes) plus Sprite Color Table
(512 bytes) totals ~4.75 KB — leaving approximately 5 KB of free VRAM.
Reduce sprite pattern count or use 8×8 sprites (512 bytes per 64 patterns)
to recover headroom.

---

## VDP Hardware Commands

The V9938 hardware command engine is accessible via registers R#32–R#46 and
operates **asynchronously** — the CPU queues a command and continues while the
VDP executes it. **Commands are only available in Graphic 3, 4, 5, 6, and 7.**
They are not available in Text modes or Graphic 1/2.

### Command Registers

| Register | Function                                      |
|----------|-----------------------------------------------|
| R#32–33  | Source X (SX), 9-bit                         |
| R#34–35  | Source Y (SY), 10-bit                        |
| R#36–37  | Destination X (DX), 9-bit                    |
| R#38–39  | Destination Y (DY), 10-bit                   |
| R#40–41  | Width (NX), 9-bit                            |
| R#42–43  | Height (NY), 10-bit                          |
| R#44     | Color (CLR), used by fill commands           |
| R#45     | Argument (ARG): direction bits DIX, DIY      |
| R#46     | Command (CMD): execute; bits 7:4 = opcode    |

Registers R#32–R#46 are written via the register indirect port (0x83 / 0x87).

`R#45` (`ARG`) is laid out as:

```
Bit 7   0     Unused
Bit 6   MXC   Color-source expansion-memory select
Bit 5   MXD   Destination expansion-memory select
Bit 4   MXS   Source expansion-memory select
Bit 3   DIY   Y direction (0 = down, 1 = up)
Bit 2   DIX   X direction (0 = right, 1 = left)
Bit 1   EQ    SRCH end condition
Bit 0   MAJ   LINE major-axis selector
```

Vanguard 8 does not provide V9938 expansion RAM, so `MXC`, `MXD`, and `MXS`
must be treated as `0` (VRAM) by software.

### Command Status

The **CE (Command Executing)** flag is bit 0 of status register **S#2**.
Poll this before issuing a new command to avoid collision:

```
Select S#2:  write R#15 = 2  (OUT port 0x81: value 0x02, then 0x8F)
Read status: IN A, (0x81)   → bit 0 = CE (1 = busy)
Restore:     write R#15 = 0  (OUT port 0x81: value 0x00, then 0x8F)
```

Relevant `S#2` bits for the command engine are:

```
Bit 7   TR   Transfer ready (CPU-streamed command handshake)
Bit 4   BD   Boundary detected (SRCH result)
Bit 0   CE   Command executing
```

`POINT` returns its sampled color in `S#7`. `SRCH` returns the boundary
x-coordinate in `S#8`/`S#9`.

For CPU-streamed commands:

- `HMMC` and `LMMC` accept CPU data through the normal VDP data port
- software polls `S#2.TR` before writing the next transfer byte

### Command Opcodes (R#46 bits 7:4)

| Opcode | Mnemonic | Operation                                 |
|--------|----------|-------------------------------------------|
| 0x0_   | STOP     | Abort current command                     |
| 0x4_   | POINT    | Read pixel color at (SX,SY)               |
| 0x5_   | PSET     | Write single pixel at (DX,DY)             |
| 0x6_   | SRCH     | Search for color along a scan line        |
| 0x7_   | LINE     | Draw a line                               |
| 0x8_   | LMMV     | Fill rectangle (logical, color from CLR)  |
| 0x9_   | LMMM     | Copy rectangle VRAM→VRAM (logical)        |
| 0xB_   | LMMC     | Transfer rectangle CPU→VRAM (logical)     |
| 0xC_   | HMMV     | Fill rectangle (byte, fast)               |
| 0xD_   | HMMM     | Copy rectangle VRAM→VRAM (byte, fast)     |
| 0xE_   | YMMM     | Copy rectangle vertically (fast)          |
| 0xF_   | HMMC     | Transfer rectangle CPU→VRAM (byte, fast)  |

### HMMM — Fast VRAM-to-VRAM Copy

HMMM copies a rectangle of bytes from one VRAM location to another without CPU
involvement after the command is queued. For an 8×8 tile in Graphic 4 (4bpp,
4 bytes wide × 8 rows):

```
NX = 4     (8 pixels / 2 pixels per byte = 4 bytes wide)
NY = 8     (8 rows)
SX = source tile column × 4  (byte address within source area)
SY = source tile row
DX = destination column × 4  (byte address in display area)
DY = destination row (scanline)
CMD = 0xD0 (HMMM)
```

HMMM executes at approximately 64 + (NX × NY × 3) master clock cycles.
For an 8×8 Graphic 4 tile: ~160 cycles (~22 µs at 7.16 MHz CPU).

In Graphic 5 (4 pixels per byte), NX must account for the narrower byte width:
an 8-pixel-wide tile at 2bpp is 2 bytes wide (NX = 2). In Graphic 6 (2 pixels
per byte, 256 bytes/line), an 8-pixel-wide tile is 4 bytes wide (NX = 4) but
coordinates are in the 512-pixel addressing space.

### Vanguard 8 Implementation Boundary

The hardware command engine exists across Graphic 3, 4, 5, 6, and 7, but the
current repo implementation milestone is still locked to the already-supported
bitmap surface. For milestone 8:

- the full documented command opcode set is exposed
- asynchronous `S#2.CE` behavior is required
- the covered pixel/byte packing model is **Graphic 4 / Screen 5**

Mode-specific command packing for Graphic 5, 6, and 7 remains part of the
later VDP mode-expansion milestone. Do not invent those layouts here.

---

## Scrolling

### Vertical Scroll

Register **R#23** sets the vertical display start line (0–255). Incrementing
this register by 1 scrolls the display up by one pixel line. The display wraps
within the VRAM page.

Vertical scroll is available in the bitmap modes (Graphic 4, 5, 6, 7) and
Graphic 3. Availability in Graphic 1 and Graphic 2 should be verified against
the V9938 Technical Data Book.

### Horizontal Scroll

Horizontal scroll register behavior should be verified against the V9938
Technical Data Book. Registers R#26 (coarse, in 8-pixel units) and R#27
(fine, 0–7 pixels) are documented for horizontal scroll in Graphic 4 and
higher bitmap modes. **Consult the V9938 Technical Data Book for exact behavior
and applicability before implementing horizontal scroll in any mode.**

### Per-Line Scroll (Raster Parallax)

Per-line horizontal scroll is achieved via the **scanline interrupt** (see
Interrupts section). The CPU updates the scroll register(s) at each target
scanline from the INT0 handler. Multiple scanline events can be chained per
frame by updating R#19 inside the handler.

Per-line scroll is only viable on VDP-A (which drives INT0). VDP-B has no
connected interrupt, so per-line scroll on VDP-B requires software driving the
register changes in sync with VDP-A's interrupt timing — this is possible but
adds CPU overhead. VDP-B vertical scroll (R#23) can be updated in the V-blank
handler with no timing concern.

---

## Sprites

The V9938 supports two sprite modes. Which mode is active depends on the
display mode:

| Display Mode            | Sprite Mode |
|-------------------------|-------------|
| Text 1, Text 2          | None (no sprites)         |
| Graphic 1, Graphic 2    | Sprite Mode 1             |
| Graphic 3–7             | Sprite Mode 2             |

Each VDP manages its own independent set of 32 sprites.

### Per-Chip Sprite Limits

| Parameter             | Mode 1              | Mode 2                    |
|-----------------------|---------------------|---------------------------|
| Sprites defined       | 32                  | 32                        |
| Sprites per scanline  | 4                   | 8                         |
| Color model           | One color per sprite| One color per sprite row  |
| Sizes                 | 8×8 or 16×16; ×2 mag| 8×8 or 16×16; ×2 mag    |
| Flip                  | Not supported       | Not supported             |

When both VDPs are active, the effective total sprite capacity doubles:
8 sprites per scanline from VDP-A plus 8 from VDP-B = **16 sprites per
scanline total**, across **64 total defined sprites** (32 per chip).

### Sprite Mode 1

Each sprite has a single color index applied uniformly to all opaque pixels.
The color is stored in bits 3:0 of SAT byte 3. Sprites are 1-bit patterns;
each pixel is either opaque (rendered in the sprite color) or transparent.

SAT entry format (Mode 1, 4 bytes per sprite):
```
Byte 0   — Y position (0–255)
Byte 1   — X position (0–255)
Byte 2   — Pattern number
Byte 3   — Color (bits 3:0); EC flag (bit 7)
  Bit  7  — EC (Early Clock): shifts sprite 32 pixels left for off-screen entry
```

### Sprite Mode 2

Per-row color. Each horizontal row of a sprite has an independently assignable
color from the current 16-entry palette. For a 16×16 sprite, 16 rows each
carry a different color, allowing rich multi-color sprite art with a 1-bit
pattern.

SAT entry format (Mode 2, 8 bytes per sprite):
```
Byte 0   — Y position (0–255)
Byte 1   — X position (0–255)
Byte 2   — Pattern number (indexes the sprite pattern generator)
Byte 3   — Color/flags
  Bits 3:0  — Default color (Mode 1 legacy field; overridden by color table in Mode 2)
  Bit  7    — EC (Early Clock): shifts sprite 32 pixels left for off-screen entry
Bytes 4–7 — (Color table stored separately in VRAM, not in SAT)
```

Sprite rendering stops when a Y value of **0xD0 (208)** is encountered,
regardless of remaining SAT entries.

### Sprite Color Table (Mode 2)

Located in VRAM at **SAT base address minus 512 bytes**. This placement is
automatic and fixed by the hardware — it is not independently configurable.
With the SAT at 0x7C00, the Sprite Color Table is at **0x7A00**.

Each sprite has 16 consecutive color entries (one per row), 1 byte each:
```
Bits 3:0  — Color index (0–15, indexes into the 16-entry palette)
Bit  7    — CC (Color Collision, read-only)
Bit  6    — IC (Invisible): if set, the row is not displayed
```

---

## Sprite Collision Detection

### Within-Chip (Hardware)

The V9938 sets the collision flag in **S#0 bit 5** when any two sprites on the
same chip have overlapping opaque pixels. Status registers S#3–S#6 report the
first collision coordinates. This applies in both Sprite Mode 1 and Mode 2.

| Register  | Content                              |
|-----------|--------------------------------------|
| S#0 bit 5 | Collision flag (cleared on S#0 read) |
| S#3       | Collision X, bits 7:0                |
| S#4       | Collision X, bit 8                   |
| S#5       | Collision Y, bits 7:0                |
| S#6       | Collision Y, bit 8                   |

### Cross-Plane (Software Only)

Collisions between a VDP-A sprite and a VDP-B sprite have no hardware
detection. Handle via software bounding box comparison against sprite position
data in SRAM.

---

## Interrupts

VDP-A's single **/INT pin connects to the CPU /INT0 line** (maskable interrupt).
VDP-B's /INT pin is not connected to the CPU — only VDP-A generates interrupts.

The /INT line is asserted when either the V-blank flag or H-blank flag is set
and the corresponding interrupt enable bit is active. The handler must read
status registers to identify the source.

Both interrupt sources (V-blank and H-blank) are available regardless of which
display mode VDP-A is in.

### Interrupt Sources and Status Registers

| Source   | Enable       | Flag register | Flag bit | Clear by           |
|----------|--------------|---------------|----------|--------------------|
| V-blank  | R#1 bit 5    | S#0           | bit 7    | Reading S#0        |
| H-blank  | R#0 bit 4    | S#1           | bit 0    | Reading S#1        |

S#0 is the default status register (read port 0x81 directly).
To read S#1, first write R#15 = 1, read port 0x81, then restore R#15 = 0:

```
Write R#15 = 1:   OUT (0x81), 0x01 ; OUT (0x81), 0x8F
Read S#1:         IN  A, (0x81)    ; bit 0 = FH (H-blank flag)
Write R#15 = 0:   OUT (0x81), 0x00 ; OUT (0x81), 0x8F
```

### Scanline Interrupt Setup

| Register | Function                                          |
|----------|---------------------------------------------------|
| R#0      | Bit 4: Enable H-blank interrupt                   |
| R#1      | Bit 5: Enable V-blank interrupt                   |
| R#19     | Target scanline number for H-blank interrupt      |

---

## V9938 Key Registers

### Mode and Control

| Register | Function                                                                   |
|----------|----------------------------------------------------------------------------|
| R#0      | Mode bits M3, M4, M5; H-blank interrupt enable (bit 4). See databook for exact bit positions. |
| R#1      | Mode bits M1, M2; V-blank IRQ (bit 5); display on (bit 6); sprite size (bit 1); mag (bit 0) |
| R#2      | Pattern name table base (tile modes and Text modes)                        |
| R#3      | Color table base, low bits (tile modes)                                    |
| R#4      | Pattern generator table base (tile and Text modes)                         |
| R#5      | Sprite attribute table base, bits 13:7 (use with R#11)                    |
| R#6      | Sprite pattern generator base                                              |
| R#7      | Border / backdrop color index                                              |
| R#8      | Bit 5: TP — color 0 transparent (set for VDP-A compositing); other flags  |
| R#9      | Bit 7: LN — 1 = 212 lines, 0 = 192 lines                                 |
| R#11     | Sprite attribute table base, bits 15:14 (required for 64 KB VRAM)        |

### Scroll

| Register | Function                                            |
|----------|-----------------------------------------------------|
| R#19     | Scanline interrupt target line                      |
| R#23     | Vertical scroll offset (Graphic 3–7; verify others) |
| R#26     | Horizontal scroll coarse — verify vs. databook      |
| R#27     | Horizontal scroll fine — verify vs. databook        |

### Status Registers (selected via R#15, read via port 0x81 / 0x85)

| Register | Content                                                           |
|----------|-------------------------------------------------------------------|
| S#0      | Bit 7: VB (V-blank); bit 6: F5 (5th sprite); bit 5: C (collision) |
| S#1      | Bit 0: FH (H-blank interrupt flag)                                |
| S#2      | Bit 0: CE (command executing); bits 6:5: display status           |
| S#3–S#4  | Sprite collision X coordinate                                     |
| S#5–S#6  | Sprite collision Y coordinate                                     |

---

## Port Reference

| Port  | Dir | Chip  | Function                        |
|-------|-----|-------|---------------------------------|
| 0x80  | R/W | VDP-A | VRAM data read/write            |
| 0x81  | W   | VDP-A | VRAM address / register command |
| 0x81  | R   | VDP-A | Status register (S#0 default)   |
| 0x82  | W   | VDP-A | Palette write                   |
| 0x83  | W   | VDP-A | Register indirect write         |
| 0x84  | R/W | VDP-B | VRAM data read/write            |
| 0x85  | W   | VDP-B | VRAM address / register command |
| 0x85  | R   | VDP-B | Status register (S#0 default)   |
| 0x86  | W   | VDP-B | Palette write                   |
| 0x87  | W   | VDP-B | Register indirect write         |

# Vanguard 8 Emulator — Pre-GUI Audit (Milestones 1-11)

## Purpose

This audit was performed before beginning milestone 12 (desktop GUI) work.
The goal is to identify features, behaviors, and capabilities that were
documented or expected in milestones 1-11 but are missing, incomplete, or
stubbed in the current codebase. The desktop GUI gap was already identified
and tracked in `docs/emulator/10-desktop-gui-audit.md`; this document
focuses on everything else.

Audit date: 2026-03-27
Audited against: `docs/spec/`, `docs/emulator/`, and actual source in `src/`.

---

## Severity Key

| Level    | Meaning |
|----------|---------|
| Critical | Core V9938/HD64180 behavior that real software will depend on |
| High     | Documented emulator feature that is missing or broken |
| Medium   | Spec-defined behavior that is stubbed or partially implemented |
| Low      | Nice-to-have or cosmetic gap relative to documentation |

---

## 1. V9938 Display Mode Coverage

### 1.1 Missing: Graphic 1, Graphic 2 renderers — Critical

The spec (`docs/spec/02-video.md`) defines nine V9938 display modes and
states both VDP chips support all of them equally. Only Graphic 3 and
Graphic 4 have scanline renderers in `src/core/video/v9938.cpp`.
`current_display_mode()` returns `unsupported` for all other modes, which
fills the background with the backdrop color.

Graphic 1 and Graphic 2 are 192-line tile modes with Sprite Mode 1.
These are the simplest tile modes and are the natural choice for
status screens, title screens, and text-heavy game UIs. Any ROM that
uses Graphic 1 or 2 for even a single screen (e.g., a boot logo or
menu) will display a blank backdrop.

Impact: High probability of hitting this in real software.

### 1.2 Missing: Text 1 and Text 2 mode renderers — Medium

Text modes (40-column and 80-column) are documented in the spec. Neither
is implemented. These are less likely to be used by action-oriented game
software, but are plausible for system menus, in-game text overlays on
VDP-A composited over a VDP-B game layer, or homebrew development tools.

Impact: Lower probability than Graphic 1/2, but still a real gap.

### 1.3 Missing: Graphic 5, 6, 7 renderers — Medium

Graphic 5 (512x212, 2bpp), Graphic 6 (512x212, 4bpp), and Graphic 7
(256x212, 8bpp direct color) are documented in the spec. None are
implemented. The milestone-11 readiness doc (`docs/emulator/09-1.0-readiness.md`)
explicitly defers Graphic 5 palette mapping and Graphic 7 direct-color
encoding pending databook verification.

These modes are used less frequently but are not exotic — Graphic 7
in particular is attractive for games that want direct RGB color without
palette management.

Impact: Medium. Unlikely to be hit by the first wave of software but
will surface eventually.

---

## 2. V9938 Sprite System Gaps

### 2.1 Missing: Sprite Mode 1 — Critical

Only Sprite Mode 2 (per-row color, 8 sprites per scanline) is implemented.
Sprite Mode 1 (single color per sprite, 4 sprites per scanline) is used
by Graphic 1 and Graphic 2. Even if those display modes are not yet
rendered, Sprite Mode 1 is a distinct rendering path that differs in:

- Color model: one color per entire sprite vs. per-row color
- Per-scanline limit: 4 instead of 8
- SAT entry format: 4 bytes per sprite vs. 8 bytes
- No Sprite Color Table in VRAM

Any software using Graphic 1 or 2 will need this path.

### 2.2 Missing: 16x16 sprites and sprite magnification — Critical

`src/core/video/v9938.cpp` hardcodes sprite size to 8x8:
```cpp
constexpr int sprite_size = 8;
```

The V9938 supports 8x8 and 16x16 sprite sizes via R#1 bit 1 (SI), and
2x magnification via R#1 bit 0 (MAG). These are not checked. 16x16
sprites are the standard sprite size for game characters and are
documented extensively in `docs/spec/02-video.md`. The recommended
VRAM layouts allocate 4,096 bytes for "128 unique 16x16 patterns."

This is almost certainly the most impactful single missing feature.
Nearly all games will use 16x16 sprites for player characters, enemies,
and projectiles.

### 2.3 Incomplete: Register-relative sprite table addressing — Medium

Sprite table base addresses (SAT, Sprite Pattern Generator, Sprite Color
Table) are hardcoded per display mode rather than derived from registers
R#5, R#6, and R#11 as the V9938 hardware does. The code acknowledges this
with a comment deferring "general base-register relocation" to a future
milestone.

Software that places sprite tables at non-default VRAM addresses will
have broken sprites. This is a plausible scenario for games that need
custom VRAM layouts.

---

## 3. V9938 Core Behavior Gaps

### 3.1 Missing: VRAM read-ahead latch — Critical

The V9938 implements a read-ahead buffer: when software sets the VRAM
address for reading, the VDP prefetches the byte at that address into
an internal latch and increments the address. The first `read_data()`
call returns the *previously* latched value, not the byte at the
current address.

The current implementation (`v9938.cpp` `read_data()`) returns
`vram_[vram_addr_]` directly with no latch. This means the first byte
read after setting a VRAM address will be wrong by one position.

This is a well-known V9938 quirk that all MSX software accounts for.
Any ROM that reads VRAM (sprite positions, pattern data, command engine
results) will get incorrect data on the first read of every address-set
sequence. This can cause subtle bugs that are hard to diagnose.

### 3.2 Missing: VDP screen disable (R#1 bit 6) — Medium

The V9938's "display enable" bit (R#1 bit 6, also called BL) controls
whether the VDP outputs active video or blanks the display. When cleared,
the screen should show the backdrop color on all pixels. The current
renderer does not check this bit.

Software commonly disables the screen during large VRAM updates to avoid
visual artifacts and to gain faster VRAM access (the VDP relaxes timing
constraints when the display is blanked). If this bit is not honored, the
emulator will render partial VRAM states that the real hardware would
never show.

### 3.3 Not verified: Command engine packing for Graphic 5/6/7 — Medium

The VDP command engine (HMMM, LMMM, etc.) currently only supports
Graphic 4 pixel/byte packing. Graphic 5 (4 pixels per byte), Graphic 6
(256 bytes per line), and Graphic 7 (1 byte per pixel) each have
different coordinate-to-VRAM-address mapping. This is documented as
deferred in `docs/emulator/04-video.md` but worth noting alongside the
renderer gaps.

---

## 4. Emulator Workflow Features

### 4.1 Missing: Quicksave/quickload slots — High

`docs/emulator/02-emulation-loop.md` documents 10 numbered quicksave
slots with `Shift+F1`-`Shift+F9` / `F1`-`F9` hotkeys, metadata sidecars,
and 128x96 PNG thumbnails. The save state system (`src/core/save_state.cpp`)
supports serialization and deserialization, but there is no slot
management, no thumbnail generation, and no hotkey-driven save/load.

This is a core usability feature for an emulator.

### 4.2 Missing: Speed control — High

`docs/emulator/02-emulation-loop.md` documents fast-forward, slow motion,
pause, and frame advance modes. None of these are implemented as
user-facing controls. The headless binary supports `--frames N` for
bounded execution, but there is no runtime speed toggle.

Frame advance is listed as a debugger control (F6) but is not wired to
any interactive path.

### 4.3 Missing: Audio WAV recording — Medium

`docs/emulator/00-overview.md` lists "Audio recording to WAV" as a
workflow feature and `docs/emulator/02-emulation-loop.md` documents
`--dump-audio` in the headless CLI. The headless path generates audio
samples and computes a digest, but there is no WAV file writer.

### 4.4 Missing: Screenshot as PNG — Low

Screenshots are currently limited to PPM format via `Display::dump_ppm_file()`.
The documentation references PNG output for both screenshots and save
state thumbnails. PPM is functional for regression testing but is not a
practical format for user-facing screenshots.

### 4.5 Incomplete: Auto-resume — Low

`docs/emulator/00-overview.md` lists "Auto-resume: reopen last ROM and
optionally restore last session on launch." The config tracks
`recent_roms` but there is no automatic ROM reload or session restore
on startup.

---

## 5. Debugger Gaps (Model Layer)

The debugger shell and panel models exist and are tested, but some
documented features are absent even from the data model layer.

### 5.1 Missing: Trace-to-file — Medium

`docs/emulator/06-debugger.md` documents unbounded execution trace
written to disk via a background queue (`--trace trace.log`). No trace
file writer exists in the codebase. The in-memory trace ring buffer
(1024 entries) is implemented.

### 5.2 Missing: Audio panel — Medium

`docs/emulator/06-debugger.md` documents per-chip audio register
inspection, per-channel mute toggles, volume trim sliders, and a
waveform visualizer. The header `src/debugger/audio_panel.hpp` exists
but the panel's data model implementation could not be confirmed as
complete. The mixer does not expose per-chip mute or volume trim
controls.

### 5.3 Missing: Symbol file support — Low

`docs/emulator/06-debugger.md` documents loading `.sym` files for
label-annotated disassembly, breakpoints by name, and memory panel
annotations. No symbol loader or annotation logic was found in the
debugger code.

### 5.4 Missing: Game debug console (port 0xFE) — Low

`docs/emulator/06-debugger.md` documents a dev-only debug output port
at 0xFE with a console panel and `[dev]` config toggle. Not implemented
in the bus I/O dispatch or debugger.

---

## 6. HD64180 / CPU Gaps

### 6.1 Not verified: Full opcode coverage — Medium

`docs/emulator/08-compatibility-audit.md` explicitly states the repo
has a "tested Vanguard 8 execution subset" and does not claim full
Z80/Z180 opcode parity. The extracted MAME Z180 core likely has broad
coverage, but the milestone-2 extraction was intentionally limited.
Specific concerns:

- `MLT` (8x8 multiply): documented as HD64180-specific, not yet
  independently tested
- `TST` (test under mask): HD64180 extension, testing status unknown
- `IN0`/`OUT0` for internal registers beyond the MMU subset: partial
  coverage

### 6.2 Documented gap: ASCI/CSIO — Low

The HD64180's serial peripherals (ASCI channels 0/1, CSIO) are listed
as deferred in `docs/emulator/03-cpu-and-bus.md`. No Vanguard 8 software
should use these (no serial port on the console), but completeness is
noted.

---

## 7. Spec-Level Deferred Items (Already Tracked)

These are already documented as intentional deferrals but are listed
here for completeness:

- Horizontal scroll (R#26/R#27): deferred pending V9938 databook
- Graphic 3 LN=1 lines 192-211: locked to backdrop
- 4-player expansion: spec not complete
- Battery-backed cartridge SRAM: spec not complete
- Cross-VDP sprite collision: software-only per spec

---

## Prioritized Recommendations

### Must fix before real software testing

1. **16x16 sprites and sprite magnification** (2.2) — Nearly all games
   will break without this. This is a straightforward addition: check
   R#1 bits 0-1, adjust sprite size and pattern fetch accordingly.

2. **VRAM read-ahead latch** (3.1) — Any ROM that reads VRAM will get
   offset data. Add a single `read_latch_` byte to V9938, prefetch on
   address set, and return/advance the latch on data reads.

3. **Sprite Mode 1** (2.1) — Required by Graphic 1 and 2. Simpler than
   Mode 2 (single color, 4 bytes per SAT entry, 4 sprites per line).

4. **Graphic 1 and Graphic 2 renderers** (1.1) — These are the
   simplest tile modes. Many games use them for menus and title screens.

5. **VDP screen disable** (3.2) — Quick to implement, prevents rendering
   artifacts during VRAM updates.

### Should fix for usability

6. **Quicksave/quickload slots** (4.1) — Core emulator UX expectation.
7. **Speed control** (4.2) — Fast-forward alone is a major usability win.
8. **Register-relative sprite table addressing** (2.3) — Required for
   non-default VRAM layouts.

### Should fix for completeness

9. **Text 1 and Text 2 renderers** (1.2)
10. **Graphic 5, 6, 7 renderers** (1.3)
11. **Trace-to-file** (5.1)
12. **Audio WAV recording** (4.3)
13. **Symbol file support** (5.3)

---

## Summary

The core emulation architecture is sound: the scheduler, interrupt model,
bus routing, save states, replay, and regression tooling are solid. The
gaps cluster in two areas:

1. **V9938 feature surface** — Only 2 of 9 display modes are rendered.
   Sprites are locked to 8x8 Mode 2 only. The VRAM read-ahead latch is
   missing. These are not edge cases; they represent the majority of the
   V9938's capabilities and will affect most software.

2. **User-facing workflow** — Quicksave slots, speed control, WAV export,
   PNG screenshots, and auto-resume are documented but unimplemented.
   These don't affect emulation correctness but are expected features of
   a usable emulator.

The most urgent items (16x16 sprites, VRAM read-ahead latch, Sprite
Mode 1, Graphic 1/2, screen disable) should be scheduled immediately after the
playable desktop baseline so ROM-driven validation can begin without mixing the
frontend bring-up milestone with broad core-compatibility work.

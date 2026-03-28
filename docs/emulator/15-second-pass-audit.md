# Vanguard 8 Emulator — Second-Pass Audit

## Purpose

This is a follow-up to the pre-GUI audit (`docs/emulator/11-pre-gui-audit.md`)
performed on 2026-03-27. That audit identified gaps across milestones 1-11.
Milestones 13-14 were subsequently completed, targeting the critical and
high-priority items. This second pass re-evaluates every original finding
against the current codebase and notes any new observations.

Audit date: 2026-03-28
Audited against: current `src/` tree and original audit findings.

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

### 1.1 Graphic 1, Graphic 2 renderers — RESOLVED

Both modes now have scanline renderers:
- `render_graphic1_background_scanline()` at `v9938.cpp:806`
- `render_graphic2_background_scanline()` at `v9938.cpp:838`

Mode detection in `current_display_mode()` correctly maps mode bits to
`DisplayMode::graphic1` and `DisplayMode::graphic2`.

### 1.2 Text 1 and Text 2 mode renderers — Still missing (Medium)

Neither text mode is implemented. The `DisplayMode` enum has no entries for
them and `current_display_mode()` returns `unsupported` for mode bits 0x01
and 0x05. These modes fill the backdrop color only.

Status unchanged from original audit. Remains a plausible gap for system
menus, text overlays on VDP-A, or homebrew tools.

### 1.3 Graphic 5, 6, 7 renderers — Still missing (Medium)

No renderers exist for these modes. The `DisplayMode` enum lacks entries and
mode detection returns `unsupported`. Additionally, the VDP command engine
only supports Graphic 4 pixel packing — all commands call `graphic4_pset()`
and `graphic4_point()` exclusively. Graphic 5 (4 pixels/byte), Graphic 6
(512-wide addressing), and Graphic 7 (1 byte/pixel direct color) each
require distinct coordinate-to-VRAM mapping in both the renderer and the
command engine.

Status unchanged. The readiness doc notes that Graphic 5 and 7 carry
spec-verification dependencies on the V9938 databook.

---

## 2. V9938 Sprite System

### 2.1 Sprite Mode 1 — RESOLVED

`render_mode1_sprites_for_scanline()` at `v9938.cpp:912-996` implements
single-color-per-sprite rendering with 4-sprites-per-scanline overflow
detection and the 0xD0 sprite terminator.

### 2.2 16x16 sprites and sprite magnification — RESOLVED

- `sprite_size_pixels()` checks R#1 bit 1 and returns 8 or 16.
- `sprite_magnified()` checks R#1 bit 0 for 2x magnification.
- 16x16 pattern addressing correctly aligns patterns and splits into
  left/right halves with 8-byte offsets.
- Both Sprite Mode 1 and Mode 2 renderers apply size and magnification.

### 2.3 Register-relative sprite table addressing — RESOLVED

Sprite table bases are now derived from registers:
- `sprite_attribute_base()` combines R#5 and R#11 (`v9938.cpp:1134-1139`)
- `sprite_pattern_base()` uses R#6 (`v9938.cpp:1126-1128`)
- `sprite_color_base()` derived from attribute base (`v9938.cpp:1130-1132`)

---

## 3. V9938 Core Behavior

### 3.1 VRAM read-ahead latch — RESOLVED

`read_data()` at `v9938.cpp:48-53` returns the previously latched value and
prefetches the byte at the current address into `read_ahead_latch_`. The
first read after an address set correctly returns stale data, matching real
V9938 behavior.

### 3.2 VDP screen disable (R#1 bit 6) — RESOLVED

`display_enabled()` checks R#1 bit 6. When cleared, `tick_scanline()` fills
the line with backdrop color and skips all background and sprite rendering.

### 3.3 Command engine packing for Graphic 5/6/7 — Still missing (Medium)

All command engine operations (LMMV, LMMM, HMMV, HMMM, YMMM, LINE, etc.)
exclusively use `graphic4_pset()`, `graphic4_point()`, and
`graphic4_command_byte_address()`. No alternate packing paths exist for
Graphic 5, 6, or 7. This is tightly coupled to the renderer gap in 1.3 —
both would need to be addressed together.

---

## 4. Emulator Workflow Features

### 4.1 Quicksave/quickload slots — Still missing (High)

`SaveState` provides `serialize()` and `load()` but no slot management,
no numbered slots, no hotkey-driven save/load, and no thumbnail generation.
This remains a core usability gap for the desktop GUI.

### 4.2 Speed control — Still missing (High)

- **Pause/resume**: Implemented in the emulator core (`Emulator::pause()`,
  `Emulator::resume()`).
- **Frame advance**: `ExecutionCommand::step_frame` exists in the debugger
  CPU panel for single-frame stepping.
- **Fast-forward / slow motion**: Not implemented. No speed multiplier or
  throttle bypass is exposed as a user-facing control.

Partial progress — pause and frame advance work in the debugger context,
but the documented fast-forward and slow-motion modes are absent.

### 4.3 Audio WAV recording — Still missing (Medium)

No WAV file writer exists. The headless path supports `--hash-audio` for
digest verification but does not write audio to disk. The documented
`--dump-audio` CLI flag is not implemented.

### 4.4 Screenshot as PNG — Still missing (Low)

Only PPM output is available via `Display::dump_ppm_file()`. PNG is
referenced in documentation for screenshots and save-state thumbnails but
no PNG encoder is present.

### 4.5 Auto-resume — Still missing (Low)

`record_recent_rom()` tracks recently loaded ROMs in config, and `--recent`
allows CLI selection by index, but there is no automatic ROM reload or
session restore on startup.

---

## 5. Debugger Gaps

### 5.1 Trace-to-file — RESOLVED

`TracePanel::write_to_file()` in `trace_panel.cpp` writes formatted
execution traces to disk with symbol annotation support. The headless CLI
exposes `--trace path.log --trace-instructions N --symbols path.sym`.

### 5.2 Audio panel — Still stub (Medium)

`AudioPanel` in `audio_panel.hpp` is an empty class. No per-chip mute,
volume trim, or waveform visualization exists. The mixer does not expose
per-channel controls.

### 5.3 Symbol file support — RESOLVED

`SymbolTable` in `symbols.hpp`/`symbols.cpp` provides:
- `.sym` file loading (hex address + label format, `;` comments)
- Exact and by-name lookup
- `format_address()` for `LABEL+0xOFFSET` display
- Auto-discovery of `.sym` files alongside ROMs
- Integration with trace writer for annotated output

### 5.4 Game debug console (port 0xFE) — Still missing (Low)

Port 0xFE is not registered in the bus I/O dispatch. No debug output
console panel exists.

---

## 6. HD64180 / CPU Gaps

### 6.1 Opcode coverage — Still limited but improved (Medium)

The Z180 core's `initialize_tables()` registers a small primary opcode set
(NOP, LD family, HALT, JP, RET, CALL, DI, EI, ED prefix). However, the ED
prefix handler (`op_ed_prefix()` at `z180_core.cpp:604`) uses bitfield
pattern matching to decode additional instructions beyond the ED opcode
table, giving broader coverage than the table alone suggests:

- **MLT** (8x8 multiply): Implemented and tested (`op_ed_mlt_rr`,
  `z180_core.cpp:647`; test at `test_cpu.cpp:147`)
- **TST** (test under mask): Implemented and tested — both register and
  immediate forms (`op_ed_tst_r`, `op_ed_tst_n`, `z180_core.cpp:637-644`;
  test at `test_cpu.cpp:169`)
- **IN0** (register form): Decoded via ED prefix bitfield matching
  (`op_ed_in0_r_n`, `z180_core.cpp:633`)
- **OUT0** (register form): Both the bitfield-decoded register form
  (`op_ed_out0_n_r`, `z180_core.cpp:635`) and the dedicated A-register
  form (`op_ed_out0_n_a`, `z180_core.cpp:652`) are implemented.

The primary opcode table (~14 entries) is still a limited subset of the
full Z80 instruction set — missing arithmetic, logic, bit manipulation,
block transfer, conditional jumps/calls, and index register (IX/IY)
instructions. Any ROM beyond simple boot/test sequences will hit
`op_unimplemented` quickly. Expanding the primary opcode surface remains
the largest CPU-side gap.

### 6.2 ASCI/CSIO — Still deferred (Low)

No change. No Vanguard 8 software should use serial peripherals.

---

## Resolution Summary

| # | Original Finding | Severity | Status |
|---|-----------------|----------|--------|
| 1.1 | Graphic 1, 2 renderers | Critical | **Resolved** |
| 1.2 | Text 1, 2 renderers | Medium | Open |
| 1.3 | Graphic 5, 6, 7 renderers | Medium | Open |
| 2.1 | Sprite Mode 1 | Critical | **Resolved** |
| 2.2 | 16x16 sprites + magnification | Critical | **Resolved** |
| 2.3 | Register-relative sprite tables | Medium | **Resolved** |
| 3.1 | VRAM read-ahead latch | Critical | **Resolved** |
| 3.2 | VDP screen disable | Medium | **Resolved** |
| 3.3 | Command engine G5/6/7 packing | Medium | Open |
| 4.1 | Quicksave/quickload slots | High | Open |
| 4.2 | Speed control | High | Partial (pause/frame advance only) |
| 4.3 | Audio WAV recording | Medium | Open |
| 4.4 | Screenshot PNG | Low | Open |
| 4.5 | Auto-resume | Low | Open |
| 5.1 | Trace-to-file | Medium | **Resolved** |
| 5.2 | Audio panel | Medium | Open (stub) |
| 5.3 | Symbol file support | Low | **Resolved** |
| 5.4 | Game debug console (0xFE) | Low | Open |
| 6.1 | Full opcode coverage | Medium | Partial (MLT, TST, IN0, OUT0 added; primary table still limited) |
| 6.2 | ASCI/CSIO | Low | Deferred (by design) |

**Resolved: 8 of 20** items (all 4 critical items plus 4 others).
**Open: 12 of 20** items (0 critical, 2 high, 6 medium, 4 low).

---

## Updated Prioritized Recommendations

All original critical items have been addressed. The remaining work falls
into two tiers.

### Tier 1 — Should fix for usability (High)

1. **Quicksave/quickload slots** (4.1) — Numbered slots with hotkeys are a
   baseline expectation once the desktop GUI is in use.
2. **Speed control** (4.2) — Fast-forward alone is a significant usability
   win. Pause and frame advance already work; wiring a speed multiplier to
   the emulation loop is the remaining piece.

### Tier 2 — Should fix for completeness (Medium)

3. **Text 1 and Text 2 renderers** (1.2) — Low risk of hitting in games
   but plausible for menus, overlays, and dev tools.
4. **Graphic 5, 6, 7 renderers + command engine packing** (1.3, 3.3) —
   These are coupled. Graphic 7 is the most likely to surface first (direct
   RGB color). Both require V9938 databook verification per existing notes.
5. **Audio panel model** (5.2) — Per-channel mute and volume trim would
   complete the debugger feature set.
6. **Audio WAV recording** (4.3) — Useful for audio debugging and content
   creation.
7. **Primary opcode expansion** (6.1) — MLT, TST, IN0, OUT0 are covered,
   but the primary Z80 opcode table (~14 entries) lacks arithmetic, logic,
   bit manipulation, block transfer, conditional branches, and IX/IY
   instructions. Any non-trivial ROM will require significant expansion.

### Tier 3 — Nice to have (Low)

8. **Screenshot PNG** (4.4) — PPM works for regression; PNG is user-facing polish.
9. **Auto-resume** (4.5) — Convenience feature for the desktop GUI.
10. **Game debug console (0xFE)** (5.4) — Dev-only feature, useful for
    homebrew but not for running existing ROMs.
11. **ASCI/CSIO** (6.2) — No known use case on the Vanguard 8 console.

---

## Conclusion

Milestones 13-14 successfully closed every critical gap identified in the
first audit. The V9938 now covers 4 of 9 display modes (up from 2), both
sprite modes with full size/magnification support, register-relative table
addressing, the VRAM read-ahead latch, and display-enable blanking. The
debugger gained trace-to-file and symbol annotation.

The remaining open items are all medium or lower severity. The two high
items (quicksave slots and speed control) are GUI workflow features rather
than emulation correctness issues — no ROM will produce incorrect output
because of them. The medium-severity renderer and command-engine gaps
(Text 1/2, Graphic 5-7) are real but unlikely to surface until specific
ROMs exercise those modes.

The emulator is now in a reasonable state for initial ROM-driven validation
using Graphic 1-4 software.

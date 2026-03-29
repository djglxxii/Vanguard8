# Milestone 2 Scene Provenance

Scene assignment:
- Title / identity scene
- Dual-VDP compositing scene

Source format:
- Assembly-authored Graphic 4 primitives expressed as byte-aligned rectangles
  in `showcase/src/showcase.asm`

Conversion target:
- Direct VDP VRAM writes through the showcase ROM's fixed and banked scene-init
  routines

Palette assumptions:
- VDP-A title palette: dark slate background, bright green body, mint accents
- VDP-A compositing palette: color `0` transparent, bright green frame, mint
  bridge/highlight
- VDP-B compositing palette: deep blue background, blue panel, cyan panel/band

Intent note:
- The title scene should read as a stable identity card first, not as a test
  pattern.
- The compositing scene should make VDP-A color-0 transparency obvious without
  needing spoken explanation: the green frame must stay in front while the blue
  VDP-B panels remain visible through the transparent field.

Revision note:
- Milestone 2 intentionally uses procedural geometry instead of imported source
  art so the ROM can stay reproducible while the asset policy is still being
  locked.

# Milestone 3 Scene Provenance

Scene assignment:
- Tile-mode presentation scene
- Sprite capability scene

Source format:
- Assembly-authored Graphic 2 pattern/name/color tables and Graphic 4 sprite
  tables in `showcase/src/showcase.asm`

Conversion target:
- Direct VDP VRAM writes and covered Graphic 4 HMMV clears issued by the
  showcase ROM's banked scene-init routines

Palette assumptions:
- Tile scene VDP-A palette: dark navy backdrop, teal panel fill, mint header,
  bright green border, amber selection bars, and pale icon highlights
- Sprite scene VDP-B palette: dark navy field, cyan slot markers, mint first
  large sprite, magenta/amber second large sprite
- Sprite scene VDP-A palette: transparent background with bright small-sprite
  accents for overflow and overlap cues

Intent note:
- The tile scene should read as a menu/status screen first, with obvious tile
  repetition and a single Sprite Mode 1 cursor marker.
- The sprite scene should make three cues visible without narration: two large
  rear-layer size/magnification sprites at the top, one empty slot in a
  nine-slot row because only eight small sprites render on a scanline, and an
  overlap pair where the lower sprite index stays in front.

Revision note:
- Milestone 3 continues the no-import policy from milestone 2: all tile and
  sprite assets remain procedural assembly source so scene intent stays easy to
  audit alongside the hardware-facing VRAM layout.

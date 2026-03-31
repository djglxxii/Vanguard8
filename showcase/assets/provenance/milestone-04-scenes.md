# Milestone 4 Scene Provenance

Scene assignment:
- Graphic 3 + Graphic 4 mixed-mode review scene
- Graphic 4 command-engine review scene

Source format:
- Assembly-authored Graphic 3 pattern/name/color tables plus Graphic 4 command
  register sequences in `showcase/src/showcase.asm`

Conversion target:
- Direct VDP register writes, covered Graphic 3 tile-table uploads, and covered
  Graphic 4 `HMMV` command launches issued by the showcase ROM's banked
  scene-init routines

Palette assumptions:
- Mixed-mode scene VDP-A palette: transparent background, amber wall tiles,
  mint marker tiles, and bright white highlight tiles
- Mixed-mode scene VDP-B palette: dark navy backdrop, cyan/teal command-deck
  blocks, soft green panel fills, and warm light accents
- Command scene VDP-A palette: dark background with cyan frame blocks, mint
  fill areas, amber command windows, and red highlight bars over a disabled
  VDP-B layer

Intent note:
- The mixed-mode scene should read as a tile HUD/foreground laid over a Graphic
  4 rear layer, with large transparent cut-outs where VDP-B clearly shows
  through and opaque VDP-A tiles that visibly win in front.
- The command scene should read as a staged Graphic 4 construction pass:
  broad filled regions appear first, then smaller inset blocks and highlight
  bars arrive over later timed steps, making command-engine driven updates easy
  to spot from single-frame captures.

Revision note:
- Milestone 4 keeps the same no-import policy as milestones 2 and 3: all scene
  assets remain procedural assembly source so the mixed-mode tile layout and
  command-engine staging remain directly auditable next to the hardware-facing
  register writes.
- The accepted banked entry layout for the command scene is split across two
  bank values because the current `sjasmplus --raw` layout places
  `command_scene_step0_bank5` at physical `0x28000` and
  `command_scene_step1_bank5` through `command_scene_step9_bank5` at physical
  `0x38000+`. The showcase ROM now switches `BBR` to `0x28` for step 0 and to
  `0x38` for steps 1-9 to match the actual packaged ROM layout.

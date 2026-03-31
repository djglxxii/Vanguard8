# Showcase Asset Guidelines

Milestone 2 locks the first generated-asset conventions used by the showcase
ROM.

Traceability:
- `docs/spec/02-video.md`
- `showcase/docs/showcase-rom-plan.md`
- `showcase/docs/milestones/02.md`

Current conventions:
- Prefer assembly-authored or procedurally described Graphic 4 primitives over
  external bitmap imports.
- Keep all art byte-aligned to the Graphic 4 2-pixel byte layout so VRAM writes
  stay deterministic and easy to audit.
- Use large flat regions, clear silhouette edges, and minimal dithering.
- Reserve VDP-A palette index `0` for either the title background color or the
  compositing transparency color, depending on the scene.
- Keep the VDP-B layer visually simpler than the VDP-A overlay in compositing
  scenes so the reveal behavior is obvious.

Palette rules:
- Use a small repeated palette family across scenes.
- Title scene: dark slate background plus green/mint identity colors on VDP-A.
- Compositing scene: deep blue background family on VDP-B and green/mint frame
  colors on VDP-A.
- Avoid noisy multi-color ramps that make layer ordering harder to read.

Generated-asset provenance format:
- Scene assignment
- Source format
- Conversion target
- Palette assumptions
- Short intent note

Milestone-2 application:
- The title scene uses a blocky `V8` crest built from Graphic 4 rectangles on
  VDP-A.
- The compositing scene uses a VDP-A frame/badge overlay with color-0
  transparency over VDP-B blue reveal panels.
- No external bitmap or audio source files are required for this milestone.

Milestone-3 application:
- Tile-mode scenes stay assembly-authored and use a small reusable pattern set
  in Graphic 2 rather than imported tilesheets or text-mode glyphs.
- Sprite scenes use simple geometric sprite families with obvious footprints so
  size, magnification, overlap, and scanline-limit behavior remain readable
  from a single capture.

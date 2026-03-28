# Showcase ROM Workspace

This tree is reserved for the Vanguard 8 showcase ROM and its supporting
content pipeline. It is intentionally separate from `src/` and `tests/` so ROM
authoring, asset conversion, and showcase-specific regression data do not blur
into the emulator implementation itself.

Traceability:
- Hardware contract: `docs/spec/00-overview.md` through `docs/spec/04-io.md`
- Emulator compatibility envelope: `docs/emulator/08-compatibility-audit.md`
- Showcase plan: `showcase/docs/showcase-rom-plan.md`

Expected subtrees:
- `showcase/docs/` planning notes, content briefs, and asset requests
- `showcase/assets/` source art/audio provided for the ROM
- `showcase/src/` assembly source, linker/build config, and ROM-side code
- `showcase/tools/` asset conversion and packaging helpers
- `showcase/tests/` showcase-specific regression manifests and captures

The first deliverable in this workspace is the planning document in
`showcase/docs/showcase-rom-plan.md`.

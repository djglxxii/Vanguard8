# Showcase ROM Workspace

This tree is reserved for the Vanguard 8 showcase ROM and its supporting
authoring workflow. It is intentionally separate from the emulator
implementation so ROM source, generated asset provenance, and showcase-specific
regression data do not blur into `src/` and `tests/`.

Traceability:
- Hardware contract: `docs/spec/00-overview.md` through `docs/spec/04-io.md`
- Emulator compatibility envelope: `docs/emulator/08-compatibility-audit.md`
- Showcase plan: `showcase/docs/showcase-rom-plan.md`
- Showcase implementation plan: `showcase/docs/showcase-implementation-plan.md`
- Showcase milestone lock: `showcase/docs/current-milestone.md`

Expected subtrees:
- `showcase/docs/` planning notes, milestone contracts, and review checklists
- `showcase/assets/` generated or procedural source art/audio plus provenance
- `showcase/src/` assembly source, linker/build config, and ROM-side code
- `showcase/tools/` asset conversion and packaging helpers
- `showcase/tests/` showcase-specific regression manifests and captures
- `showcase/tasks/` planned, active, completed, and blocked task files

Workflow notes:
- Only the task file currently promoted into `showcase/tasks/active/` may be
  executed.
- Each completed task file becomes the human-review markdown artifact for that
  step.
- No later task or milestone should start until the human review of the prior
  task is complete and the milestone is accepted.

The current planning surfaces are:
- `showcase/docs/showcase-rom-plan.md`
- `showcase/docs/showcase-implementation-plan.md`
- `showcase/docs/current-milestone.md`
- `showcase/docs/agent-kickoff-prompts.md`

Stable wrapper interface:
- Build: `python3 showcase/tools/package/build_showcase.py`
- Run headless: `python3 showcase/tools/package/run_showcase_headless.py --frames 1 --trace build/showcase/m0.trace`

Milestone 0 output layout:
- `build/showcase/showcase.rom`
- `build/showcase/showcase.sym`
- `build/showcase/*.trace`

Milestone 1 review surfaces:
- Headless checkpoint run:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 240 --trace build/showcase/m1.trace --hash-frame 239 --symbols build/showcase/showcase.sym`
- Checkpoint notes:
  `showcase/tests/milestone-01-checkpoints.md`

Milestone 2 review surfaces:
- Asset guidance:
  `showcase/docs/asset-guidelines.md`
- Provenance note:
  `showcase/assets/provenance/milestone-02-scenes.md`
- Headless checkpoint notes:
  `showcase/tests/milestone-02-checkpoints.md`
- Contract run:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 480 --trace build/showcase/m2.trace --hash-frame 479 --symbols build/showcase/showcase.sym`

Milestone 3 review surfaces:
- Provenance note:
  `showcase/assets/provenance/milestone-03-scenes.md`
- Headless checkpoint notes:
  `showcase/tests/milestone-03-checkpoints.md`
- Contract run:
  `python3 showcase/tools/package/run_showcase_headless.py --frames 720 --trace build/showcase/m3.trace --hash-frame 719 --symbols build/showcase/showcase.sym`

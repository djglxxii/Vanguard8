# Vanguard 8 Showcase ROM — Incremental Implementation Plan

## Purpose

This plan breaks showcase-ROM work into ordered milestones that can be
implemented, reviewed, and accepted incrementally. It is derived from
`showcase/docs/showcase-rom-plan.md` and uses the same lock-step milestone/task
discipline as the emulator plan, but scoped to the showcase workspace.

Traceability:
- Hardware contract: `docs/spec/00-overview.md` through `docs/spec/04-io.md`
- Emulator compatibility envelope: `docs/emulator/08-compatibility-audit.md`
- Bring-up tooling posture: `docs/emulator/milestones/14.md`
- Showcase ROM plan: `showcase/docs/showcase-rom-plan.md`

## Planning Rules

- The spec remains authoritative. If the ROM needs behavior the spec does not
  define, update the spec first or stop.
- The ROM exists to verify documented emulator behavior, not to justify new
  emulator behavior.
- Keep generated assets coherent, simple, and reproducible. Avoid noisy content
  that makes failures harder to spot.
- Every milestone must leave the showcase workspace in a reviewable state.
- Prefer deterministic headless verification alongside human-visible scenes.
- Keep build and verification entry points stable once milestone 0 defines
  them.
- Do not silently broaden scope into emulator-core work, CI polish, or later
  showcase scenes before the active milestone is accepted.
- If a showcase task exposes an emulator-core defect, stop and record it as a
  blocked showcase task. Resolve the defect through the main emulator
  milestone/task flow first, then resume the showcase task after the emulator
  verification passes.

## Explicit Deferrals

These areas stay out of scope unless the spec or this plan is revised first:

- Text 1 or Text 2 showcase scenes
- Graphic 5, Graphic 6, or Graphic 7 showcase scenes
- Horizontal scroll via `R#26`/`R#27`
- Per-line horizontal parallax built on unverified scroll behavior
- Any invented Graphic 3 `LN=1` background fetch behavior for lines `192-211`
- 4-player expansion behavior
- Battery-backed cartridge SRAM mapping details
- Showcase-only emulator features added just for polish

## Milestone Control Files

Use the following files to keep showcase work locked to one step at a time:

- `showcase/docs/current-milestone.md`
  Declares the only showcase milestone currently allowed to advance.
- `showcase/docs/milestones/NN.md`
  Per-milestone contract files with allowed scope, explicit non-goals, allowed
  paths, required verification, exit criteria, and forbidden extras.
- `showcase/docs/milestone-acceptance-checklist.md`
  Human review checklist used before moving a showcase milestone from
  `ready_for_verification` to `accepted`.
- `showcase/tasks/`
  Planned, active, completed, and blocked task files for showcase work.

## Human Review Gate

The showcase ROM uses a stricter review cadence than the emulator plan:

- Each milestone contains one execution task.
- The completed task file is the human-review markdown artifact.
- After a task is completed, append the completion summary and move the task
  file to `showcase/tasks/completed/`.
- Do not promote the next task into `showcase/tasks/active/` until a human has
  reviewed and approved the completed task summary.
- Do not advance `showcase/docs/current-milestone.md` to the next milestone
  until the current milestone is accepted.

Workflow:

1. Lock work to the milestone named in `showcase/docs/current-milestone.md`.
2. Promote only that milestone's planned task into `showcase/tasks/active/`
   after explicit human approval to begin.
3. Implement only the active task and the current milestone contract.
4. Append the completion summary, move the task to `completed/`, and stop for
   human review.
5. Mark the milestone `ready_for_verification` only after the task is complete
   and the milestone's verification commands pass.
6. Mark the milestone `accepted` only after human review and the acceptance
   checklist are both satisfied.
7. Advance the lock and promote the next task only after acceptance.

## Milestone Summary

| Milestone | Goal |
|-----------|------|
| 0 | Bootstrap the showcase ROM source tree, stable build wrappers, and hello ROM |
| 1 | Bring up boot/MMU/vector flow, scene loop, and first deterministic checkpoint |
| 2 | Add generated-asset conventions plus title and dual-VDP compositing scenes |
| 3 | Add tile-mode and sprite verification scenes |
| 4 | Add mixed-mode and Graphic 4 command-engine verification scenes |
| 5 | Add the audio verification scene and deterministic audio checkpointing |
| 6 | Add the system-validation scene, regression manifests, and freeze docs |

## Milestones

### Milestone 0 — Showcase Workspace Bootstrap and Stable Build Entry Points

Objective:
- Create the initial ROM-side source tree, deterministic build wrappers, and a
  minimal bootable ROM image that proves the showcase workspace can produce
  `showcase.rom` and `showcase.sym`.

Deliverables:
- ROM source entry point under `showcase/src/`
- Stable build wrapper under `showcase/tools/package/`
- Stable headless-run wrapper under `showcase/tools/package/`
- Output layout for ROM, symbol, and capture artifacts
- Minimal bootable ROM that reaches an identifiable screen state

Exit criteria:
- A developer can run one documented build command and get both a ROM and a
  symbol file.
- A developer can run one documented headless command against that ROM and
  capture a trace or frame hash.

### Milestone 1 — Bring-Up Cartridge, Scene Loop, and First Checkpoint

Objective:
- Prove the ROM-side boot flow, MMU/vector setup, scene sequencing, and first
  banked-content transition using deterministic timing and traceable symbols.

Deliverables:
- Boot/reset code
- MMU and interrupt vector setup
- Scene loop scaffold
- First bank switch in the ROM flow
- First checkpoint manifest entry

Exit criteria:
- The ROM can boot, advance through a deterministic loop, cross at least one
  banked-content boundary, and expose a stable trace/frame checkpoint.

### Milestone 2 — Generated Asset Conventions, Title, and Dual-VDP Compositing

Objective:
- Lock the generated-asset approach and deliver the first coherent
  verification-facing scenes: the identity screen and the compositing scene.

Deliverables:
- Asset-guideline doc for generated source assets
- Provenance note format for generated assets
- Title/identity scene
- Dual-VDP compositing scene

Exit criteria:
- The ROM presents a coherent visual language without requiring external art.
- The compositing scene makes VDP-A transparency and VDP-B reveal behavior
  visually obvious and checkpointable.

### Milestone 3 — Tile-Mode and Sprite Verification Scenes

Objective:
- Add the first scenes that directly exercise Graphic 1/2 and the covered
  sprite behavior needed for title/menu-style ROM validation.

Deliverables:
- Tile-mode presentation scene
- Sprite capability scene
- Updated checkpoint manifest entries for those scenes

Exit criteria:
- Graphic 1/2 and sprite behavior are visible in stable scenes whose failures
  are obvious from screenshots and frame hashes.

### Milestone 4 — Mixed-Mode and Graphic 4 Command-Engine Scenes

Objective:
- Add the remaining planned video verification scenes that depend on covered
  Graphic 3/4 mixed-mode behavior and Graphic 4 command execution.

Deliverables:
- Graphic 3 + Graphic 4 mixed-mode scene
- Graphic 4 command-engine scene
- Updated checkpoint manifest entries for those scenes

Exit criteria:
- The ROM covers the planned mixed-mode and command-engine validation paths
  without relying on unresolved scroll or Graphic 3 line-fetch behavior.

Implementation note:
- Milestone 4 originally exposed a timed-HD64180 `EI` handling bug plus a
  packaged bank-layout split in the current `sjasmplus --raw` output above the
  `0x18000` region. The current repo carries both fixes and the milestone-4
  checkpoint artifacts needed for review.

### Milestone 5 — Audio Verification Scene and Audio Checkpoints

Objective:
- Add one coherent scene that isolates YM2151, AY-3-8910, MSM5205, and mixed
  playback in a deterministic loop suitable for audio hashing and trace review.

Deliverables:
- Audio verification scene
- Deterministic chip-sequence source data
- Audio checkpoint notes and expected hash capture flow

Exit criteria:
- The ROM exposes stable audio-only and mixed-audio windows suitable for human
  listening checks and headless hashing.

### Milestone 6 — System Validation Scene, Regression Manifests, and Freeze

Objective:
- Finish the ROM as a practical regression artifact by adding the system
  validation scene, freezing checkpoint manifests, and documenting the final
  human-review handoff.

Deliverables:
- System validation scene
- Final frame/audio/trace checkpoint manifests
- Replay inputs if needed
- Final review notes for the full loop

Exit criteria:
- The full loop can be rebuilt, run headlessly, and reviewed by a human using
  only the documented showcase commands and manifests.

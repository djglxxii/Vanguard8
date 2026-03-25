# AGENTS.md

## Purpose

This repository currently contains the hardware spec, emulator design documents,
and milestone/task workflow for a faithful emulator of the hypothetical
**Vanguard 8** hardware. The implementation tree may be absent or partially
present while the project is being reset or rebuilt from scratch. The main risk
is context drift:
features getting added, simplified, or "cleaned up" until the emulator no
longer matches the written hardware spec.

This file is here to prevent that. Treat it as the operating contract for any
coding agent working in this repo.

---

## Source of Truth

Use the repo's spec documents as the authority, not memory and not generic
"similar system" assumptions.

### Canonical documents

1. `docs/spec/00-overview.md`
   System-level architecture, memory map, port map, interrupt wiring, cartridge
   limits, and cross-subsystem constraints.
2. `docs/spec/01-cpu.md`
   HD64180 behavior, MMU, DMA, timers, and interrupt semantics.
3. `docs/spec/02-video.md`
   Dual-V9938 architecture, compositing, palettes, bitmap layout, commands,
   scrolling, and sprite behavior.
4. `docs/spec/03-audio.md`
   YM2151, AY-3-8910, MSM5205, port behavior, and mixing model.
5. `docs/spec/04-io.md`
   Controller input, timing reference, interrupt handling flow, ROM banking, and
   reset behavior.

### Non-authoritative documents

- `README.md` is a summary, not a hardware authority.
- If `README.md` conflicts with `docs/spec/*.md`, follow `docs/spec/*.md`.

### Conflict rule

- `00-overview.md` defines global system contracts.
- `01` through `04` define subsystem detail.
- If two spec files appear to disagree, do not silently choose one. Stop,
  document the conflict, and resolve it in the spec before locking behavior into
  code.

### External references

- External chip manuals may be used only when the repo spec explicitly says a
  behavior still needs verification.
- Do not let external docs quietly override this repo's written spec.
- If external research changes an implementation decision, update the spec in
  the same change or before the code change.

---

## Core Hardware Invariants

These are the baseline facts that must stay stable unless the spec itself
changes.

- CPU is a **Hitachi HD64180** clocked at **7.15909 MHz** from a
  **14.31818 MHz** master crystal.
- Logical memory map is fixed:
  `0x0000-0x3FFF` fixed ROM, `0x4000-0x7FFF` banked ROM window,
  `0x8000-0xFFFF` system SRAM.
- Physical SRAM base is `0xF0000`; boot MMU setup is `CBAR=0x48`,
  `CBR=0xF0`, `BBR=0x04`.
- Maximum cartridge ROM is **960 KB**.
- `BBR >= 0xF0` is illegal because it maps into SRAM space.
- Video is two fully capable **V9938** chips supporting all V9938 display
  modes, composited digitally. The primary/default mode is **Graphic 4 /
  Screen 5** (256×212, 4bpp bitmap); developers may configure each VDP in any
  V9938 mode, including different modes on each chip simultaneously.
- VDP-A sits in front of VDP-B; VDP-A color index `0` is transparent and lets
  VDP-B show through (in all palette-based modes, when R#8 TP bit is set).
- VDP-B interrupt output is **not** connected to the CPU.
- Audio is exactly three chips: **YM2151**, **AY-3-8910**, and **MSM5205**.
- INT0 is shared by **VDP-A** and **YM2151**. INT1 is dedicated to
  **MSM5205 /VCLK**. There is no NMI source.
- Controllers are read from ports `0x00` and `0x01` and are **active low**.
- The design constraint is still in force: **no custom silicon**.

If code contradicts any of the above, assume the code is wrong unless the spec
was intentionally revised.

---

## Spec-Lock Workflow

Follow this sequence before making any non-trivial code change.

1. Identify the subsystem.
   CPU, memory, video, audio, timing, interrupts, cartridge, or input.
2. Re-open the relevant spec files.
   Do not rely on prior context from an earlier turn.
3. Extract the exact rules you are implementing.
   Use register names, port numbers, address ranges, timing constants, and
   interrupt lines exactly as written.
4. Implement only documented behavior.
   If the spec does not say it, do not invent it.
5. Encode the rule in tests.
   Tests should pin behavior to the written spec, not to a convenience model.
6. If you must make a new architectural choice, update the spec first or in the
   same change.

For substantial work, keep a short traceability note in your reasoning or change
summary of the form:

`Implemented against docs/spec/02-video.md (compositing, palette, sprite limits)`

That is not bureaucracy. It is anti-drift protection.

---

## Milestone-Lock Workflow

Implementation work is additionally constrained by the active milestone
contract.

Canonical milestone control files:

- `docs/emulator/07-implementation-plan.md`
  Ordered milestone definitions, tests, and exit criteria.
- `docs/emulator/current-milestone.md`
  The only milestone currently allowed to be implemented.
- `docs/emulator/milestones/`
  One contract file per milestone, defining allowed scope, non-goals, allowed
  paths, required tests, exit criteria, forbidden extras, and verification
  commands.
- `docs/process/milestone-acceptance-checklist.md`
  Human review checklist before a milestone is accepted.
- `docs/tasks/active/`
  Queue of the task files the executing coding agent is allowed to pick up.
- `docs/tasks/completed/`
  Completed task files, kept with their final completion summary.
- `docs/tasks/blocked/`
  Tasks that could not be completed, kept with their incompletion summary and
  blockers.
- Milestone contract `Verification Commands`
  These are the authoritative verification steps until a shared verifier script
  is added to the repo.

Before making any implementation change:

1. Read `docs/emulator/current-milestone.md`.
2. Read the matching file in `docs/emulator/milestones/`.
3. Re-open only the spec and emulator design docs listed by that milestone.
4. Restate the current milestone's:
   objective, allowed scope, explicit non-goals, required tests, and exit
   criteria.
5. Make changes only within the milestone's allowed paths unless the milestone
   contract itself is updated first.
6. Do not implement deliverables from any later milestone, even if they feel
   convenient or architecturally adjacent.
7. Before marking a milestone ready for review, run the active milestone's
   declared verification commands.
8. Before marking a milestone accepted, rerun the active milestone's declared
   verification commands and check
   `docs/process/milestone-acceptance-checklist.md`.
9. If an active task file exists for the milestone, follow that task file as the
   execution contract and produce the required completion or incompletion
   summary.

Milestone discipline rules:

- If a requested change is outside the active milestone contract, stop and say
  so.
- If a change seems to need later-milestone infrastructure, implement the
  narrowest documented version that satisfies the current milestone only.
- Do not silently "prepare for later" by adding extra subsystems, UI, debugger
  hooks, replay features, or speculative abstractions.
- If the current milestone contract is too weak to support a safe change,
  strengthen the contract first, then code against it.
- When a task is completed, move its file from `docs/tasks/active/` to
  `docs/tasks/completed/` and append the required completion summary.
- When a task cannot be completed, move its file from `docs/tasks/active/` to
  `docs/tasks/blocked/` and append the required incompletion summary with
  concrete blockers.

The active milestone is a hard scope boundary, not a suggestion.

---

## Implementation Rules

### General

- Emulate the specified machine, not a nearby real-world console.
- Prefer explicit hardware-model names over generic abstractions. Example:
  `bbr`, `cbar`, `int0_pending`, `vdp_a`, `vdp_b`, `msm5205_vclk`.
- Preserve distinctions the spec cares about, even if they are inconvenient.
  Example: INT0 behavior versus INT1 vectored behavior.
- Keep clock derivations rooted in the **14.31818 MHz** master clock unless the
  spec says otherwise.

### When behavior is unspecified

- Stop and mark it as unspecified.
- Do not smooth over the gap with guessed behavior, "reasonable defaults," or
  another machine's behavior.
- Add a narrow `TODO(spec)` comment only when necessary, and mention the exact
  missing document or unresolved question.

### When behavior is approximate

- If the spec gives a qualitative or approximate rule, implement only the level
  of precision the spec currently supports.
- Do not pretend precision that the spec does not claim.

### Documentation discipline

- If code exposes a missing or ambiguous rule, fix the spec, not just the code.
- Keep terminology consistent with the spec docs.
- Avoid introducing alternate names for the same register, flag, or memory
  region unless the spec already uses them.

---

## Current "Do Not Invent" Areas

These topics are explicitly not stable enough for freeform implementation.

- **Horizontal scroll behavior (R#26/R#27)**
  `docs/spec/02-video.md` and `docs/spec/04-io.md` both say R#26/R#27 behavior
  needs verification against the V9938 Technical Data Book before relying on it
  in any mode.
- **Per-line horizontal parallax**
  Do not treat it as fully specified until the horizontal scroll behavior is
  verified and written into the repo spec.
- **4-player expansion**
  Planned, but not fully specified. Only the 2-controller interface is fully
  defined today.
- **Optional battery-backed cartridge SRAM mapping details**
  Mentioned in overview, but not defined deeply enough yet for a complete mapper
  model. Do not invent banking or arbitration rules.

If asked to implement one of these areas, either update the spec first or make
the limitation explicit in code and tests.

---

## Testing Expectations

- Every bug fix or feature should tighten spec coverage.
- Tests should reference the hardware rule they protect in nearby comments or in
  the test name when practical.
- Good candidates for early invariant tests include:
  - MMU boot mapping and illegal `BBR` range.
  - ROM bank window behavior.
  - INT0 versus INT1 dispatch semantics.
  - VDP-A-over-VDP-B compositing with color `0` transparency.
  - Controller active-low input decoding.
  - VDP-B not generating CPU interrupts.

Do not loosen a test to match a speculative implementation. Fix the model or
fix the spec.

---

## Working Posture For Future Agents

- Re-anchor yourself in the spec at the start of each task.
- Be suspicious of your own memory.
- Be more comfortable saying "unspecified" than inventing behavior.
- Keep code, tests, and docs aligned in the same change whenever possible.

The repository's value is not just that it produces an emulator. Its value is
that the emulator remains traceable to a coherent written hardware definition.

# Vanguard 8 Emulator — Compatibility Audit Notes

## Purpose

This note records what milestone 11 has actually audited about compatibility,
what the current repo proves with tests, and what still remains a documented
implementation assumption.

It is intentionally narrower than the hardware spec. The spec stays
authoritative; this file records how much of that contract the current
implementation has been checked against.

## Z180 / HD64180 Audit Status

The emulator still uses the extracted MAME Z180 core in `third_party/z180/`
through `src/core/cpu/z180_adapter.cpp`. The repo has **not** independently
proven that every HD64180 behavior matches that core. The current status is:

- Verified in repo tests:
  reset defaults, MMU translation through `CBAR`/`CBR`/`BBR`, `OUT0` handling
  for the internal MMU registers, `INT0` IM1 dispatch, `INT1` vectored
  `I`/`IL` dispatch, DMA channel surfaces, and HD64180 PRT timer vectoring.
- Verified in repo integration paths:
  scheduler-driven interrupt timing, bank-switch logging, save-state reload,
  rewind, replay, and headless regression execution all run through the same
  extracted CPU adapter surface.
- Not independently audited against external chip manuals:
  full opcode parity, `MLT`, the broader `IN0` internal-register surface beyond
  the ports covered by tests, undocumented flag edge cases, and any behavior
  outside the repo's implemented opcode subset.

Current conclusion:

- The repo has a **tested Vanguard 8 execution subset** backed by deterministic
  regression coverage.
- The broader statement "Z180 equals HD64180 in all respects relevant to all
  future software" remains an **implementation assumption**, not a closed
  audit finding.

## Real-ROM Compatibility Status

The repo currently contains deterministic fixture assets under `tests/replays/`
and a replay-backed headless regression command. These prove the current
emulator stays stable for the covered fixture session.

What has been run from repo state:

- full `ctest` regression
- replay-backed headless regression
- long-running save/replay determinism stress coverage

What is still limited:

- No external game-ROM corpus is checked into the repo.
- No compatibility matrix is yet recorded for commercial or homebrew software.
- Any broader real-ROM pass depends on obtaining ROMs whose licensing and test
  policy are acceptable for this project.

## Current Performance / Correctness Posture

The current milestone-11 repo is strongest where the spec is explicit and where
tests pin the behavior:

- scheduler timing and interrupt cadence
- save-state and replay determinism
- covered VDP rendering paths and compositing rules
- documented audio surface timing

The main remaining compatibility risk is not obvious nondeterminism; it is
**surface area that is still intentionally narrow** relative to full HD64180
and full V9938 software expectations.

# Current Milestone Lock

- Active milestone: `37`
- Title: `Timed HD64180 LD E,n Opcode Coverage for PacManV8 T020`
- Status: `ready_for_verification`
- Locked on plan: `docs/emulator/07-implementation-plan.md`
- Contract file: `docs/emulator/milestones/37.md`

Execution rules:
- Only milestone `37` task files present in `docs/tasks/active/` may be
  executed.
- Milestone `31` is accepted.
- Milestone `32` is accepted. The frontend audio plumbing shipped on
  2026-04-19; the underlying CPU/audio co-scheduling defect that made
  PacManV8 inaudible was out of M32 scope and was fixed by M33-T01.
  `M32-T01-frontend-live-audio-playback.md` now lives in
  `docs/tasks/completed/` with a completion summary that cites M33-T01 as
  the resolving change.
- Milestone `33` verification commands were rerun on 2026-04-20 after
  refreshing the stale PacManV8 T017 300-frame audio regression hash to
  `24ce40791e466f9f686ee472b5798128065458e06a51f826666ae444ddfb5c75`.
- Milestone `34` is accepted. Task `M34-T01` is in `docs/tasks/completed/`.
- Milestone `35` is accepted. Task `M35-T01` is in
  `docs/tasks/completed/`.
- Milestone `36` is accepted on 2026-04-21. Task `M36-T01` is in
  `docs/tasks/completed/`. The HALT-resume fix (a dedicated
  `Core::wake_from_halt_for_interrupt()` helper that clears `halted_`
  and bumps `pc_` past the HALT opcode before the interrupt service
  pushes the return address) was verified via `ctest` at `173/173` and
  by the PacManV8 T020 runtime evidence showing
  `GAME_FLOW_CURRENT_STATE` now advances past its initial ATTRACT
  value.
- Milestone `37` is `ready_for_verification`. Added timed-core support
  for `LD E,n` (`0x1E`), `LD H,n` (`0x26`), and `LD L,n` (`0x2E`)
  (precedent: M34 family closure). Four new focused CPU tests pin
  each destination register, immediate-byte semantics, `PC+2`
  advancement, flag preservation, and the exact
  `LD D,n → LD E,n → LD A,n` sequence at PacManV8 `0x332C..0x3331`.
  Adapter T-state timing entries added to the existing 7-T-state
  case group in `src/core/cpu/z180_adapter.cpp`. `ctest` is green at
  `177/177` (173 prior + 4 new), no pre-existing test relaxed. The
  canonical PacManV8 T020 repro
  (`vanguard8_headless --rom build/pacman.rom --frames 1020 ...`)
  runs to completion without the `Unsupported timed Z180 opcode 0x1E
  at PC 0x332E` abort, and three repeat runs emit identical frame
  SHA-256 `a79b667a...` and event-log digest `11584233142677547359`,
  confirming determinism.
- Keep milestone `37` work inside the allowed paths declared in
  `docs/emulator/milestones/37.md`: `third_party/z180/`, `src/core/cpu/`,
  `tests/`, and the doc/task files listed in the contract.
- Do not bundle new opcode coverage beyond the three `LD r,n` forms,
  interrupt-acceptance rewiring, HALT-wake changes, VDP / audio /
  scheduler / frontend work, M35 CLI changes, or PacManV8 ROM edits
  into this milestone.
- Canonical Vanguard8 headless binary for verification is
  `cmake-build-debug/src/vanguard8_headless`.

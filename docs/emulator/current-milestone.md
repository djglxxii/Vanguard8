# Current Milestone Lock

- Active milestone: `38`
- Title: `Timed HD64180 RET NC Opcode Coverage for PacManV8 T020`
- Status: `accepted`
- Locked on plan: `docs/emulator/07-implementation-plan.md`
- Contract file: `docs/emulator/milestones/38.md`

Execution rules:
- Milestone `38` is accepted on 2026-04-21. Task `M38-T01` is in
  `docs/tasks/completed/`. Timed-core support for `RET NC` (`0xD0`)
  and `RET C` (`0xD8`) was verified via `ctest` at `178/178`
  (177 prior + 1 new test case covering five sections) and by the
  PacManV8 T020 runtime evidence showing the scene-1 → scene-2
  review-index advance now executes past `PC=0x2F75` and produces
  stable frame SHA-256 hashes at frame `1770`
  (`082dd26d090aa2632128d266bddd6faa0c50b5b96819dfc7e6f02f3276d93066`,
  digest `3498303648502710181`), frame `2520`
  (`c80b7468c3a56d2383582b717f17bc36fde775f5821ab4311081d796e4f0a2ff`,
  digest `10606555846525264891`), and frame `2640`
  (`31f8226ca0fe920a1b85e33ecd0625ba0846439a3a15d146e1497c817285d34c`,
  digest `6526782969573701363`) across repeat runs, without the
  `Unsupported timed Z180 opcode 0xD0 at PC 0x2F75` abort.
- No new milestone is active until a milestone `39` contract is
  defined in `docs/emulator/milestones/` and this lock file is
  updated to point at it. Do not open new task files under
  `docs/tasks/active/` without that contract.
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
- Milestone `37` is accepted on 2026-04-21. Task `M37-T01` is in
  `docs/tasks/completed/`. Timed-core support for `LD E,n` (`0x1E`),
  `LD H,n` (`0x26`), and `LD L,n` (`0x2E`) was verified via `ctest`
  at `177/177` (173 prior + 4 new) and by the PacManV8 T020 runtime
  evidence showing scene-1 intermission drawing now captures at
  frame `1020` with stable frame SHA-256 `a79b667a...` and event-log
  digest `11584233142677547359` across three repeat runs, without
  the `Unsupported timed Z180 opcode 0x1E at PC 0x332E` abort.
- Canonical Vanguard8 headless binary for verification is
  `cmake-build-debug/src/vanguard8_headless`.

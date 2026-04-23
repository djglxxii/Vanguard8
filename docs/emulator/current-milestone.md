# Current Milestone Lock

- Active milestone: `43`
- Title: `Logical Peek Row Prefix Format for PacManV8 T021`
- Status: `active`
- Locked on plan: `docs/emulator/07-implementation-plan.md`
- Contract file: `docs/emulator/milestones/43.md`

Execution rules:
- Milestone `43` was activated on 2026-04-23 to close the next
  real-ROM T021 blocker exposed after M42 added the authorized
  `SCF` (`0x37`) timed opcode coverage. With SCF running, the
  PacManV8 T021 `pattern_replay_tests.py` harness no longer
  aborts at `PC=0x1315`, and the headless binary now runs to the
  requested `--inspect-frame` checkpoint and produces a valid
  inspection report. The harness then fails with
  `pattern_replay_tests.py error: inspection report did not
  contain logical 0x8270:13`. The root cause is emitter-side:
  `src/frontend/headless_inspect.cpp::append_byte_row` formats
  both `[peek-mem]` (20-bit physical) and `[peek-logical]`
  (16-bit logical) row prefixes with `hex20`, producing 5-digit
  logical-row prefixes (`  0x08270:`) that silently fail the
  harness's 4-digit `BYTE_ROW_PATTERN` regex. The authoritative
  fix is narrow and emitter-side — logical-peek rows must use
  a 16-bit width matching the block header's `logical 0xHHHH`
  format — and the matching task file is
  `docs/tasks/active/M43-T01-logical-peek-row-prefix-format.md`.
  Authorized implementation scope is limited to the logical-peek
  row format, refreshed tests/golden, and the declared
  verification commands. No CPU opcode work, no physical-peek
  format changes, no other observability surface changes.
- Milestone `42` is accepted on 2026-04-23. Task `M42-T01` is in
  `docs/tasks/completed/`. Timed-core support for `SCF` (`0x37`)
  was verified via `ctest` at `191/191` (190 prior + 1 new test
  case covering five sections) and by the PacManV8 T021 runtime
  evidence showing the canonical replay harness now runs past
  `PC=0x1315` without an `Unsupported timed Z180 opcode 0x37`
  abort. The headless binary runs to the requested checkpoint
  and produces a valid inspection report (representative frame
  SHA-256 at frame 60:
  `4a63cec305375edd4b20e85ba9830d83888e2eaf4327a29c229cfc7ce7a79693`,
  event-log digest `6563162820683566367`). The PacManV8
  `pattern_replay_tests.py` harness now surfaces a separate,
  non-opcode blocker — its `BYTE_ROW_PATTERN` regex expects
  4-digit hex row prefixes while Vanguard 8's
  `src/frontend/headless_inspect.cpp::append_byte_row` formats
  logical-peek row addresses with `hex20` (5 digits) — which is
  outside M42's allowed paths and is routed to milestone `43`.
  No new CPU-side unsupported-opcode blocker was exposed.
- Milestone `42` was activated on 2026-04-23 to close the next
  real-ROM timed CPU compatibility gap exposed after M41. The
  PacManV8 T021 replay validation harness reported
  `Unsupported timed Z180 opcode 0x37 at PC 0x1315` (`SCF`)
  immediately after the `collision_prepare_tile` divide-by-eight
  sequence, used as the "return with carry set" idiom
  (`SCF` followed by `RET`). The immediate downstream path is
  already fully timed, so no look-ahead opcode was bundled. The
  matching task file now lives at
  `docs/tasks/completed/M42-T01-timed-scf-opcode-coverage.md`.
  Authorized implementation scope was limited to `SCF` (`0x37`),
  focused tests, non-perturbation evidence, and the declared
  verification commands.
- Milestone `41` is blocked on 2026-04-23 after `M41-T01` implemented
  the authorized `SRL H` (`0xCB 0x3C`) and `RR L` (`0xCB 0x1D`) timed
  CB-prefix surface and verified it with focused CPU coverage
  (67 cases, 815 assertions), full `ctest` (190/190 passed), the
  non-perturbation fixture replay (byte-identical digests to M40),
  and the declared headless smoke proof. The PacManV8 T021 replay
  validation harness now runs past the previous `PC=0x1302` `SRL H`
  blocker and the immediate `RR L` sites, then reaches a new
  out-of-scope timed opcode: `Unsupported timed Z180 opcode 0x37 at
  PC 0x1315` (`SCF`, Set Carry Flag). `M41-T01` now lives in
  `docs/tasks/blocked/` with the incompletion summary and
  verification evidence. No new milestone is active until a
  follow-up contract is defined in `docs/emulator/milestones/` and
  this lock file is updated to point at it.
- Milestone `41` was activated on 2026-04-22 to close the next
  real-ROM timed CPU compatibility gap exposed after M40. The
  PacManV8 blocked task
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`
  reported `Unsupported timed Z180 opcode 0xCB 0x3C at PC 0x1302`
  (`SRL H`) in the `collision_prepare_tile` divide-by-eight path,
  and identified the immediate same-path `RR L` (`0xCB 0x1D`)
  look-ahead gap. The matching task file is
  `docs/tasks/blocked/M41-T01-timed-srl-h-and-rr-l-opcode-coverage.md`.
  Authorized implementation scope was limited to those two CB-prefix
  sub-opcodes, focused tests, non-perturbation evidence, and the
  declared verification commands.
- Milestone `40` is blocked on 2026-04-22 after `M40-T01` implemented
  the authorized `EX DE,HL` (`0xEB`) and `OR (HL)` (`0xB6`) timed
  opcode surface and verified it with focused CPU coverage, full
  `ctest`, and the declared headless smoke proof. The PacManV8 T021
  replay validation harness now runs past the previous `PC=0x11DA`
  `EX DE,HL` blocker and the immediate `OR (HL)` site, then reaches a
  new out-of-scope timed CB-prefix opcode:
  `Unsupported timed Z180 opcode 0xCB 0x3C at PC 0x1302` (`SRL H` in
  `collision_prepare_tile`). `M40-T01` now lives in
  `docs/tasks/blocked/` with the incompletion summary and verification
  evidence. Milestone `41` is the follow-up contract that authorizes
  `CB 3C` and the immediate `CB 1D` look-ahead.
- Milestone `40` was activated on 2026-04-22 to close the next real-ROM
  timed CPU compatibility gap exposed after M39. The PacManV8 blocked
  task
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`
  now reports `Unsupported timed Z180 opcode 0xEB at PC 0x11DA`
  (`EX DE,HL`) in `collision_init`, and identifies the immediate
  same-path `OR (HL)` (`0xB6`) look-ahead gap. The matching task file
  is now
  `docs/tasks/blocked/M40-T01-timed-ex-de-hl-and-or-hl-opcode-coverage.md`.
  Authorized implementation scope is limited to those two opcodes,
  focused tests, non-perturbation evidence, and the declared
  verification commands.
- Milestone `39` is blocked on 2026-04-22 after `M39-T01` completed the
  authorized timed opcode surface but the PacManV8 T021 replay validation
  harness reached a new out-of-scope timed opcode:
  `Unsupported timed Z180 opcode 0xEB at PC 0x11DA` (`EX DE,HL`).
  `M39-T01` now lives in `docs/tasks/blocked/` with the incompletion
  summary and verification evidence. Milestone `40` is the follow-up
  contract that authorizes `0xEB` and the immediate `0xB6` look-ahead.
- Milestone `39` was planned on 2026-04-22 to close the next real-ROM
  timed CPU compatibility gap exposed by the PacManV8 blocked task
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`.
  The canonical headless runtime currently aborts with
  `Unsupported timed Z180 opcode 0xB at PC 0x1209` (`DEC BC` in
  `collision_init`) along the T021 replay validation path. The
  milestone covers `DEC BC` (`0x0B`), `INC BC` (`0x03`), `INC DE`
  (`0x13`), `DEC HL` (`0x2B`), and a narrow CB-prefix dispatch surface
  for `SRL A` (`CB 3F`) and `BIT 4..7,A` (`CB 67`/`6F`/`77`/`7F`),
  per the look-ahead in the T021 blocker. The matching `M39-T01` task
  was executed and moved to `docs/tasks/blocked/` after the follow-on
  out-of-scope `0xEB` blocker was exposed.
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

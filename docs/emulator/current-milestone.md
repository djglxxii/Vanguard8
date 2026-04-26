# Current Milestone Lock

- Active milestone: `47`
- Title: `Full MAME HD64180 Core Import (Retire Per-Opcode CPU Milestones)`
- Status: `active`
- Locked on plan: `docs/emulator/07-implementation-plan.md`
- Contract file: `docs/emulator/milestones/47.md`

Execution rules:
- Milestone `47` was activated on 2026-04-26 after a repository
  audit (`docs/emulator/16-rom-readiness-audit.md`) confirmed
  that the timed Z180 dispatch in
  `third_party/z180/z180_core.{hpp,cpp}` and
  `src/core/cpu/z180_adapter.cpp` is a milestone-2 boot stub with
  hand-rolled per-opcode timing tables. The user has rejected
  continuing the one-opcode-per-milestone pattern and authorized
  importing the full MAME HD64180 / Z180 core in a single pass.
  The Vanguard 8 *is* an HD64180 system; all HD64180
  functionality must be available by default.
- Authorized implementation scope is limited to:
  `third_party/z180/`, `src/core/cpu/`, `tests/test_cpu.cpp`,
  `tests/replays/` (digest re-pinning only — no fixture ROM or
  replay byte-stream changes),
  `docs/emulator/07-implementation-plan.md`,
  `docs/emulator/current-milestone.md`,
  `docs/emulator/milestones/47.md`,
  `docs/emulator/16-rom-readiness-audit.md` (status update only),
  and the `docs/tasks/` queues. No VDP, audio, scheduler,
  debugger, save-state format, headless format, or desktop GUI
  changes. No vendoring of a Z80-only core plus a local HD64180
  extension layer. No PacManV8 source edits.
- After M47 lands, "full opcode coverage" is the shipped state
  of `third_party/z180/`, not a non-goal. Any future ROM trap on
  a CPU dispatch path is a bug in the adapter or in the imported
  core's pin, not an invitation to open a new per-opcode
  milestone.
- Matching task file:
  `docs/tasks/active/M47-T01-mame-z180-core-import.md`.
- Milestone `46` was accepted on 2026-04-24. Task `M46-T01` lives
  in `docs/tasks/completed/`. The full ALU register/immediate
  timed tail (`ADC A,r`, `SUB r`, `SBC A,r`, `XOR r`, plus the
  matching immediates) is covered in the extracted core, and the
  PacManV8 T021 harness passes both replay cases end-to-end
  against the M46 tree. M47 supersedes the M46-era hand-rolled
  dispatch entirely; once M47 lands, the M46 range-checks no
  longer exist in the adapter (their semantics are subsumed by
  the imported core).

Pre-M47 execution rules (kept for traceability):
- Milestone `46` was activated on 2026-04-24 after the user pointed
  the Vanguard8 agent at the PacManV8 T021 blocked-task section
  `### Recommended fix (Vanguard8 repo)` in
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`
  and asked for a milestone built from that recommendation. At
  activation, the T021 replay harness aborted with `Unsupported timed
  Z180 opcode 0xD6 at PC 0x3EA` (`SUB n` in PacManV8
  `movement_distance_to_next_center_px:326`). The same blocked-task
  log enumerates the confirmed sibling `SUB B` (`0x90`) sites in
  the same function at `PC=0x3BE`/`0x3CA`/`0x3D2` and the strongly
  suspected `XOR r` (`0xA8..0xAD`, `0xAE`) and immediate-arithmetic
  peers (`ADC A,n` / `SBC A,n` / `XOR n`) that will surface along
  the same active replay path.
- Per the recommended fix and continuing the M44/M45 philosophy of
  consolidating remaining T021 closure work, milestone `46` covers
  the full register-form ALU tail (`ADC A,r` `0x88..0x8F`,
  `SUB r` `0x90..0x97`, `SBC A,r` `0x98..0x9F`, `XOR r`
  `0xA8..0xAF`) plus the matching immediate peers (`ADC A,n`
  `0xCE`, `SUB n` `0xD6`, `SBC A,n` `0xDE`, `XOR n` `0xEE`) in a
  single pass, so the T021 replay path cannot abort again on a
  sister ALU opcode inside the same rebuild cycle. If a further
  out-of-scope gap surfaces after M46, a new milestone contract is
  opened — M46 is not broadened.
- Authorized implementation scope is limited to timed extracted-
  core coverage for the opcodes above (expressed as range-checks
  alongside the existing `LD r,r'`, `ADD A,r`, and `AND r` range-
  checks), removal of the now-redundant explicit dispatch entries
  for `0x91`, `0x93`, and `0xAF`, focused CPU tests, non-
  perturbation regressions, and the listed doc/task files inside
  `third_party/z180/`, `src/core/cpu/`, `tests/`,
  `docs/emulator/07-implementation-plan.md`,
  `docs/emulator/current-milestone.md`,
  `docs/emulator/milestones/46.md`, and the `docs/tasks/` queues.
  No PacManV8 source edits from this repo, and no unrelated VDP,
  audio, scheduler, debugger, save-state, headless format, or
  desktop GUI work. No speculative broadening into rotate/shift,
  index-register, or ED-prefix families without a fresh T021
  repro. No edits to the existing explicit `OR r` (`0xB0..0xB7`) or
  `CP r` (`0xB8..0xBF`) entries — the recommended fix explicitly
  excludes those ranges.
- Matching task file:
  `docs/tasks/completed/M46-T01-timed-alu-register-immediate-opcode-coverage.md`.
- Milestone `46` implementation completed on 2026-04-24. The full
  ALU register/immediate timed tail (`ADC A,r`, `SUB r`, `SBC A,r`,
  `XOR r`, plus `ADC A,n` / `SUB n` / `SBC A,n` / `XOR n`) is
  covered in the extracted core and adapter timing path, focused CPU
  and full `ctest` verification passed, and the canonical PacManV8
  T021 harness passed both replay cases end-to-end. The milestone is
  ready for human verification/acceptance.
- Milestone `45` is blocked on 2026-04-23 after `M45-T01`
  completed the in-scope timed `AND r` / `AND (HL)` surface:
  `AND B` (`0xA0`), `AND C` (`0xA1`), `AND D` (`0xA2`),
  `AND E` (`0xA3`), `AND H` (`0xA4`), `AND L` (`0xA5`),
  `AND (HL)` (`0xA6`), and `AND A` (`0xA7`). Focused CPU coverage
  pins operand/result behavior, flags, `PC` advance, 4/7 T-state
  classification, the `AND (HL)` memory-read path, and a negative
  `XOR E` (`0xAB`) out-of-scope guard. `cmake --build
  cmake-build-debug` passed, `ctest --test-dir cmake-build-debug
  --output-on-failure` passed at `194/194` with the usual showcase
  skip, and the direct frame-40 repro now exits cleanly with event-log
  digest `9745782898622768779` and frame SHA-256
  `4a63cec305375edd4b20e85ba9830d83888e2eaf4327a29c229cfc7ce7a79693`.
  The canonical PacManV8 T021 harness now advances past the M45
  `AND E` / `AND (HL)` sites, then stops on a new out-of-scope timed
  opcode: `Unsupported timed Z180 opcode 0xD6 at PC 0x3EA`
  (`SUB n` in PacManV8 `movement_distance_to_next_center_px`,
  `/home/djglxxii/src/PacManV8/src/movement.asm:326`). `M45-T01`
  has been moved to
  `docs/tasks/blocked/M45-T01-timed-and-r-and-and-hl-opcode-coverage.md`.
  No new Vanguard8 milestone is active until a follow-up contract is
  defined.
- Milestone `45` was planned on 2026-04-23 after the PacManV8
  team resolved the out-of-scope fidelity issue that had blocked
  `M44-T01`. The canonical PacManV8 T021 harness now surfaces a
  new Vanguard8-side timed-opcode gap:
  `Unsupported timed Z180 opcode 0xA3 at PC 0x125B` (`AND E` in
  `collision_consume_tile`). The blocked-task log at
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`
  also identifies `AND (HL)` (`0xA6`) at `PC=0x1260` as the
  strongly suspected immediate follow-on.
- Per the user's 2026-04-23 direction, and continuing M44's
  philosophy of consolidating remaining T021 closure work,
  milestone `45` covers the full base `AND r` register family
  (`0xA0`–`0xA5`, `0xA7`) plus `AND (HL)` (`0xA6`) in one pass
  so the T021 replay path cannot abort again on a sister
  `AND r` opcode inside the same rebuild cycle. If a further
  out-of-scope gap surfaces after M45, a new milestone contract
  is opened — M45 is not broadened.
- Authorized implementation scope is limited to timed extracted-
  core coverage for the `AND` family opcodes above, focused
  tests, non-perturbation regressions, and the listed doc/task
  files inside `third_party/z180/`, `src/core/cpu/`, `tests/`,
  `docs/emulator/07-implementation-plan.md`,
  `docs/emulator/current-milestone.md`,
  `docs/emulator/milestones/45.md`, and the `docs/tasks/`
  queues. No PacManV8 source edits from this repo, and no
  unrelated VDP, audio, scheduler, debugger, save-state,
  headless format, or desktop GUI work. No speculative
  broadening into `OR r`, `XOR r`, `CP r`, rotate/shift,
  index-register, or ED-prefix families without a fresh T021
  repro.
- Matching task file:
  `docs/tasks/blocked/M45-T01-timed-and-r-and-and-hl-opcode-coverage.md`.
- Milestone `44` is blocked on 2026-04-23 after `M44-T01`
  completed the in-scope Vanguard8 timed-opcode work exposed by the
  PacManV8 T021 replay path. Timed support for `SUB E` (`0x93`),
  same-path `SUB C` (`0x91`), and follow-on `CPL` (`0x2F`) was added
  with focused CPU coverage. `ctest --test-dir cmake-build-debug
  --output-on-failure` passed at `193/193` with the usual showcase
  skip, and the direct frame-444 repro now completes with frame hash
  `4a63cec305375edd4b20e85ba9830d83888e2eaf4327a29c229cfc7ce7a79693`
  instead of aborting on `0x93`. The canonical PacManV8 T021 harness
  now runs without a Vanguard8 timed-opcode abort or headless report
  mismatch, but fails PacManV8 fidelity assertions: both replay cases
  observe score/dots `0/0` and Pac-Man tile `(14, 26)` where the
  harness expects progressed movement/scoring. The observed reports
  still show `active=1`, advancing `replay_frame`, and replay-driven
  `last_dir` changes, so the remaining blocker is PacManV8
  gameplay/expectation state outside milestone 44's allowed Vanguard8
  scope. `M44-T01` has been moved to `docs/tasks/blocked/`.
- Milestone `44` was activated on 2026-04-23 after reviewing the
  latest blocker ledger in
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`.
  Milestones `39` through `43` each closed one distinct T021 blocker,
  but the replay path still exposes additional Vanguard8-side gaps.
  The current confirmed blocker is
  `Unsupported timed Z180 opcode 0x93 at PC 0xFAE` (`SUB E`) in
  `ghost_update_all_targets`, and the same blocked-task log identifies
  same-path `SUB C` (`0x91`) sites as the most plausible immediate
  follow-on traps. Milestone `44` replaces the prior
  one-blocker-per-milestone pattern for the rest of T021.
- Matching task file:
  `docs/tasks/blocked/M44-T01-pacmanv8-t021-remaining-compatibility-closure.md`
  after the 2026-04-23 incompletion summary. During execution, every
  newly surfaced in-scope T021 blocker was recorded in that single
  task file instead of opening `M45+` micro-milestones.
- Authorized implementation scope is limited to timed HD64180 opcode
  coverage and narrow headless replay, inspection, or reporting fixes
  directly proven by the T021 repro path inside `third_party/z180/`,
  `src/core/cpu/`, `src/frontend/headless*`, `tests/`, and the listed
  doc/task files. No PacManV8 source edits from this repo, and no
  unrelated VDP, audio, scheduler, debugger, save-state, or desktop
  GUI work.
- The PacManV8 blocked-task log dated 2026-04-23 also confirms the
  earlier parser mismatch was fixed on the PacManV8 side and the replay
  now advances past the M43 row-prefix issue, so the remaining blockers
  are back on the Vanguard8 side unless proven otherwise.
- Milestone `43` is blocked on 2026-04-23 after `M43-T01`
  completed the in-scope Vanguard8 emitter-side change:
  logical peek rows now emit 4-digit `0xHHHH:` prefixes,
  physical peek rows still emit 5-digit `0xHHHHH:` prefixes,
  the headless observability golden/assertions were refreshed,
  `ctest --test-dir cmake-build-debug --output-on-failure`
  passed at `191/191` with the usual showcase skip, and
  `vanguard8_headless_replay_regression` remained pinned. A
  direct PacManV8 smoke run with `--peek-logical 8270:13
  --peek-mem F0270:13` now emits:
  `logical 0x8270 physical 0xf0270 region ca1 length 13`
  followed by row prefix `  0x8270:`. The canonical PacManV8
  `pattern_replay_tests.py` command still exits with
  `inspection report did not contain logical 0x8270:13`, but
  the remaining blocker is external to Vanguard8:
  `/home/djglxxii/src/PacManV8/tools/pattern_replay_tests.py`
  currently defines
  `BYTE_ROW_PATTERN = re.compile(r"^\\s+0x([0-9a-f]{4}):((?: [0-9a-f]{2})+)$")`,
  which matches a literal `\s` sequence rather than leading
  whitespace and therefore cannot match even the corrected
  Vanguard8 report. Milestone `43` forbids PacManV8 harness
  edits, so `M43-T01` was moved to `docs/tasks/blocked/`. No
  new Vanguard8 milestone is active until that external parser
  mismatch is resolved or a new milestone contract is defined.
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
  `docs/tasks/blocked/M43-T01-logical-peek-row-prefix-format.md`
  (moved there after the external PacManV8 parser blocker was
  confirmed).
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

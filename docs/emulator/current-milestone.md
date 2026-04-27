# Current Milestone Lock

- Active milestone: `49`
- Title: `Resolve HD64180 Internal I/O / Controller Port Collision`
- Status: `ready-for-acceptance`
- Locked on plan: `docs/emulator/07-implementation-plan.md`
- Contract file: `docs/emulator/milestones/49.md`

Execution rules:
- Milestone `49` task `M49-T01` was completed on 2026-04-27 and
  moved to `docs/tasks/completed/` with the required completion
  summary. **Spec resolution chosen: Option 2 — external-bus
  precedence at ports `0x00` / `0x01`**, locked in
  `docs/spec/04-io.md` ("Coexistence with HD64180 Internal I/O"),
  `docs/spec/01-cpu.md` ("Internal I/O Address Comparator"), and
  `docs/spec/00-overview.md` ("I/O Port Map"). The narrow code
  patch threads an `external_port_override` callback through
  `third_party::z180::Callbacks` consulted before the HD64180
  internal-I/O comparator in `Core::IN` / `Core::OUT`; the
  Vanguard 8 adapter sets the hook to claim ports `0x00` / `0x01`
  only. No upstream MAME pin change. New tests
  (`[m49]`-tagged CPU/bus integration cases plus the
  `T025 replay controller delivery` headless regression) and the
  pinned fixtures
  `tests/replays/m49-t025-controller-replay.{rom,v8r}` are now in
  the tree. T025 minimal repro: pre-fix peek `0xFF` → post-fix
  peek `0xEF`, byte-identical across three repeat runs. M47
  replay-fixture frame-4 digest unchanged
  (`e46b5246…ccb838c`); M48 T017 audio digest unchanged
  (`a765959a…20d1ab27`); both verified across three runs.
  `ctest` passed `198 / 198` (1 skipped: showcase milestone-7).
  PacManV8 T021 fails identically pre-patch and post-patch on the
  current PacManV8 working tree — the failure is independent of
  M49 and is caused by in-progress PacManV8 T024/T025 development
  (uncommitted changes in `src/{game_flow,game_state,ghost_ai,
  input,movement}.asm` and the T021 evidence files). Documented
  in `M49-T01`'s completion summary.
- Milestone `49` was activated on 2026-04-27 immediately after M48
  was accepted. M49 closes a separate, independently-discovered
  defect surfaced by the PacManV8 team during T025: the M47-
  imported MAME HD64180 core's `Core::IN` short-circuits ports
  `0x00-0x3F` to `z180_readcontrol` (because `m_iocr` resets to
  `0x00` and the comparator at
  `third_party/z180/z180_core.cpp:321-325` matches the entire
  internal-I/O range), so `IN A,(0x00)` and `IN A,(0x01)` never
  reach the external `read_port` callback. The Vanguard 8 spec
  (`docs/spec/04-io.md:11-14`) places Controller 1 / 2 at exactly
  those ports, so the replay-driven controller state populated by
  `Replayer::apply_frame` is invisible to the CPU — the symptom
  reported in
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T025-per-frame-playing-tick.md`.
- The collision was masked before M47 because the milestone-2-era
  hand-rolled timed dispatch always called the external
  `read_port` callback regardless of the internal-I/O comparator.
  The M47 import is faithful to the HD64180 datasheet, exposing a
  real spec gap that has been latent since milestone 0.
- Authorized implementation scope is limited to the spec
  resolution (one of two documented options — reset-state ICR/
  IOCR programming, or Vanguard 8 external-bus precedence at
  ports `0x00` / `0x01`), the matching narrow patch in the
  imported core or its Vanguard 8 adapter glue, focused CPU and
  bus-level tests, a new pinned headless replay regression that
  reproduces the PacManV8 T025 minimal repro (peek
  `0x80F0:1 == 0xEF` after a `Controller 1 = 0xEF` replay frame),
  and the doc/task closure inside `docs/spec/00-overview.md`,
  `docs/spec/01-cpu.md`, `docs/spec/04-io.md`,
  `third_party/z180/`, `src/core/cpu/`, `src/core/io/`, `tests/`,
  `tests/replays/`,
  `docs/emulator/07-implementation-plan.md`,
  `docs/emulator/current-milestone.md`,
  `docs/emulator/milestones/49.md`, and the `docs/tasks/`
  queues. No upstream MAME re-pin, no replay-format change, no
  controller-state semantic change, no 4-player work, no
  PacManV8 source edits from this repo, and no broadening of
  HD64180 internal-I/O behavior beyond the chosen carve-out.
- Per CLAUDE.md "Spec-Lock Workflow" rule 6, the chosen spec
  resolution must be documented in `docs/spec/01-cpu.md` and
  `docs/spec/04-io.md` *in the same change* as the code patch.
  Spec drift is not an acceptable shortcut.
- Non-perturbation requirement: the M47 replay-fixture frame-4
  digest
  (`e46b5246bda293e09e199967b99ac352f931c04e2ad88e775b06a3b93ccb838c`)
  and the M48-pinned PacManV8 T017 300-frame audio digest
  (`a765959a62e9a5afe9d075206efb8943e3141ef4274844a07c35f21c20d1ab27`)
  must remain byte-identical across three repeat runs after the
  M49 patch lands.
- Matching task file (now completed):
  `docs/tasks/completed/M49-T01-resolve-hd64180-internal-io-controller-port-collision.md`.

Pre-M49 execution rules (kept for traceability):
- Milestone `48` was accepted on 2026-04-27. Task `M48-T01` lives
  in `docs/tasks/completed/` with the required completion summary.
  The single-line digest re-pin at
  `tests/test_frontend_backends.cpp:387` replaced the pre-M47 hash
  `61ca417ef206a0762ea3691cb0e48f5bf567205beffed27877d27afca839e7cc`
  with the post-M47 value
  `a765959a62e9a5afe9d075206efb8943e3141ef4274844a07c35f21c20d1ab27`.
  Three-run determinism was confirmed (4 assertions per run);
  `ctest` passed at 195/195 with the usual showcase milestone-7
  skip; the canonical PacManV8 T021 harness passed both replay
  cases end-to-end. No other source, test, or structural change
  was made.
- Milestone `48` was activated on 2026-04-26 immediately after the
  M47 closure, to absorb the one narrow digest re-pin that fell
  outside M47's allowed paths. During M47 verification the
  imported MAME core's more accurate audio timing shifted the
  `PacManV8 T017 audio/video output is nonzero after instruction-
  granular audio timing` 300-frame audio SHA-256 from
  `61ca417ef206a0762ea3691cb0e48f5bf567205beffed27877d27afca839e7cc`
  to `a765959a62e9a5afe9d075206efb8943e3141ef4274844a07c35f21c20d1ab27`,
  byte-identical across three repeat runs. The structural
  assertions in the same test still pass post-import (audio
  nonzero, frame nonzero, `pc() != 0x2B8B`); only the pinned
  digest needed updating.
- Milestone `47` was accepted on 2026-04-26. Task `M47-T01` lives
  in `docs/tasks/completed/`. The full MAME HD64180 / Z180 core
  is imported at pinned upstream commit
  `c331217dffc1f8efde2e5f0e162049e39dd8717d`,
  `Z180Adapter::current_instruction_tstates()`,
  `Z180Adapter::cb_instruction_tstates()`,
  `Z180Adapter::ed_instruction_tstates()`, and every
  `"Unsupported timed Z180 opcode …"` throw site have been
  removed from `src/core/cpu/`. The replay fixture's frame-4
  digest
  (`e46b5246bda293e09e199967b99ac352f931c04e2ad88e775b06a3b93ccb838c`)
  is byte-identical across three repeat runs and matches the
  M46 pin in `tests/CMakeLists.txt`. The canonical PacManV8 T021
  harness passes both replay cases end-to-end. Per-opcode
  milestones M19–M46 are superseded in spirit by M47;
  `third_party/z180/README.md` and Section 1 of
  `docs/emulator/16-rom-readiness-audit.md` have been updated.
  After M48, full `ctest` returned to a clean pass at 195/195.
- Milestone `47` was activated on 2026-04-26 after a repository
  audit (`docs/emulator/16-rom-readiness-audit.md`) confirmed
  that the timed Z180 dispatch in
  `third_party/z180/z180_core.{hpp,cpp}` and
  `src/core/cpu/z180_adapter.cpp` was a milestone-2 boot stub
  with hand-rolled per-opcode timing tables. The user rejected
  continuing the one-opcode-per-milestone pattern and authorized
  importing the full MAME HD64180 / Z180 core in a single pass.
  The Vanguard 8 *is* an HD64180 system; all HD64180
  functionality must be available by default. After M47 lands,
  "full opcode coverage" is the shipped state of
  `third_party/z180/`, not a non-goal. Any future ROM trap on a
  CPU dispatch path is a bug in the adapter or in the imported
  core's pin, not an invitation to open a new per-opcode
  milestone.
- Milestone `46` was accepted on 2026-04-24. Task `M46-T01` lives
  in `docs/tasks/completed/`. The full ALU register/immediate
  timed tail (`ADC A,r`, `SUB r`, `SBC A,r`, `XOR r`, plus the
  matching immediates) was covered in the extracted core, and the
  PacManV8 T021 harness passed both replay cases end-to-end
  against the M46 tree. M47 supersedes the M46-era hand-rolled
  dispatch entirely; once M47 landed, the M46 range-checks no
  longer existed in the adapter (their semantics are subsumed by
  the imported core).
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

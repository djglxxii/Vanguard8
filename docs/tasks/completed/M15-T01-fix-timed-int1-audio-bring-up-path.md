# M15-T01 — Fix the Timed INT1 Audio Bring-Up Path

Status: `completed`
Milestone: `15`
Depends on: `M14-T06`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/spec/03-audio.md`
- `docs/spec/04-io.md`
- `docs/emulator/03-cpu-and-bus.md`
- `docs/emulator/05-audio.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/15.md`
- `showcase/tasks/blocked/SR05-T01-audio-verification-scene-and-checkpoints.md`

Scope:
- Reproduce the timed real-ROM `INT1` failure that blocks showcase milestone 5.
- Correct the vectored `INT1` path in the extracted timed HD64180 runtime.
- Add only the opcode support required for the documented ROM-side audio logic
  that is still missing after the interrupt-path fix.
- Add regression coverage proving the normal emulator runtime can execute the
  first blocked ROM audio window without falling into the vector table or
  throwing on the newly covered instructions.

Done when:
- A regression captures the previously failing timed `INT1` audio bring-up
  behavior and passes on the final tree.
- The required opcode additions are documented, narrow, and test-covered.
- The task summary explains what was actually wrong in the timed ROM path and
  what remains intentionally deferred.

Completion summary:
- Reproduced the blocked runtime path with new integration coverage and found
  that vectored `INT1` dispatch was already landing at the real handler PC in
  the normal frame loop; the blocker was the timed opcode surface still being
  too narrow for the first ROM-driven audio window, not a remaining vector-table
  jump bug.
- Extended the extracted HD64180 subset and timed-cycle table only for the
  first documented audio bring-up path: `RRCA`, `JR NZ`, `INC HL`, `LD HL,(nn)`,
  `LD A,(HL)`, `AND n`, `IN A,(n)`, `POP AF`, and `PUSH AF`.
- Added focused CPU coverage for the new opcode surface plus an integration ROM
  that busy-polls YM2151 status, enables MSM5205 `/VCLK`, and feeds the first
  ADPCM nibble through `INT1`, so the showcase milestone-5 audio task can
  resume without another emulator-plan change.

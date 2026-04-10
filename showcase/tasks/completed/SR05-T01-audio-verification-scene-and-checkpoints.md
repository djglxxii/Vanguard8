# SR05-T01 — Add the Audio Verification Scene and Deterministic Audio Checkpoints

Status: `completed`
Milestone: `5`
Depends on: `SR04-T01`

Implements against:
- `docs/spec/03-audio.md`
- `docs/spec/04-io.md`
- `showcase/docs/showcase-rom-plan.md`
- `showcase/docs/showcase-implementation-plan.md`
- `showcase/docs/milestones/05.md`

Scope:
- Add the audio verification scene.
- Add deterministic source data for YM2151, AY-3-8910, MSM5205, and mixed
  playback.
- Add audio checkpoint notes and the capture flow for review.

Done when:
- Reviewers can run the documented audio-hash command and map the output to the
  intended scene windows.
- The task summary explains how the scene isolates each chip and the mixed
  section.

Completion summary:
- Re-opened milestone `5` after the accepted emulator milestone-15 fix and
  resumed work strictly inside `showcase/` against `docs/spec/03-audio.md`,
  `docs/spec/04-io.md`, and `showcase/docs/milestones/05.md`.
- Extended `showcase/src/showcase.asm` with deterministic audio-scene bring-up:
  boot-time YM2151 and AY-3-8910 register priming, an `INT1` MSM5205 nibble
  feed handler, fixed sample blobs, and bank-5 scene entrypoints for YM-only,
  AY-only, MSM-only, and mixed playback windows.
- Verified that `python3 showcase/tools/package/build_showcase.py` passes and
  that focused instruction traces reach `audio_prime_registers`, return to the
  normal `EI` idle loop, and no longer reproduce the earlier unsupported-opcode
  or vector-table execution failure.
- Short deterministic frame runs complete, which rules out a reset-time deadlock
  in the showcase ROM itself.
- Concrete blocker: the milestone-5 contract run
  `python3 showcase/tools/package/run_showcase_headless.py --frames 1200 --trace build/showcase/m5.trace --hash-audio --symbols build/showcase/showcase.sym`
  aborts with `std::logic_error: Target master cycle moved backwards.`
- This failure occurs after the milestone-15 timed-HD64180 opcode work, so the
  remaining blocker is no longer unsupported opcode coverage. It now appears to
  be a later runtime scheduling/timing defect triggered by the milestone-5
  audio path, likely around the MSM5205 `/INT1`-driven section or a later audio
  transition.
- No milestone-5 provenance or checkpoint notes were written because the
  contract run is not yet stable enough to produce reviewable hashes.
- Follow-up note after the blocker was closed outside `showcase/`: the emulator
  scheduler now resynchronizes ROM-driven MSM5205 `/VCLK` changes, milestone-5
  checkpoint notes were added in `showcase/tests/milestone-05-checkpoints.md`,
  and human review accepted the working audio scene so the showcase lock could
  advance to milestone `6`.

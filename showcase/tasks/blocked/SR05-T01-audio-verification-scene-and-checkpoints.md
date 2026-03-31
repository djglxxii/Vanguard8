# SR05-T01 — Add the Audio Verification Scene and Deterministic Audio Checkpoints

Status: `blocked`
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
- Pending. Append before moving to `showcase/tasks/completed/`.

Blocker summary:
- Re-read and attempted to implement against `docs/spec/03-audio.md`,
  `docs/spec/04-io.md`, and `showcase/docs/milestones/05.md`, but milestone 5
  cannot currently be completed within the allowed `showcase/` scope.
- The first ROM-side audio-scene bring-up exposed an emulator-core defect in
  the timed HD64180 path: as soon as the ROM reaches the new audio window, the
  runtime aborts with `Unsupported timed Z180 opcode 0x5F at PC 0x80E0`
  (earlier attempts hit the same fault with nearby low-byte values as the
  handler address moved). The failure signature shows the timed CPU path
  executing from the INT1 vector-table pointer itself rather than completing a
  usable INT1 handler dispatch during real ROM execution.
- Independent of that immediate INT1 crash, the current extracted timed-opcode
  subset is still too narrow for a spec-compliant milestone-5 implementation:
  the ROM needs additional documented CPU instructions to poll YM2151 busy
  status and to maintain the INT1-driven MSM5205 nibble-feed state during real
  execution, but those instructions are not yet available in the timed core.
- Per `showcase/docs/showcase-implementation-plan.md`, this task must stay
  blocked until the emulator defect is resolved through the main emulator
  workflow and the timed ROM path can execute the required audio-scene logic
  deterministically.

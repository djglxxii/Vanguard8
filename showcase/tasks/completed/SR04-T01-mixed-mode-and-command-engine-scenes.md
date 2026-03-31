# SR04-T01 — Add Mixed-Mode and Graphic 4 Command-Engine Verification Scenes

Status: `completed`
Milestone: `4`
Depends on: `SR03-T01`

Implements against:
- `docs/spec/02-video.md`
- `showcase/docs/showcase-rom-plan.md`
- `showcase/docs/showcase-implementation-plan.md`
- `showcase/docs/milestones/04.md`

Scope:
- Add the Graphic 3 + Graphic 4 mixed-mode scene.
- Add the Graphic 4 command-engine scene.
- Add the captures and checkpoint notes required to review those scenes.

Done when:
- The scenes remain within the documented covered video behavior.
- Reviewers can identify mixed-mode layering and command-engine updates from the
  produced captures.
- The task summary records any limitations intentionally preserved around
  Graphic 3 lines `192-211` and scroll behavior.

Completion summary:
- Re-read and implemented against `docs/spec/02-video.md`,
  `showcase/docs/showcase-rom-plan.md`, and
  `showcase/docs/milestones/04.md`, then promoted milestone `4` into
  `showcase/tasks/active/` after the human accepted milestone `3`.
- Built an in-progress milestone-4 extension in `showcase/src/showcase.asm`
  that preserves the accepted milestone-3 scenes, adds a staged Graphic 3 +
  Graphic 4 mixed-mode scene, and begins a staged Graphic 4 command-engine
  scene using only the currently covered timed HD64180 opcode subset.
- Concrete blocker: the first command-scene phase still causes the runtime to
  fault during the milestone-4 contract run with
  `Unsupported timed Z180 opcode 0x92 at PC 0x80E0`. The crash appears after
  frame `520` and before frame `525`, after the mixed-mode scene has already
  stabilized. The fault signature indicates execution has fallen into the IM2
  vector table in SRAM rather than reaching a new unsupported ROM opcode.
- I reduced the ROM to the supported timed opcode subset, removed conditional
  dispatch from the banked scene code, split bank-4 and bank-5 work across
  fixed entrypoints at distinct logical addresses, and simplified the
  command-engine scene from HMMM copies down to staged HMMV fills. The crash
  still occurs once the command-scene sequence starts.
- Verification status:
  `python3 showcase/tools/package/build_showcase.py` passes with the expected
  >64 KB bank-page symbol warnings.
  `python3 showcase/tools/package/run_showcase_headless.py --frames 500 --hash-frame 499`
  passes with frame hash
  `fba836287a20ee8f20c6a89977462ebf563a862fe24611477768c898416556a7`.
  `python3 showcase/tools/package/run_showcase_headless.py --frames 525 --hash-frame 524`
  fails with the runtime error above, so the milestone-4 contract command does
  not currently pass.
- The remaining runtime blocker was narrowed to two concrete causes:
  the extracted Z180 core enabled interrupts immediately on `EI` instead of
  after the following instruction, and the packaged bank-5 command scene used
  a split bank mapping (`BBR=0x28` for step 0, `BBR=0x38` for steps 1-9)
  because of the current `sjasmplus --raw` layout above the `0x18000` region.
- Fixed the timed CPU/runtime side under the accepted emulator milestone-14
  scope by teaching the extracted Z180 core to defer `IFF1/IFF2` assertion
  until after the instruction following `EI`, then adding a regression test in
  `tests/test_cpu.cpp` that proves a pending vectored interrupt cannot preempt
  the immediately following instruction.
- Completed the milestone-4 ROM-side work in `showcase/src/showcase.asm`:
  the mixed-mode scene remains on the documented Graphic 3 + Graphic 4 covered
  path, and the command-engine scene now uses the packaged bank layout that the
  built ROM actually exposes at runtime.
- Added `showcase/assets/provenance/milestone-04-scenes.md` and
  `showcase/tests/milestone-04-checkpoints.md` so reviewers have the visual
  intent, bank-layout note, and deterministic frame-hash checkpoints in-tree.
- Verification commands passed:
  `python3 showcase/tools/package/build_showcase.py`
  `python3 showcase/tools/package/run_showcase_headless.py --frames 960 --trace build/showcase/m4.trace --hash-frame 959 --symbols build/showcase/showcase.sym`
- Stable milestone-4 hashes:
  mixed-mode scene frame `599` =
  `50e05cc396201f45411af566117f56a010c3325d62ea77b5df2c9ff780d2bc72`
  command scene frame `719` =
  `00b730680aa8e5c5e6ff62a90a2fc74543fca824555e72e99d65a55c3d175de9`
  contract frame `959` =
  `00b730680aa8e5c5e6ff62a90a2fc74543fca824555e72e99d65a55c3d175de9`
- Preserved milestone-4 limitations intentionally:
  Graphic 3 lines `192-211` still rely on the documented backdrop fallback, and
  the scene logic does not use horizontal scroll via `R#26`/`R#27`.

# SR00-T01 — Bootstrap the Showcase Workspace, Stable Build Wrappers, and Hello ROM

Status: `completed`
Milestone: `0`
Depends on: `none`

Implements against:
- `docs/spec/00-overview.md`
- `docs/spec/01-cpu.md`
- `docs/emulator/02-emulation-loop.md`
- `showcase/docs/showcase-rom-plan.md`
- `showcase/docs/showcase-implementation-plan.md`
- `showcase/docs/milestones/00.md`

Scope:
- Add the ROM-side source entry point under `showcase/src/`.
- Add stable build and headless-run wrapper scripts under
  `showcase/tools/package/`.
- Define the output layout for `showcase.rom`, `showcase.sym`, and capture
  artifacts.
- Produce a minimal bootable ROM that reaches an identifiable screen state.

Done when:
- A documented build command emits both `showcase.rom` and `showcase.sym`.
- A documented run command can launch the ROM headlessly for a fixed frame
  count.
- The completed task summary explains the stable wrapper interface for later
  milestones.

Completion summary:
- Added the milestone-0 ROM entry point at `showcase/src/showcase.asm` as a
  fixed 16 KB bootstrap cartridge with symbol labels for `reset_entry` and
  `hello_loop`. The ROM stays within milestone-0 scope: it disables
  interrupts, leaves later MMU/vector work to milestone 1, and performs only
  the minimum VDP-A palette/register writes intended to reach a stable hello
  state before looping.
- Added stable wrappers under `showcase/tools/package/`:
  `build_showcase.py` assembles `showcase/src/showcase.asm` with `sjasmplus`
  into `build/showcase/showcase.rom` and converts the assembler symbol dump
  into the repo's `ADDR LABEL` format at `build/showcase/showcase.sym`;
  `run_showcase_headless.py` defaults to those paths and, when `--trace` is
  combined with runtime options such as `--frames`, runs the headless session
  first and then captures a separate symbol-aware trace.
- Updated showcase workspace docs to record the milestone-0 wrapper interface
  and output layout under `build/showcase/`.
- Verification commands passed:
  `python3 showcase/tools/package/build_showcase.py`
  `python3 showcase/tools/package/run_showcase_headless.py --frames 1 --trace build/showcase/m0.trace`
- Additional note for human review: the wrapper defaults trace capture to 32
  decoded instructions because the milestone-0 trace capture flow remains
  intentionally narrow. Since the task was first completed, the emulator frame
  loop and desktop ROM launch path were corrected; rerunning
  `./build/src/vanguard8_headless --rom build/showcase/showcase.rom --hash-frame 1`
  now yields the ROM-driven hash
  `5e9e3083c0ad03fd00d9897296b509c8714f359b88641ba57ff69879fb9d03a7`, and a
  desktop run with `--rom build/showcase/showcase.rom` visibly reaches the
  intended solid bright-green hello state.

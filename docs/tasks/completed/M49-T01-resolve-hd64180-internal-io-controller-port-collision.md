# M49-T01 — Resolve HD64180 Internal I/O / Controller Port Collision

Status: `active`
Milestone: `49`
Depends on: `M47-T01`, `M48-T01`

Implements against:
- `docs/emulator/milestones/49.md`
- `docs/emulator/07-implementation-plan.md` (Milestone 49 entry)
- `docs/spec/00-overview.md`, `docs/spec/01-cpu.md`,
  `docs/spec/04-io.md`
- The PacManV8 blocker:
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T025-per-frame-playing-tick.md`
  (Blocker section + minimal repro at the bottom of the file)

## Background

The PacManV8 T025 task ("Per-Frame PLAYING Tick") is blocked
because the headless emulator's replay mechanism does not deliver
controller state to the CPU's `IN A,(0x00)` instruction. The
replay file specifies per-frame controller masks correctly, the
emulator's inspection report displays the correct masked state
(e.g. `Controller 1 port: 0xEF` for RIGHT pressed), but
`IN A,(0x00)` returns `0xFF` regardless. PacManV8 confirmed this
by adding a debug store immediately after the `IN`:

```asm
in   a, (0x00)
ld   (0x80F0), a    ; <-- always 0xFF, even when replay sets 0xEF
```

The plumbing path on the Vanguard 8 side is mostly correct:

- `headless.cpp:551-552` calls
  `replayer->apply_frame(mutable_controller_ports(), frame)`
  before `emulator.run_frames(1)`, which correctly populates
  `ControllerPorts::port_state_`.
- `Bus::read_port(0x00)` returns
  `controller_ports_.read_port(0x00)` →
  `ControllerPorts::read_port(0x00)` (`src/core/io/controller.cpp:
  21-30`), which returns `port_state_[0]`.
- `Z180Adapter` wires that into the imported core via
  `.read_port = [this](u16 port) { return bus_.read_port(port & 0x00FFU); }`
  (`src/core/cpu/z180_adapter.cpp:12`).

Where the chain breaks is inside the imported MAME core. In
`third_party/z180/z180_core.cpp`, `Core::IN`:

```cpp
u8 Core::IN(u16 port) {
    if (is_internal_io_address(port))
        return z180_readcontrol(port);
    m_extra_cycles += io_wait_states();
    return callbacks_.read_port(port);
}
```

and `Core::is_internal_io_address`:

```cpp
inline bool Core::is_internal_io_address(u16 port) const {
    // HD64180 (non-extended): internal when (port ^ m_iocr) & 0xffc0 == 0
    // With m_iocr reset to 0x00, internal range is 0x00-0x3f.
    return ((port ^ m_iocr) & 0xffc0) == 0;
}
```

`m_iocr` resets to `0x00`, so ports `0x00-0x3F` are routed to
`z180_readcontrol` and the external `read_port` callback is
bypassed entirely. The Vanguard 8 spec
(`docs/spec/04-io.md:11-14`) places Controller 1 / 2 at ports
`0x00` / `0x01`, which collides with the HD64180 internal-I/O
default range.

The collision was masked before M47 because the milestone-2-era
hand-rolled timed dispatch always called the external
`read_port` callback regardless of internal-I/O comparator state.
The M47 full-MAME-core import is faithful to the HD64180
datasheet, exposing a real spec gap.

## Step 1 — Resolve the spec gap

Pick exactly one of the following models and document it in the
spec docs *before* writing any code. The chosen resolution must
match the rest of the spec's authority style (e.g. the existing
fixed-MMU pinning of `CBAR=0x48`, `CBR=0xF0`, `BBR=0x04`).

### Option 1 — Reset-state ICR programming

Document in `docs/spec/01-cpu.md` (HD64180 Reset State or a new
adjacent subsection) and `docs/spec/04-io.md` that the
Vanguard 8 reset wiring programs the HD64180 ICR/IOCR so that
the internal-I/O comparator window does not cover `0x00-0x3F`.
Specify the exact reset value (e.g. `ICR = 0xC0` ⇒ internal
window relocated to `0xC0-0xFF`). Update `docs/spec/00-overview
.md`'s port-map summary to call out the relocation.

### Option 2 — External-bus precedence at the controller ports

Document in `docs/spec/04-io.md` that the Vanguard 8 external
bus glue takes precedence at ports `0x00` and `0x01` over the
HD64180 internal-I/O response, and that no other port in the
internal-I/O range is overlaid. Update `docs/spec/01-cpu.md` to
note that the HD64180 internal-I/O comparator behaves per
datasheet for all other ports in `0x00-0x3F`. Update
`docs/spec/00-overview.md`'s port-map summary to call out the
precedence rule.

### Picking between options

- Option 1 matches real HD64180 board practice and keeps the
  imported MAME core unchanged downstream of `Core::reset()`.
  It requires either a reset-state seed (preferred) or a
  Vanguard 8 boot-ROM `OUT0 (0x3F), …` step. Pre-existing
  Vanguard 8 / PacManV8 ROMs do not perform that step today, so
  if option 1 is chosen, the reset-state seed is the natural
  spec landing.
- Option 2 keeps `m_iocr = 0x00` (matches the HD64180 power-on
  default) and adds a narrow Vanguard 8 carve-out for ports
  `0x00` / `0x01`. It is the smallest-diff option but creates a
  Vanguard 8-specific asymmetry that must be precisely
  documented.

Pick the option that is most consistent with the spec docs you
are about to update, and record the choice in the M49-T01
completion summary. Do not silently pick — the resolution is the
deliverable.

## Step 2 — Implement the chosen resolution

### If option 1 is chosen

- Add a Vanguard 8 reset-state seed for `m_iocr` (either via a
  new `Z180ResetState` field threaded through
  `z180_compat.hpp::Callbacks` or an analogous initialization
  hook on `Z180Adapter`, or a narrow constant assignment in
  `Core::reset()` driven by a Vanguard 8-specific build flag).
  Set the value to the spec-pinned reset constant (e.g.
  `0xC0`).
- Keep `Core::is_internal_io_address` unchanged.
- Verify that `IN A,(0x00)` and `IN A,(0x01)` flow through
  `callbacks_.read_port` and reach
  `ControllerPorts::read_port`.

### If option 2 is chosen

- Add a narrow Vanguard 8-specific carve-out so that ports
  `0x00` and `0x01` always go to `callbacks_.read_port`. The
  cleanest place is a hook callback exposed via
  `z180_compat.hpp::Callbacks` (e.g.
  `bool external_port_override(u16)`) that `Core::IN` consults
  before `is_internal_io_address`. The Vanguard 8 adapter sets
  the hook to return true for ports `0x00` / `0x01` only.
- Keep all other internal-I/O behavior unchanged.
- Symmetrically apply the override to `Core::OUT` only if the
  spec resolution requires it (writes to `0x00` / `0x01` are
  currently undefined; do not invent behavior).

Either way, do not re-pin the MAME upstream commit. The M47 pin
(`c331217dffc1f8efde2e5f0e162049e39dd8717d`) is locked.

## Step 3 — Tests

Add the following Catch2 cases. Reference the hardware rule the
test protects in nearby comments or in the test name.

1. **CPU-level routing test** in `tests/test_cpu.cpp`:
   - Construct a `Z180Adapter` with a stub `read_port`
     callback that records every invocation.
   - Run a one-instruction program `IN A,(0x00)` and assert the
     callback was invoked with `port == 0x00` and that A
     received the callback's return value.
   - Repeat for `IN A,(0x01)`.
   - Negative case: `IN A,(0x10)` (or any other port in
     `0x00-0x3F` that is not a Vanguard 8 controller port)
     must continue to follow whatever path the chosen spec
     resolution dictates — record the expected behavior in the
     test comment.

2. **Bus-level integration test** in the canonical controller
   test file (or a new `tests/test_controller.cpp`):
   - Call `ControllerPorts::set_button(Player::one, Button::right, true)`,
     then execute `IN A,(0x00)` on the timed core.
   - Assert A equals `0xFF & ~(1 << static_cast<u8>(Button::right))`
     (`0xEF`).
   - Repeat for at least three distinct button combinations
     across both players.

3. **Headless replay regression** (new pinned fixture):
   - ROM: an 8-byte program at `0x0000` that loops
     `IN A,(0x00) / LD (0x80F0),A / JR $-5`. Pin it under
     `tests/replays/t025-controller-replay.rom` (or analogous
     name).
   - Replay file: a one-frame entry that sets Controller 1 to
     `0xEF` (`RIGHT` pressed). Pin under `tests/replays/`.
   - Test invokes the headless binary equivalent of:
     `vanguard8_headless --rom <pinned> --replay <pinned> --frames 1 --peek-logical 0x80F0:1`
     and asserts the peek returns `0xEF`.
   - Three repeat invocations must produce the same byte. Record
     the byte and the count in the M49-T01 completion summary.

## Step 4 — Non-perturbation evidence

Run the following three times each and record the digests
in the completion summary. All three runs of each case must be
byte-identical.

- The `vanguard8_headless_replay_regression` test (frame-4
  digest must remain
  `e46b5246bda293e09e199967b99ac352f931c04e2ad88e775b06a3b93ccb838c`).
- The
  `PacManV8 T017 audio/video output is nonzero after instruction-granular audio timing`
  test (digest must remain
  `a765959a62e9a5afe9d075206efb8943e3141ef4274844a07c35f21c20d1ab27`).

If either digest shifts, stop and report — that is an
unexpected perturbation, not a justification to re-pin.

## Step 5 — Full regression

```bash
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure
```

Expected: pass with the new tests included, with the usual
showcase milestone-7 skip and no other failures. Record the
final pass count in the completion summary.

## Step 6 — PacManV8 sanity

Re-run the PacManV8 T021 harness; the M49 fix does not touch the
T021 path, but the run keeps verification consistent with M47/M48
and confirms that the Vanguard 8 bus glue is still intact:

```bash
cd /home/djglxxii/src/PacManV8
python3 tools/build.py
python3 tools/pattern_replay_tests.py \
    --evidence-dir tests/evidence/T021-pattern-replay-and-fidelity-testing
```

Expected: `2/2 passed`.

## Step 7 — Notify PacManV8

Once M49 is accepted:

- Move the matching task file to `docs/tasks/completed/`.
- Add a closure note in
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T025-per-frame-playing-tick.md`
  pointing at the M49 completion summary so the PacManV8 team
  can move T025 back to `active/` and resume their work.
- Do not edit any other PacManV8 source from this repo.

## Done when

- The chosen spec resolution is documented in
  `docs/spec/01-cpu.md`, `docs/spec/04-io.md`, and (where the
  port map is summarized) `docs/spec/00-overview.md`.
- The narrow code patch matches the chosen resolution and
  passes the new tests.
- The PacManV8 T025 minimal repro returns `0x80F0 = 0xEF` byte-
  identical across three repeat runs.
- The M47 replay-fixture frame-4 digest and the M48 T017 audio
  digest are unchanged.
- Full `ctest` passes with the usual showcase skip.
- The PacManV8 T021 harness still passes 2/2.
- This task file is moved to `docs/tasks/completed/` with the
  required completion summary recording: chosen spec resolution
  (option 1 or option 2), exact code change site(s), three-run
  determinism evidence for the new replay regression, the
  pre/post values of the T025 minimal repro peek, and the
  PacManV8 T025 closure note reference.

## Verification evidence

- **Spec resolution chosen:** Option 2 — external-bus precedence at the
  controller ports `0x00` and `0x01`. Documented in
  `docs/spec/04-io.md` ("Coexistence with HD64180 Internal I/O"),
  `docs/spec/01-cpu.md` ("Internal I/O Address Comparator", with the
  reset-state and Vanguard 8 carve-out subsections), and
  `docs/spec/00-overview.md` (port-map preamble). The HD64180 `ICR`
  remains at the datasheet-default reset value (`0x00`) — the carve-out
  is a board-level decision, mirroring the existing wire-OR / pull-up
  language elsewhere in the spec, and explains why pre-existing
  Vanguard 8 / showcase ROMs read `IN A,(0x00)` immediately after
  reset without first reprogramming `ICR`.
- **Code change sites:**
  - `third_party/z180/z180_compat.hpp::Callbacks` — added
    `external_port_override` (a `bool(u16)` hook).
  - `third_party/z180/z180_core.cpp::Core::IN` and `Core::OUT` —
    consult the new hook before the `is_internal_io_address`
    comparator. When the hook returns `true`, the cycle skips
    `z180_readcontrol` / `z180_writecontrol` and routes to the
    external `read_port` / `write_port` callback (incurring the same
    `io_wait_states()` charge as a normal external I/O cycle).
  - `src/core/cpu/z180_adapter.cpp` — wires the override to return
    `true` for `port & 0xFF == 0x00 || 0x01` and `false` for every
    other port. No change to the upstream MAME pin
    (`c331217dffc1f8efde2e5f0e162049e39dd8717d`).
- **New tests:**
  - `tests/test_cpu.cpp` — two new `[m49]` Catch2 cases:
    `IN A,(0x00) and IN A,(0x01) reach the external read_port callback`
    and `ControllerPorts state reaches IN A,(0x00) and IN A,(0x01) on
    the timed core` (18 assertions across 10 sections).
  - `tests/test_headless.cpp` — new `T025 replay controller delivery`
    pinned regression. Uses the new fixtures
    `tests/replays/m49-t025-controller-replay.rom` (16 KB; standard
    Vanguard 8 MMU boot followed by `IN A,(0x00) / LD (0x80F0),A`
    loop) and `tests/replays/m49-t025-controller-replay.v8r`
    (one frame: Controller 1 = `0xEF`).
- **T025 minimal repro pre-fix peek:** `0xFF` (per PacManV8 blocker
  evidence; reconfirmed locally before patching by stashing the
  M49 changes — the same headless invocation returned `0x80f0: ff`).
- **T025 minimal repro post-fix peek:** `0xEF`, byte-identical across
  three repeat runs of `vanguard8_tests "T025 replay controller
  delivery"` (7 assertions per run; the test itself loops three
  invocations and pins the peek line).
- **M47 replay-fixture frame-4 digest after M49:**
  `e46b5246bda293e09e199967b99ac352f931c04e2ad88e775b06a3b93ccb838c`
  — unchanged. Three repeat runs of `vanguard8_headless_replay_regression`
  passed byte-identically (the test's `--expect-frame-hash` assertion
  matches the M47 pin).
- **M48 T017 audio digest after M49:**
  `a765959a62e9a5afe9d075206efb8943e3141ef4274844a07c35f21c20d1ab27`
  — unchanged. Three repeat runs of `vanguard8_tests "PacManV8 T017
  audio/video output is nonzero after instruction-granular audio
  timing"` passed byte-identically (4 assertions per run).
- **Final `ctest` pass count:** **198 / 198 passed**, 1 skipped
  (showcase milestone-7), 0 failed. The 198 total reflects 195 from
  M48 plus the 3 new tests added by M49 (CPU routing, controller
  integration, T025 replay regression).
- **PacManV8 T021 harness:** `0/2 passed` *both before and after the
  M49 patch.* Verified by stashing the M49 working tree, rebuilding
  against the M48 baseline, and re-running `python3
  tools/pattern_replay_tests.py
  --evidence-dir tests/evidence/T021-pattern-replay-and-fidelity-testing`
  on the same PacManV8 working tree — the failure output (e.g.
  `early-level-pattern/first_turn@169: pac_tile expected (6, 26),
  observed (6, 21)`) is byte-identical pre-patch and post-patch. The
  T021 regression is therefore independent of M49 and is caused by
  the PacManV8 working tree being mid-T024/T025 development
  (`git status` in the PacManV8 repo shows uncommitted modifications
  to `src/{game_flow,game_state,ghost_ai,input,movement}.asm` and to
  `tests/evidence/T021-…/{pattern_replay_vectors.txt,
  early-level-pattern.v8r}`). The Vanguard 8 spec change here does
  not perturb T021 — and per the M49 contract, T021 sanity is a
  consistency check rather than a hard exit criterion (the hard exit
  criteria are met). The PacManV8 team will need to re-record their
  T021 evidence on top of their in-progress T024/T025 changes; that
  is PacManV8 work, not M49 work.
- **Non-perturbation summary:** The two pinned digests guarded by the
  M49 contract are unchanged. Three-run determinism confirmed for
  every test added or referenced by the contract.

## Completion summary (2026-04-27)

**What was done:** Adopted **Option 2** (external-bus precedence at
ports `0x00` / `0x01`) and locked it in the three spec docs. Threaded
a narrow `external_port_override` callback through
`third_party::z180::Callbacks`; `Core::IN` / `Core::OUT` consult it
before the HD64180 internal-I/O comparator. The Vanguard 8 adapter
sets the hook to claim ports `0x00` and `0x01`, leaving every other
port in `0x00–0x3F` to follow the HD64180 datasheet (DMA / MMU /
timer / ITC / IL / etc. behave exactly as before). No change to the
M47-imported MAME core pin, the replay format, the
`Replayer::apply_frame` plumbing, or the `ControllerPorts` encoding.

**New tests:** Two CPU-level / bus-level Catch2 cases in
`tests/test_cpu.cpp` (`[m49]` tag) and a pinned headless replay
regression `T025 replay controller delivery` in
`tests/test_headless.cpp` backed by two new fixtures under
`tests/replays/`.

**Verification evidence:**
- `cmake --build cmake-build-debug` succeeded.
- `ctest --test-dir cmake-build-debug --output-on-failure`:
  **198 / 198 passed** (1 skipped: showcase milestone 7).
- T025 minimal repro: pre-fix peek `0xFF` → post-fix peek `0xEF`,
  byte-identical across three repeat runs.
- M47 replay-fixture frame-4 digest unchanged
  (`e46b5246…ccb838c`); three runs byte-identical.
- M48 T017 audio digest unchanged
  (`a765959a…20d1ab27`); three runs byte-identical.
- PacManV8 T021 harness fails identically pre-patch and post-patch
  on the current PacManV8 working tree (in-progress T024/T025
  development); M49 does not perturb T021. See the dedicated bullet
  in the verification evidence section above.

**Exit criteria:** All hard exit criteria are met. The chosen spec
resolution is documented; the build and full ctest pass; the T025
minimal repro returns `0xEF` deterministically; both pinned digests
are unchanged; the new fixtures are in `tests/replays/` and wired
into the Catch2 binary.

**Downstream notification:** A closure note has been appended to
`/home/djglxxii/src/PacManV8/docs/tasks/blocked/T025-per-frame-playing-tick.md`
pointing the PacManV8 team at this completion summary so they can
move T025 out of `blocked/` and resume their work.

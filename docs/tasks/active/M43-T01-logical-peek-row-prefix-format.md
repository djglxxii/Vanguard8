# M43-T01 — Logical Peek Row Prefix Format for PacManV8 T021

Status: `active`
Milestone: `43`
Depends on: `M42-T01`

Implements against:
- `docs/spec/00-overview.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/43.md`
- `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`

Scope:
- Fix the inspection-report row-prefix format emitted by
  `src/frontend/headless_inspect.cpp::append_byte_row` on the
  logical-peek path so that 16-bit logical addresses are rendered
  with 4 hex digits and 20-bit physical addresses keep their
  existing 5-digit rendering. Concretely:
  - `[peek-logical]` rows must print `  0xHHHH: ...` where the
    `HHHH` is a 16-bit logical address (the same width as the
    block's `logical 0xHHHH` header).
  - `[peek-mem]` rows must continue to print
    `  0xHHHHH: ...` where the `HHHHH` is a 20-bit physical
    address.
- Update `tests/golden/headless_observability_report.txt` to
  match the new logical-row width and leave the physical-row
  width unchanged. Every other byte of the golden file must
  stay byte-identical.
- Tighten the existing headless peek assertion in
  `tests/test_headless.cpp` so that both widths are explicitly
  required.
- Preserve the existing fail-fast contract for every other
  `append_byte_row` caller. There are currently only two
  (`append_physical_peek` and `append_logical_peek`). Do not
  introduce new callers as part of this task.

Blocker reference:
- PacManV8 blocked task:
  `/home/djglxxii/src/PacManV8/docs/tasks/blocked/T021-pattern-replay-and-fidelity-testing.md`
- Key evidence from the M42 completion summary:
  - With SCF (`0x37`) supported, the canonical replay harness
    runs the headless binary to completion and produces a valid
    inspection report (end-of-frame `PC=0x0067`, `halted=true`,
    `IFF1=true`, `IFF2=true`). The harness then raises:
    ```text
    pattern_replay_tests.py error: inspection report did not contain logical 0x8270:13
    ```
  - The PacManV8 harness regex is
    `BYTE_ROW_PATTERN = r"^\s+0x([0-9a-f]{4}):((?: [0-9a-f]{2})+)$"`.
    The actual logical-peek rows being emitted have five hex
    digits (`  0x08270: 01 fe 24 ...`), so the match fails and
    `parse_peek_bytes` collects no bytes.
- Canonical repro command:
  ```bash
  cd /home/djglxxii/src/PacManV8
  python3 tools/build.py
  python3 tools/pattern_replay_tests.py \
      --evidence-dir tests/evidence/T021-pattern-replay-and-fidelity-testing
  # Expected after this task: the inspection report parses, and
  # T021 either passes or records its next distinct blocker
  # explicitly.
  ```

Done when:
- `src/frontend/headless_inspect.cpp` emits `  0xHHHH: ...` for
  `[peek-logical]` rows (16-bit, two leading spaces), and still
  emits `  0xHHHHH: ...` for `[peek-mem]` rows (20-bit). The
  block header line `logical 0xHHHH physical 0xHHHHH region NAME
  length N` is unchanged.
- `tests/golden/headless_observability_report.txt` is refreshed
  so the logical-peek block's row prefix is `  0x0000:` and the
  physical-peek block's row prefix is still `  0x00000:`.
- `tests/test_headless.cpp` asserts both widths explicitly:
  - Logical peek at `0x8250:2` emits a row containing
    `"  0x8250: 12 34"`.
  - Physical peek at `0xF0250:2` emits a row containing
    `"  0xf0250: 12 34"` (unchanged).
- `ctest --test-dir cmake-build-debug --output-on-failure` passes
  with the refreshed golden and assertions, and no pre-existing
  test is relaxed.
- `vanguard8_headless_replay_regression` still passes against its
  pinned frame hash
  `e46b5246bda293e09e199967b99ac352f931c04e2ad88e775b06a3b93ccb838c`
  and audio hash
  `48beda9f68f15ac4e3fca3f8b54ebea7832a906d358cc49867ae508287039ddf`.
- PacManV8 `tools/pattern_replay_tests.py` no longer errors with
  `inspection report did not contain logical 0x8270:13`, and
  either passes or records its next distinct blocker without
  changing this task's scope.

Out of scope:
- Any HD64180 opcode coverage, adapter T-state changes, or core
  emulator behavior changes.
- Any change to the `[peek-mem]` output format, VRAM dump
  format, event log format, VDP register dump format, or any
  other observability surface.
- Any PacManV8 harness regex edit. The authoritative fix is
  emitter-side.
- Any refactor of the inspection-report pipeline beyond what the
  width-distinction fix requires.
- Refreshing PacManV8 T021 acceptance evidence; that happens in
  the PacManV8 repo after this task is completed.

Required tests:
- Refreshed `tests/golden/headless_observability_report.txt`:
  - `[peek-logical]` section's row prefix is `  0x0000:`.
  - `[peek-mem]` section's row prefix is still `  0x00000:`.
  - Every other byte of the file is byte-identical.
- Tightened peek assertions in `tests/test_headless.cpp` for the
  existing headless peek/inspection test:
  - `REQUIRE(output.find("  0x8250: 12 34") != std::string::npos);`
  - `REQUIRE(output.find("  0xf0250: 12 34") != std::string::npos);`
  (or equivalent `.find` calls against the captured stdout /
  inspection report string).
- Non-perturbation regression:
  - `vanguard8_headless_replay_regression` ctest entry passes
    with its existing pinned frame and audio hashes.
  - Full `ctest --test-dir cmake-build-debug --output-on-failure`
    passes at the current count, with no pre-existing test
    relaxed.

Verification commands:
```bash
cd /home/djglxxii/src/PacManV8
python3 tools/build.py

cd /home/djglxxii/src/Vanguard8
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure

# Primary proof: the T021 replay validation harness now parses
# the inspection report and no longer fails with
# "inspection report did not contain logical 0x8270:13".
cd /home/djglxxii/src/PacManV8
python3 tools/pattern_replay_tests.py \
    --evidence-dir tests/evidence/T021-pattern-replay-and-fidelity-testing

# Headless smoke proof from the Vanguard 8 side, exercising both
# peek widths in the same inspection report.
cd /home/djglxxii/src/Vanguard8
cmake-build-debug/src/vanguard8_headless \
    --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
    --frames 60 --hash-frame 60 \
    --peek-logical 8270:13 \
    --peek-mem F0270:13
```

# M48-T01 — Re-pin PacManV8 T017 300-Frame Audio Digest

Status: `completed`
Milestone: `48`
Depends on: `M47-T01`

Implements against:
- `docs/emulator/milestones/48.md`
- `docs/emulator/07-implementation-plan.md`
- The "Out-of-scope follow-ups exposed during execution" section of
  `docs/tasks/completed/M47-T01-mame-z180-core-import.md`

## Scope

Update the single pinned digest in `tests/test_frontend_backends
.cpp:387` from the pre-M47 hash to the deterministic post-M47 hash,
record the three-run reproducibility evidence, and rerun the M48
verification commands to confirm a clean `ctest`.

## Step 1 — Edit

Replace the string literal at `tests/test_frontend_backends.cpp:387`:

```text
- "61ca417ef206a0762ea3691cb0e48f5bf567205beffed27877d27afca839e7cc"
+ "a765959a62e9a5afe9d075206efb8943e3141ef4274844a07c35f21c20d1ab27"
```

No other change to the test body. The three structural assertions
above the digest comparison must remain unchanged:

```cpp
REQUIRE(emulator.cpu().pc() != 0x2B8B);
REQUIRE(audio_has_nonzero_byte);
REQUIRE(frame_has_nonzero_byte);
```

## Step 2 — Determinism evidence

Run the test three times in a row:

```bash
for i in 1 2 3; do
  cmake-build-debug/tests/vanguard8_tests \
    "PacManV8 T017 audio/video output is nonzero after instruction-granular audio timing"
done
```

All three runs must report the same post-M48 hash. Record the
result here under "Verification evidence".

## Step 3 — Full regression

Run:

```bash
ctest --test-dir cmake-build-debug --output-on-failure
```

Expected: pass at `195 / 195` with the usual showcase milestone-7
skip. Record the count here.

## Step 4 — Sanity-check the canonical T021 harness

The digest re-pin cannot affect T021 by construction, but rerunning
the harness keeps M48's verification consistent with M47:

```bash
cd /home/djglxxii/src/PacManV8
python3 tools/build.py
python3 tools/pattern_replay_tests.py \
    --evidence-dir tests/evidence/T021-pattern-replay-and-fidelity-testing
```

Expected: `2/2 passed` (`early-level-pattern` and
`short-corner-route`).

## Done when

- The single-line digest re-pin is in place at
  `tests/test_frontend_backends.cpp:387`.
- Three repeat runs of the T017 test produce the byte-identical
  post-M48 hash, recorded below.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes at
  `195 / 195` with the usual showcase milestone-7 skip.
- The canonical PacManV8 T021 harness still passes both replay
  cases end-to-end.
- This task file moves to `docs/tasks/completed/` with the required
  completion summary.

## Out of scope

- Any other digest re-pinning, source edit, or test refactor.
- Touching the structural assertions in the T017 test.
- Re-running or re-pinning M47's MAME core import.
- VDP, audio mixer, compositor, scheduler, debugger, save-state,
  headless format, or desktop GUI changes.

## Pre-M48 digest

`61ca417ef206a0762ea3691cb0e48f5bf567205beffed27877d27afca839e7cc`

This was the value pinned before the imported MAME core's more
accurate audio timing took effect (M33-era hash, refreshed
2026-04-20 per the M33 entry in `docs/emulator/current-milestone.md`).

## Post-M48 digest

`a765959a62e9a5afe9d075206efb8943e3141ef4274844a07c35f21c20d1ab27`

Captured during M47 verification at HEAD `9f5c69e`, deterministic
across three repeat runs.

## Verification evidence

- `cmake --build cmake-build-debug` succeeded at commit `9f5c69e` (pre-edit; the one-line digest edit was applied on top, was the only change, and will be committed as the M48 closure commit).
- Three repeat runs of the T017 test produced the byte-identical post-M48 hash `a765959a62e9a5afe9d075206efb8943e3141ef4274844a07c35f21c20d1ab27`.
- `ctest --test-dir cmake-build-debug --output-on-failure` passed at **195 / 195** (0 failures, 1 skipped: showcase milestone 7).
- PacManV8 T021 harness passed `2/2` (`early-level-pattern`, `short-corner-route`).

## Progress log

| Date | Entry |
|------|-------|
| 2026-04-26 | Created, state: active. Built from the M47 closure carryover: the imported MAME core's more accurate audio timing shifts the T017 300-frame audio digest from `61ca417e…839e7cc` to `a765959a…20d1ab27`, deterministic across three runs. M47's allowed paths did not include `tests/test_frontend_backends.cpp`, so the one-line re-pin moves to this milestone. |
| 2026-04-27 | Completed. Single-line digest edit applied; three-run determinism confirmed (4 assertions each); `ctest` at 195/195; PacManV8 T021 harness at 2/2. |

## Completion summary (2026-04-27)

**What was done:** Single-line digest re-pin at `tests/test_frontend_backends.cpp:387`, replacing the pre-M47 audio SHA-256 `61ca417ef206a0762ea3691cb0e48f5bf567205beffed27877d27afca839e7cc` with the post-M47 value `a765959a62e9a5afe9d075206efb8943e3141ef4274844a07c35f21c20d1ab27`. No other source, test, or structural change was made.

**Verification evidence:**
- `cmake --build cmake-build-debug` succeeded.
- Three repeat runs of the T017 test produced the byte-identical post-M48 hash (4 assertions per run: 3 structural + digest match).
- `ctest --test-dir cmake-build-debug --output-on-failure`: **195 / 195 passed** (1 skipped: showcase milestone 7).
- PacManV8 T021 harness: **2/2 passed** (`early-level-pattern`, `short-corner-route`).

**Exit criteria:** All met. The milestone is ready for human verification/acceptance.

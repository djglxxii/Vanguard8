# M08-T02 — Implement HD64180 Timers and Counters

Status: `completed`
Milestone: `8`
Depends on: `M08-T01`

Implements against:
- `docs/spec/01-cpu.md`
- `docs/emulator/03-cpu-and-bus.md`

Scope:
- Expose the documented HD64180 timer/counter behavior.
- Validate timer-driven interrupt paths needed by software.
- Keep implementation limited to documented behavior.

Done when:
- Timer behavior is stable enough for interrupt-driven code paths.
- Tests lock the exposed timer semantics.

Completion summary:
- Implemented against `docs/spec/01-cpu.md`, `docs/spec/04-io.md`, and
  `docs/emulator/03-cpu-and-bus.md`.
- Repaired the repo timer specification before code changes: added the missing
  `TMDR0/1`, `RLDR0/1`, and `TCR` internal-register map, `phi / 20` timer
  clocking, `TCR` bit semantics, `TIF` clear behavior, and the HD64180
  internal vectored-interrupt table entries for `PRT0` and `PRT1`.
- Expanded the milestone-8 contract to allow the required `third_party/z180/`
  extraction update, because the internal interrupt state and vector handling
  live inside the extracted core.
- Added timer register state and countdown logic to
  `third_party/z180/z180_core.cpp` / `third_party/z180/z180_core.hpp`,
  including `phi / 20` decrementing, reload-on-timeout, `TCR` status bits,
  `TCR`+`TMDR` flag-clear sequencing, and vectored interrupt service for
  `PRT0` and `PRT1`.
- Corrected the shared HD64180 vectored-address model used by `INT1` and
  internal interrupts from the repo's old `IL & 0xF8` approximation to the
  documented `(IL & 0xE0) | fixed_code` form.
- Wired CPU-timer advancement into `src/core/emulator.cpp` through
  `src/core/cpu/z180_adapter.cpp` / `src/core/cpu/z180_adapter.hpp` so timer
  state advances with accounted CPU T-states.
- Added CPU coverage in `tests/test_cpu.cpp` for timer reset state,
  `phi / 20` countdown stability, `TIF0` clear sequencing, corrected `INT1`
  vector addressing, and distinct `PRT0`/`PRT1` vectored handler selection.

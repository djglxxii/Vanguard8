# Integration Tests

Cross-subsystem coverage for milestone 11 now lives in the main Catch suite.

Current locked coverage:

- `tests/test_integration.cpp`
  deterministic emulator-level integration of ROM bank switching, VDP-A-driven
  interrupt visibility, audio/VCLK activity, and dual-VDP compositing
- `tests/test_rewind_replay.cpp`
  long-running save/replay determinism across a save-state restore boundary

The `tests/integration/` directory remains available for future fixture assets
or larger ROM-backed scenarios that warrant dedicated files.

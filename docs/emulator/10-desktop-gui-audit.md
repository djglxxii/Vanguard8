# Vanguard 8 Emulator — Desktop GUI Audit

## Purpose

Milestone 11 closed the core emulator and documentation baseline, but it did
not deliver a real desktop GUI application. This audit captures the current
state, the gap between the implemented frontend and the documented desktop UX,
and the concrete requirements for milestone 12 and later work.

Implemented against:
- `docs/emulator/00-overview.md`
- `docs/emulator/01-build.md`
- `docs/emulator/04-video.md`
- `docs/emulator/06-debugger.md`

## Current Repo State

Observed implementation:
- `src/main.cpp` launches `frontend::run_frontend_app()`.
- `src/frontend/app.cpp` parses CLI flags, loads config/ROMs, builds a fixture
  frame, prints frontend/debugger status to stdout, and exits.
- `src/frontend/display.cpp` stores a `256x212` RGB frame in memory, computes a
  digest, and can dump PPM output. It does not create a GPU texture, swapchain,
  or visible window.
- `src/frontend/sdl_window.hpp` and `src/frontend/perf_overlay.hpp` are stub
  placeholders with no SDL or on-screen rendering behavior.
- `src/debugger/debugger.cpp` exposes layout and render snapshot metadata, but
  does not call Dear ImGui or draw interactive panels.
- `src/frontend/CMakeLists.txt` does not compile any SDL, OpenGL, or ImGui
  backend implementation files.

Observed runtime behavior:
- `build/src/vanguard8_frontend` starts, prints build/frontend status, and
  exits.
- `--video-fixture` prints controller port values and a deterministic frame
  digest.
- `--debugger` prints debugger snapshot metadata such as dockspace id, visible
  panel count, and CPU PC.
- No visible application window appears on the desktop.

## Documentation Drift Identified

The following documents describe a live SDL/OpenGL/ImGui desktop application
that does not exist in the current implementation:
- `docs/emulator/00-overview.md`
- `docs/emulator/01-build.md`
- `docs/emulator/02-emulation-loop.md`
- `docs/emulator/06-debugger.md`
- milestone definitions in `docs/emulator/07-implementation-plan.md`

The docs are directionally useful, but they currently read like implemented
behavior rather than target architecture. Milestone 12+ work must either bring
the implementation up to that design or narrow the design text where the target
has changed. This audit assumes the intended outcome is still a real Linux
desktop application with SDL2, OpenGL, and Dear ImGui.

## Required Desktop GUI Capabilities

The emulator should not be considered a GUI application until all of the
following are true.

### 1. Visible Application Lifetime

Required:
- A process lifetime that stays open until the user closes the window or quits.
- An SDL event loop that pumps window, keyboard, mouse, controller, and drop
  events continuously.
- Exit paths that shut down the window, GL context, audio device, and config
  persistence cleanly.

Missing today:
- No persistent frontend loop.
- No event pump.
- No window lifecycle.

### 2. Real Window and Graphics Backend

Required:
- SDL window creation with title, sizing, fullscreen/windowed transitions, and
  focus handling.
- OpenGL context creation and teardown.
- A display backend that uploads the composited `256x212` RGB frame to a real
  texture and presents it each frame.
- A path for shader selection or a documented nearest-neighbor fallback.
- Readback support or an equivalent abstraction so tests can still compare GUI
  output against headless output deterministically.

Missing today:
- `SdlWindow` is a stub.
- `Display` is only an in-memory upload buffer.
- No GL resources, no presentation path, no swap interval management.

### 3. Live Frontend Runtime Integration

Required:
- `run_frontend_app()` refactored from one-shot report generation into a looped
  application runtime.
- Tight integration between the frontend loop and the existing emulation
  scheduler so frames, audio, and debugger rendering are advanced continuously.
- A clear ownership model for frontend state: config, ROM session, window,
  display backend, debugger host, and audio output.

Missing today:
- No runtime object owning the desktop session.
- No continuous frame stepping or frame pacing in the GUI process.

### 4. Interactive Input Path

Required:
- SDL keyboard and gamepad events mapped into the existing controller port
  abstraction.
- Runtime hotkeys for debugger toggle, fullscreen, pause, frame advance, and
  screenshot functions once those surfaces are live.
- Text input and mouse focus routing that does not interfere with Dear ImGui
  when the debugger is open.

Missing today:
- Input bindings exist as deterministic abstractions and CLI flags, but not as
  live SDL event handling.

### 5. Live Audio Output

Required:
- SDL audio device initialization and teardown.
- A frontend-owned audio queue/ring buffer fed from the existing deterministic
  mixer/resampler path.
- Underrun/overrun handling that preserves determinism in headless mode while
  remaining usable in live desktop mode.

Missing today:
- Audio is verified through deterministic hashes and core-side sample
  generation, not through a live output device.

### 6. ROM Workflow and Desktop UX

Required:
- Launching the frontend should produce a window even before a ROM is loaded.
- ROM open flow must work through command-line path, drag-and-drop, and the
  documented native file dialog.
- Recent-ROM selection and frontend status need an on-screen surface instead of
  terminal-only reporting.
- Error paths such as invalid ROMs or backend initialization failures need
  user-visible messaging in the desktop app.

Missing today:
- The ROM workflow is CLI/status oriented.
- No visible empty-state UI or runtime error surface exists.

### 7. Live Dear ImGui Debugger

Required:
- Dear ImGui context creation and SDL/OpenGL backend wiring.
- Translation of the existing debugger snapshot/control classes into real ImGui
  draw code.
- Per-panel rendering for the milestone-9/11 debugger surfaces without changing
  the core ownership model.
- Layout persistence and hotkey toggles integrated into the real desktop loop.

Missing today:
- `DebuggerShell` is only a snapshot/data-model layer.
- No ImGui context or renderer backend exists.

### 8. Regression Strategy for GUI Work

Required:
- The desktop frontend must remain testable without requiring an interactive
  desktop session in CI.
- Backend-free unit tests should continue to verify deterministic frame
  composition, config handling, ROM loading, and debugger snapshots.
- New seams are needed around the window/display/audio hosts so lifecycle,
  config application, and frame presentation contracts can be tested with fakes
  while one or more integration tests exercise the real SDL/OpenGL path when
  available.

Missing today:
- The deterministic seams exist only for the stubbed frontend, not for a real
  backend host layer.

## Recommended Implementation Shape

The narrowest safe path is:

1. Preserve the current emulator core, headless binary, and deterministic test
   strategy as the source of truth.
2. Introduce explicit frontend host abstractions rather than pushing SDL/OpenGL
   calls directly into core classes.
3. Keep `Display` split between:
   - a backend-neutral frame upload/readback contract used by tests
   - a desktop GL presenter used only by the live frontend
4. Keep debugger panels split between:
   - snapshot/control logic that remains unit-testable
   - Dear ImGui presentation code that consumes those snapshots
5. Add desktop GUI work in layers so the first milestone produces a real window
   and visible video before audio, ROM UX polish, and live debugger work are
   stacked on top.

## Concrete File-Level Gaps

Likely required additions or refactors:
- `src/frontend/app.cpp`
  Refactor from report-style command handler into persistent app runtime.
- `src/frontend/sdl_window.*`
  Implement SDL window, GL context, fullscreen/window events, swap interval.
- `src/frontend/display.*`
  Split current digest/readback logic from actual GPU presentation backend.
- `src/frontend/audio_device.*`
  New desktop audio output host over the existing core mixer/resampler.
- `src/frontend/rom_session.*` or equivalent
  Centralize ROM open/reload/error/session state for the windowed app.
- `src/frontend/imgui_host.*`
  Own Dear ImGui context and backend integration.
- `src/debugger/*`
  Add live rendering adapters that consume existing snapshot/control classes.
- `src/frontend/CMakeLists.txt` and top-level build files
  Link SDL2, OpenGL, Dear ImGui backends, and any desktop-only helpers
  intentionally rather than relying on manifest dependencies alone.
- `tests/`
  Add frontend runtime, backend-fake, and desktop integration coverage.

## Milestone Mapping

The recommended roadmap is:
- Milestone 12: playable desktop frontend baseline with ROM launch, live video,
  live audio, minimal input, and lightweight runtime/debug status
- Milestone 13: critical pre-GUI compatibility closure for real ROM testing
- Milestone 14: lightweight bring-up tooling and only the smaller closures that
  prove necessary during ROM validation

This intentionally defers a full live Dear ImGui debugger path until it is
explicitly reprioritized. The immediate desktop goal is playable ROM bring-up,
not debugger-surface completeness.

That milestone series is defined formally in:
- `docs/emulator/07-implementation-plan.md`
- `docs/emulator/milestones/12.md`
- `docs/emulator/milestones/13.md`
- `docs/emulator/milestones/14.md`

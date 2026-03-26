# Vanguard 8 Emulator — Main Emulation Loop

## Overview

The emulator's inner loop is **event-scheduled**. A lightweight scheduler
maintains a sorted queue of upcoming hardware events expressed in master clock
cycles. Each iteration runs the CPU only until the next event boundary, fires
the event, then continues. This ensures that H-blank, V-blank, and /VCLK
interrupts are asserted at the correct master-cycle offset within the frame —
not batched to the end of a full scanline.

All timing is expressed in **master clock cycles** at 14.31818 MHz.
The CPU runs at master ÷ 2 (7.15909 MHz), so one CPU T-state = 2 master cycles.

---

## Key Timing Constants

```cpp
constexpr double MASTER_HZ       = 14'318'180.0;
constexpr int    CPU_DIVIDER     = 2;     // CPU clock = master / 2
constexpr int    AY_DIVIDER      = 8;     // AY-3-8910 clock = master / 8

// NTSC frame structure (both V9938s in Graphic 4, LN=1)
constexpr int ACTIVE_LINES       = 212;
constexpr int TOTAL_LINES        = 262;
constexpr int VBLANK_LINES       = TOTAL_LINES - ACTIVE_LINES; // 50

// Master cycles per scanline (approximate; must be consistent)
constexpr int MASTER_PER_LINE    = 910;   // 14.31818 MHz / 15.734 kHz ≈ 910

// CPU T-states per scanline
constexpr int CPU_TSTATES_PER_LINE = MASTER_PER_LINE / CPU_DIVIDER; // 455

// H-blank begins partway through the scanline (after 256 active pixels)
// Approximate; exact value from V9938 databook pixel clock timing
constexpr int MASTER_HBLANK_START = 512; // ~56% of scanline
```

---

## Event Scheduler

The scheduler is a priority queue of `Event` entries sorted by
`master_cycle_due`. At the start of each frame it is populated with all
events whose timing is known in advance. Events whose timing depends on
runtime state (e.g., YM2151 timer overflow) are inserted dynamically when
they occur.

```cpp
struct Event {
    uint64_t master_cycle_due;
    EventType type;             // HBLANK, VBLANK, VCLK, VBLANK_END, ...
    int       scanline;         // for HBLANK and VBLANK events
};
```

### Pre-queued events per frame

```
For each active scanline S in 0..211:
    HBLANK event at master cycle: frame_start + S * MASTER_PER_LINE + MASTER_HBLANK_START

VBLANK event at master cycle: frame_start + 212 * MASTER_PER_LINE

VBLANK_END event at master cycle: frame_start + 262 * MASTER_PER_LINE   (= next frame_start)

MSM5205 /VCLK events: at VCLK_PERIOD intervals throughout the frame,
    where VCLK_PERIOD = MASTER_HZ / sample_rate_hz
    (4 kHz → 3,580 cycles; 6 kHz → 2,386 cycles; 8 kHz → 1,790 cycles)
    These are pre-queued for the full frame when the sample rate is set,
    and re-queued at the start of each frame using the last VCLK timestamp
    as the base.
```

---

## Frame Loop Structure

```
populate_scheduler_for_frame()

while scheduler has events for this frame:
    next_event = scheduler.peek()
    run_cpu_and_audio_until(next_event.master_cycle_due)
    fire_event(next_event)
    scheduler.pop()

// After all events: render composited lines and present frame
present_frame()
sync_to_59_94_hz()
```

### run_cpu_and_audio_until(target_cycle)

The long-term runtime shape runs the CPU instruction-by-instruction and
advances audio chips in lockstep until the master cycle counter reaches
`target_cycle`.

```
while master_cycle < target_cycle:
    // Run one CPU instruction
    tstates = cpu.step()
    elapsed_master = tstates * CPU_DIVIDER
    master_cycle += elapsed_master

    // Advance audio chips by the same elapsed master cycles
    ym2151.advance(elapsed_master)
    ay8910.advance(elapsed_master / AY_DIVIDER)
    msm5205.advance(elapsed_master)

    // Advance VDP command engines by elapsed master cycles
    vdp_a.advance_command(elapsed_master)
    vdp_b.advance_command(elapsed_master)
```

**Milestone 3 implementation note:** the current headless scheduler uses
deterministic CPU T-state accounting at `master ÷ 2` and records event timing
in master cycles, but it does not yet expose full instruction-stepped CPU
execution or chip advancement through this loop. The milestone-3 verifier
locks the scheduler timing, frame accounting, and runtime controls; fuller
per-instruction stepping remains for later integration work.

If an INT0 or INT1 is pending and the CPU's enable conditions are met, the
CPU core handles the interrupt response inside `cpu.step()` (the core checks
pending interrupts before fetching each instruction, as the HD64180 does in
hardware). The interrupt acknowledge cycle is included in the returned T-state
count.

**DMA:** If a DMA transfer is active, `cpu.step()` suspends normal
fetch/execute and runs one DMA cycle instead. The CPU core handles this
internally. DMA cycles are counted as master cycles in the same way.

### fire_event(event)

```
HBLANK at scanline S:
    vdp_a.tick_scanline(S)          // render line S into line buffer
    vdp_b.tick_scanline(S)
    compositor.composite(S)         // merge line buffers into framebuffer row S
    if vdp_a.hblank_irq_enabled() and vdp_a.target_line() == S:
        vdp_a.set_hblank_flag()
        bus.int0_state.vdp_a_hblank = true   // gated on R#0 bit 4 (H-blank IRQ enable), not CPU mask
        // INT0 line is now asserted; CPU core will service on next step()

VBLANK:
    vdp_a.set_vblank_flag()    // always: sets S#0 bit 7 regardless of IRQ enable
    // INT0 is only asserted if VDP-A V-blank IRQ is enabled (R#1 bit 5).
    // This gates on the VDP's own /INT output enable — not on the CPU's mask.
    if vdp_a.vblank_irq_enabled():
        bus.int0_state.vdp_a_vblank = true
    // VDP-B: no interrupt output, unconnected

VBLANK_END (scanline 262, = next frame start):
    vdp_a.clear_vblank_flag()
    frame_start = master_cycle
    populate_scheduler_for_frame()
```

**VDP /INT output vs CPU interrupt mask are distinct.** The VDP only drives
its /INT pin low when both the relevant status flag is set AND the VDP's own
interrupt-enable register bit is set (R#1 bit 5 for V-blank; R#0 bit 4 for
H-blank). This is the VDP deciding whether to assert the line. The CPU
independently decides whether to service INT0 based on its own enable state
(IFF1, ITE0). The bus models the physical /INT line state — which is the
VDP's output, not a pre-filtered signal from the CPU.

### Tick VDPs

`vdp_a.tick_scanline(S)` and `vdp_b.tick_scanline(S)` are called at the
H-blank event boundary for each active scanline, and at the V-blank event
boundary for the transition scanline. Responsibilities:
- Render the scanline into the chip's internal line buffer (4bpp, 256 pixels)
- Evaluate sprite rendering for this line (update 5th-sprite flag, collision)

VDP-B never asserts any CPU interrupt. Its /INT line is unconnected.

### Audio — MSM5205 /VCLK → INT1

When `msm5205.advance(elapsed)` crosses a /VCLK boundary:
```
msm5205.on_vclk():
    bus.int1_asserted = true    // unconditional; CPU core services via I/IL vector
```

The INT1 response is mode-independent and uses the I/IL vector mechanism
(see `docs/spec/01-cpu.md`). The CPU core handles this on the next `step()`
call when INT1 is pending and ITE1 is set in the ITC register.

**Milestone 3 implementation note:** the current scheduler records deterministic
`/VCLK` event timestamps from the nominal 4/6/8 kHz rates in master cycles and
uses those timestamps as the INT1 timing proof surface. Full MSM5205 device
advancement and bus-level INT1 assertion are deferred to the later audio
integration milestone.

### Step 5 — Compositor

For each active scanline (0–211), merge VDP-A and VDP-B line buffers:

```
for x in 0..255:
    pixel_a = vdp_a.line_buffer[x]   // 4bpp color index
    pixel_b = vdp_b.line_buffer[x]   // 4bpp color index

    // VDP-A color 0 = transparent (YS pin high → mux selects VDP-B)
    if pixel_a == 0:
        framebuffer[scanline][x] = vdp_b.palette[pixel_b]
    else:
        framebuffer[scanline][x] = vdp_a.palette[pixel_a]
```

The output is a 256×212 array of 9-bit RGB values (3 bits per channel),
expanded to 24-bit RGB for the OpenGL framebuffer texture.

**Layer toggles** (debugger feature) short-circuit this logic:
- Disable VDP-A background: treat all VDP-A background pixels as color 0
- Disable VDP-A sprites: strip sprite pixels from VDP-A's line buffer before composite
- Disable VDP-B background / sprites: zero out VDP-B line buffer (or parts of it)

### Step 6 — Present Frame

Upload the 256×212 RGB framebuffer to the OpenGL texture, then draw the
fullscreen quad with the active shader (integer-scale pass-through or CRT).
Dear ImGui is rendered on top in the same OpenGL frame if the debugger is open.

### Step 7 — Frame Synchronization

SDL2 vsync is the preferred sync method. If the host display is not 60 Hz,
fall back to a software frame limiter targeting 59.94 Hz using
`SDL_GetPerformanceCounter`.

---

## INT0 Handler — Source Identification

The INT0 vector (0x0038 in IM1) is shared by VDP-A and YM2151. The handler
must read status registers to identify and clear all pending sources:

1. Read VDP-A S#0 (IN port 0x81) — clears V-blank flag; check bit 7 (VB)
2. Read VDP-A S#1 (select via R#15=1, read port 0x81, restore R#15=0) —
   clears H-blank flag; check bit 0 (FH)
3. Read YM2151 status (IN port 0x40) — check bit 1 (Timer A), bit 0 (Timer B)

The emulator must not de-assert INT0 until all three sources have been read
and their flags cleared, because the wire-OR means INT0 stays low as long
as any source is still asserting it.

---

## Speed Control

| Mode         | Implementation                                           |
|--------------|----------------------------------------------------------|
| Normal       | Frame limiter at 59.94 Hz                                |
| Fast-forward | Remove frame limiter; run as fast as host allows         |
| Slow motion  | Frame limiter set to a fraction (e.g., 25% = ~15 Hz)    |
| Pause        | Main loop blocks on input; no emulation ticks            |
| Frame advance| One frame executes then immediately pauses               |

Fast-forward still generates audio (at accelerated pitch), or optionally
mutes audio during fast-forward. Slow motion generates audio at reduced rate
with resampling compensation.

---

## Save States

A save state is a binary snapshot of all mutable emulator state:

- CPU: all registers (main + alternate sets, I, R, PC, SP, IX, IY,
  MMU registers, DMA state, timer state, ITC, IL, interrupt pending flags)
- SRAM: all 32 KB
- Cartridge: current BBR value (ROM data itself is not saved — the ROM file
  is re-linked on load)
- VDP-A: all registers (R#0–R#46), all status register state, VRAM (64 KB),
  palette (16 entries), sprite state, scanline counter
- VDP-B: same as VDP-A
- YM2151: full register state (all channels + operators)
- AY-3-8910: all 14 registers
- MSM5205: control register, ADPCM decoder state (step index, predictor)
- Master cycle counter, current scanline, carry-over T-states

Save states are stored as versioned binary files with a magic header.
The version field allows future format changes to be detected.

### Quicksave Slots

Ten numbered slots (0–9) are available. Each slot stores one save state file
at `~/.local/share/vanguard8/saves/<rom-hash>/slot<N>.v8s`. Alongside the
state file, a small metadata sidecar (`slot<N>.meta`) records:

```toml
rom     = "<sha256 of ROM>"
created = "<ISO-8601 timestamp>"
frame   = <frame number>
label   = ""          # user-editable description
```

A 128×96 PNG thumbnail of the composited framebuffer at save time is written
to `slot<N>.png` and shown in the slot picker UI.

Default hotkeys: `Shift+F1`–`Shift+F9` for quicksave to slots 1–9;
`F1`–`F9` for quickload. Slot 0 is accessible only via the UI (reserved as
a scratch slot with no accidental-overwrite risk).

---

## Rewind

The rewind system maintains a ring buffer of recent save states. While
playing, a snapshot is taken every 2 frames (at 59.94 Hz this yields ~30
snapshots per second). The default buffer holds 30 seconds of history
(~900 snapshots). The maximum configurable duration is 60 seconds (~1,800
snapshots).

### Memory budget

Each snapshot is approximately 180 KB (32 KB SRAM + 128 KB VRAM × 2 chips +
~1 KB CPU/audio/misc registers). At 900 snapshots the ring buffer uses ~162 MB.
At the 60-second maximum it uses ~324 MB. These are acceptable on a modern
Linux workstation. No delta compression is used — the implementation is kept
simple and the memory cost is treated as a known trade-off.

### Behaviour

- Rewind is active while the rewind key is held (default: `R`).
- Each held frame steps the ring buffer one snapshot backward and restores it.
- Audio plays in reverse during rewind (pitch-shifted), or can be muted via
  config.
- Rewind is automatically disabled during fast-forward to avoid filling the
  buffer with meaningless states.
- The ring buffer is cleared on ROM load and on any manual save-state load
  (to avoid rewinding past a discontinuity).

```toml
[rewind]
enabled        = true
duration_secs  = 30    # 10–60
mute_audio     = false
```

---

## Input Recording and Replay

The input recorder captures controller state on a per-frame basis and writes
it to a file. A replay file fully reproduces a session when played back from
the same ROM and starting state, making it suitable for bug reproduction and
regression testing.

### File format

```
Header (binary):
  Magic:           4 bytes  "V8RR"
  Version:         1 byte
  ROM SHA-256:     32 bytes
  Anchor type:     1 byte   (0 = power-on, 1 = embedded save state)
  Frame count:     4 bytes  (0 = open-ended / recorded until stop)
  [Save state:     variable, present only if anchor type = 1]

Per-frame records (one per emulated frame, sequential):
  Frame number:    4 bytes
  Controller 1:    1 byte   (active-low button state, port 0x00)
  Controller 2:    1 byte   (active-low button state, port 0x01)
```

### Recording

Recording is started from the emulator menu or with `--record output.v8r`.
On stop, the frame count field is filled in and the file is finalized. The
ROM SHA-256 is computed at load time. If anchored to a save state, the state
is embedded at the start of the file.

### Replay

Replay overrides the live controller input ports with the recorded values
for each frame. If the ROM SHA-256 does not match the loaded ROM, replay
refuses to start and exits with an error.

Replay is available interactively (menu or `--replay file.v8r`) and from
the command line in headless mode (see below). In headless mode, a mismatch
between the recorded frame count and actual execution produces a nonzero
exit code.

---

## Headless / CLI Mode

The `vanguard8_headless` binary runs the emulator core without an SDL window,
OpenGL context, or ImGui. It is built from the same `vanguard8_core` library
as the main emulator; only the frontend is absent.

### Use cases

- Automated regression testing against a known-good framebuffer or audio hash
- Running replay files in CI without a display
- Scripted ROM validation after emulator changes

### Command-line interface

```bash
vanguard8_headless --rom game.rom [options]

Options:
  --frames N          Run for exactly N frames then exit
  --replay file.v8r   Feed recorded input; exit when replay ends
  --dump-frame N      Write frame N framebuffer to frame_N.png
  --dump-audio        Write all generated audio to audio.wav
  --hash-frame N      Print SHA-256 of frame N framebuffer to stdout
  --hash-audio        Print SHA-256 of all generated audio to stdout
  --expect-frame-hash N HASH  Exit nonzero if frame N hash does not match
  --expect-audio-hash HASH    Exit nonzero if audio hash does not match
  --load-state file.v8s       Start from a save state instead of power-on
  --save-state N file.v8s     Write save state at frame N
```

Exit codes: `0` success, `1` hash mismatch, `2` replay ROM mismatch,
`3` ROM load failure, `4` emulator error.

### Regression test workflow

```bash
# Record a baseline
vanguard8_frontend --rom game.rom --record baseline.v8r

# Verify the baseline still holds after an emulator change
vanguard8_headless --rom game.rom --replay baseline.v8r \
  --hash-frame 3600 \
  --expect-frame-hash 3600 <known-hash>
```

Replay files stored in `tests/replays/` and run via `ctest` provide
system-level regression coverage that no unit test can replicate.

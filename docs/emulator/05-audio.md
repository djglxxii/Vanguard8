# Vanguard 8 Emulator — Audio System

## Overview

Three chips produce audio. Each is emulated by a dedicated library. The
`AudioMixer` collects their output, sums the channels into a stereo pair,
resamples to the SDL2 output rate, and writes to a ring buffer that SDL2
drains asynchronously.

| Chip       | Library       | Clock source          | Native sample rate       | Output    |
|------------|---------------|-----------------------|--------------------------|-----------|
| YM2151     | Nuked-OPM     | Master ÷ 4 (3.57954 MHz) | 3.57954 MHz ÷ 64 = **55,930 Hz** | Stereo (per-channel pan) |
| AY-3-8910  | Ayumi         | Master ÷ 8 (1.78977 MHz) | ~111,860 Hz (internal)  | Mono → center |
| MSM5205    | MAME MSM5205  | 384 kHz resonator (independent of master) | 4,000 / 6,000 / 8,000 Hz (selectable) | Mono → center |

Clock relationships are fully specified in `docs/spec/03-audio.md`. Key points
for the emulator:
- The YM2151 sample rate (55,930 Hz = master ÷ 256) is derived from the master
  clock via integer division and can be tracked on the master-cycle timeline.
- The MSM5205 384 kHz input clock has **no integer relationship to the master
  clock**. /VCLK events must be timed using the stated output rates (4/6/8 kHz)
  expressed as master-cycle intervals, not derived from the master clock.

The mixer normalizes all chips to a shared intermediate rate before summing,
then resamples to 48 kHz for SDL2 output.

---

## YM2151 — Nuked-OPM

**Library:** `nukeykt/Nuked-OPM` (`third_party/nuked-opm/`)

Nuked-OPM is a cycle-accurate YM2151 emulator. It advances one YM2151 master
clock cycle at a time and produces output at the YM2151's internal sample rate.

### Integration

```cpp
class YM2151 {
    OPM_State opm;                 // Nuked-OPM internal state

    void write_address(uint8_t addr);   // Port 0x40 write
    void write_data(uint8_t data);      // Port 0x41 write
    uint8_t read_status();              // Port 0x40 read (busy + IRQ flags)

    // Called from main loop: run for N master cycles, accumulate samples
    void run_cycles(int master_cycles, SampleBuffer& out);

    // IRQ output: YM2151 Timer A/B overflow → INT0
    bool irq_pending() const;
};
```

### Busy Flag

The YM2151 requires ~68 master cycles between register writes. The busy flag
(status port bit 7) is held high during this window. The Nuked-OPM core tracks
this internally; `read_status()` returns the current busy state.

Games that do not poll the busy flag will write too fast. The emulator does
not enforce write timing on the game's behalf — it passes the write to the
core, which handles it. This is the correct behavior (the hardware also does
not prevent fast writes; it just may not register them correctly).

### IRQ → INT0

When YM2151 Timer A or B overflows and IRQ is enabled (register 0x14),
`irq_pending()` returns true. The bus sets `int0_state.ym2151_timer = true`.
Reading port 0x40 (YM2151 status) clears the IRQ flags in the core and
allows the bus to de-assert `int0_state.ym2151_timer`.

### Stereo Output

The YM2151 produces stereo natively. Each of its 8 channels has independent
left/right enable bits (register 0x20+ch, bits 7:6). Nuked-OPM outputs a
left sample and right sample per clock cycle.

---

## AY-3-8910 — Ayumi

**Library:** `true-grue/ayumi` (`third_party/ayumi/`)

Ayumi is a small, accurate AY-3-8910 emulator that outputs floating-point
samples at a configurable rate.

### Integration

```cpp
class AY8910 {
    ayumi chip;                    // Ayumi internal state
    uint8_t selected_reg = 0;      // Latched register number

    void write_address(uint8_t reg);    // Port 0x50 write
    void write_data(uint8_t data);      // Port 0x51 write
    uint8_t read_data();                // Port 0x51 read

    void run_cycles(int ay_cycles, SampleBuffer& out);
};
```

### Clock

The AY-3-8910 clock is master ÷ 8 = 1.7897725 MHz.
In `run_cycles`, the main loop passes `master_cycles / 8` (integer division;
fractional cycles carried to the next call).

### Output Panning

The AY-3-8910 is center-panned (equal left + right). Ayumi's three channels
are summed to a mono value, then copied to both stereo output channels.

---

## MSM5205 — MAME Core

**Library:** MAME `msm5205.cpp` extracted to `third_party/msm5205/`

The MSM5205 decodes a stream of 4-bit ADPCM nibbles into a 12-bit PCM output.
The emulator must feed one nibble per /VCLK event via the INT1 handler.

### Integration

```cpp
class MSM5205 {
    MSM5205State state;            // Extracted MAME core state
    Bus* bus;                      // Back-reference for INT1 assertion

    void write_control(uint8_t data);   // Port 0x60: sample rate + reset
    void write_data(uint8_t nibble);    // Port 0x61: 4-bit ADPCM nibble

    // Called from main loop: advance by N master cycles
    // Fires INT1 via bus when /VCLK boundary is reached
    void run_cycles(int master_cycles, SampleBuffer& out);
};
```

### /VCLK Timing

The MSM5205 generates a /VCLK strobe at its selected sample rate. In master cycles:

| Sample Rate | Master cycles per /VCLK  |
|-------------|--------------------------|
| 4 kHz       | 14,318,180 / 4,000 = 3,580 |
| 6 kHz       | 14,318,180 / 6,000 = 2,386 |
| 8 kHz       | 14,318,180 / 8,000 = 1,790 |

`run_cycles` counts master cycles. When the accumulated count exceeds one
/VCLK period, it:
1. Notifies the MAME core to advance one ADPCM step
2. Calls `bus->assert_int1()` to trigger the INT1 handler
3. Subtracts one period from the accumulator

At 8 kHz, INT1 fires every ~1790 master cycles (~895 CPU cycles). At this
rate the CPU spends approximately 10–20 cycles per INT1 handler, leaving
875+ cycles for other work — a very lightweight interrupt.

### Port 0x60 — Control

```
Bits 1:0 — Sample rate: 00=4kHz, 01=6kHz, 10=8kHz, 11=reset/stop
Bit  7   — Reset: 1 = hold MSM5205 in reset (silent, no /VCLK)
```

When bit 7 is set (reset), `assert_int1()` is not called. /VCLK is suppressed.

### ADPCM Encoding

OKI ADPCM (compatible with standard ADPCM tools). Nibbles are stored high-
nibble-first in cartridge ROM. The game's INT1 handler reads the next byte
from a ROM pointer, shifts out the high nibble first, then the low nibble on
the subsequent INT1, and writes each to port 0x61.

---

## AudioMixer (`src/core/audio/audio_mixer.hpp`)

The mixer collects output from all three chips, normalizes them onto a shared
intermediate timeline, sums into a stereo pair, resamples to the SDL2 output
rate, and feeds the SDL2 audio callback ring buffer.

### Common Intermediate Rate

The three chips produce samples at incompatible native rates:

```
YM2151:   ~55,930 Hz  (master ÷ 256)
AY-3-8910: ~111,860 Hz (master ÷ 128)
MSM5205:   4,000 / 6,000 / 8,000 Hz (selectable)
```

Mixing "sample-by-sample" directly across these rates is undefined without
first normalizing to a common timeline. The mixer uses a **common intermediate
rate of 55,930 Hz** — the YM2151's native output rate, which is the lowest
of the three and therefore requires no upsampling for that chip.

Before summing, each chip's output is resampled to 55,930 Hz via libsamplerate:
- **YM2151**: already at 55,930 Hz — passed through unchanged.
- **AY-3-8910**: downsampled from ~111,860 Hz to 55,930 Hz (÷2 relationship
  means a simple decimate-by-2 with low-pass filter is sufficient, but
  libsamplerate is used for correctness).
- **MSM5205**: upsampled from 4/6/8 kHz to 55,930 Hz.

### Per-Frame Flow

```
1. At start of each frame: clear per-chip staging buffers.

2. During run_cpu_and_audio_until() calls: each chip appends samples to its
   own staging buffer at its native rate as master cycles elapse.

3. At end of frame: resample each chip's staging buffer to 55,930 Hz.

4. Sum resampled buffers sample-by-sample at 55,930 Hz:
     left  = (ym2151_L × vol_ym) + (ay_mono × vol_ay) + (msm_mono × vol_msm)
     right = (ym2151_R × vol_ym) + (ay_mono × vol_ay) + (msm_mono × vol_msm)

5. Resample summed stereo stream from 55,930 Hz to SDL2 output rate (48,000 Hz).

6. Push converted samples into SDL2 audio ring buffer.
```

Each chip writes to its own buffer (not a shared accumulator), so rates are
never conflated before the explicit resample step.

### Mismatch Handling

If the emulator runs slower than real time (host is too slow), the audio ring
buffer may underrun. If the emulator runs too fast (fast-forward), the buffer
may overflow. The mixer handles both:

- **Underrun:** SDL2 fills silence. Not ideal but not a crash.
- **Overflow:** Mixer drops the oldest samples in the ring buffer. During
  fast-forward, audio is typically muted or pitched up — the user controls this.

### Volume Trim

Each chip has an independent float volume multiplier (0.0–2.0, default 1.0).
Applied in the summing step before resampling. Accessible from the debugger's
audio panel and from the config file.

---

## SDL2 Audio Callback

SDL2 uses a push model (the callback pulls from the ring buffer when it needs
samples). The ring buffer is large enough to hold ~100 ms of audio at 48 kHz
to absorb frame-timing jitter without underrunning.

```cpp
void sdl_audio_callback(void* userdata, uint8_t* stream, int len) {
    AudioMixer* mixer = static_cast<AudioMixer*>(userdata);
    mixer->drain(stream, len);   // Pull from ring buffer; zero-fill if empty
}
```

SDL2 audio format: `AUDIO_F32SYS`, stereo, 48000 Hz, 1024-sample buffer.

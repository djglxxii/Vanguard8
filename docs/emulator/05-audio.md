# Vanguard 8 Emulator — Audio System

## Overview

Implemented against `docs/spec/03-audio.md`, milestone 7, and milestone 32.

`Bus` owns the three chip models, routes the documented ports, and advances
them on the master-cycle timeline. `AudioMixer` samples the chip outputs onto
the common 55,930 Hz timeline (master ÷ 256), then emits a deterministic 48 kHz
stereo stream. The headless path keeps that stream for SHA-256 regression
checks. The desktop frontend consumes the same bytes per frame and queues them
to SDL for live playback; there is no second frontend mixer or resampler.

| Chip       | Source                      | Clock / cadence                  | Output role |
|------------|-----------------------------|----------------------------------|-------------|
| YM2151     | Vendored Nuked-OPM          | Master ÷ 4 input, sampled at master ÷ 256 | Native stereo |
| AY-3-8910  | Vendored Ayumi              | Master ÷ 8 input, sampled at master ÷ 128 | Mono, center-panned |
| MSM5205    | Narrow extracted MAME logic | `/VCLK` at 4/6/8 kHz on master timeline | Mono, center-panned |

---

## Bus Surface

`src/core/bus.cpp` now routes the documented audio ports directly:

| Port | Direction | Device      | Behavior |
|------|-----------|-------------|----------|
| 0x40 | W         | YM2151      | Address latch |
| 0x40 | R         | YM2151      | Busy/status read |
| 0x41 | W         | YM2151      | Register data write |
| 0x50 | W         | AY-3-8910   | Address latch |
| 0x51 | W         | AY-3-8910   | Register data write |
| 0x51 | R         | AY-3-8910   | Latched register read |
| 0x60 | W         | MSM5205     | Rate select and reset/stop control |
| 0x61 | W         | MSM5205     | Next 4-bit ADPCM nibble |

The bus also mirrors the documented interrupt contribution:
- YM2151 timer IRQ contributes to `INT0` through `int0_state.ym2151_timer`
- MSM5205 `/VCLK` assertions accumulate onto `INT1`

---

## YM2151 Wrapper

Files:
- `src/core/audio/ym2151.hpp`
- `src/core/audio/ym2151.cpp`
- `third_party/nuked-opm/opm.c`
- `third_party/nuked-opm/opm.h`

`Ym2151` is a thin wrapper around the vendored Nuked-OPM state:
- `write_address()` handles port `0x40` writes
- `write_data()` handles port `0x41` writes
- `read_status()` exposes the vendored busy and timer status bits
- `irq_pending()` exposes the vendored timer IRQ output for `INT0`
- `advance_master_cycle()` clocks the chip on every fourth master cycle and
  captures a stereo sample every 64 YM clocks

Busy timing:
- The wrapped core exposes the busy flag after YM writes.
- The current test coverage locks the observed write timing window used by the
  vendored core rather than synthesizing a separate software busy counter.

---

## AY-3-8910 Wrapper

Files:
- `src/core/audio/ay8910.hpp`
- `src/core/audio/ay8910.cpp`
- `third_party/ayumi/ayumi.c`
- `third_party/ayumi/ayumi.h`

`Ay8910` keeps the documented register latch and applies masked register writes
into the vendored Ayumi state:
- Tone registers use the documented 12-bit period layout
- Noise, mixer, volume, and envelope registers are masked to their documented
  bit widths
- Port `0x51` reads return the currently latched register value

Timing model:
- The wrapper advances Ayumi every 128 master cycles
- Two AY native samples are averaged into each common 55,930 Hz mixer sample
- Output is treated as mono and copied equally into left/right

---

## MSM5205 Wrapper

Files:
- `src/core/audio/msm5205_adapter.hpp`
- `src/core/audio/msm5205_adapter.cpp`
- `third_party/msm5205/msm5205_core.cpp`
- `third_party/msm5205/msm5205_core.hpp`

The repo does not vendor the full MAME device framework for milestone 7.
Instead it keeps a narrow extraction of the ADPCM difference table and
step/signal update logic needed for the Vanguard 8 surface.

`Msm5205Adapter` implements:
- Port `0x60` control decoding:
  `00=4 kHz`, `01=6 kHz`, `10=8 kHz`, `11=stop/reset`, and bit `7` reset hold
- Port `0x61` nibble latching
- `trigger_vclk()` for scheduler-owned `/VCLK` events
- Center-panned mono output from the decoded ADPCM signal

Scheduler ownership:
- `Emulator::populate_scheduler_for_frame()` schedules `/VCLK` events from the
  current MSM5205 control state
- `Emulator::fire_event(EventType::vclk)` advances the adapter and asserts
  `INT1` through the bus

---

## Mixer And Resampler

Files:
- `src/core/audio/audio_mixer.hpp`
- `src/core/audio/audio_mixer.cpp`
- `src/frontend/audio_output.hpp`
- `src/frontend/audio_output.cpp`

The mixer is deterministic and master-clock-rooted:

1. YM2151, AY-3-8910, and MSM5205 advance inside `Bus::run_audio()`
2. Every 256 master cycles, `AudioMixer` captures one common stereo sample
3. AY contributes the average of its two 128-cycle native samples
4. MSM5205 contributes its most recent decoded mono sample as a held value
5. The current common sample is resampled to 48 kHz using a rational master-
   clock stepper
6. The mixer appends signed 16-bit little-endian stereo PCM bytes and folds
   each sample into a deterministic digest

The headless verifier reads `AudioMixer::output_bytes()` after the requested
frame count and hashes the complete PCM stream. The desktop frontend uses
`AudioQueuePump` once per runtime frame. The pump reads
`AudioMixer::consume_output_bytes()` only when the SDL device queue is below the
configured high-water mark, so back-pressure does not silently discard pending
mixer bytes. `SdlAudioOutputDevice` opens an SDL queue-backed stereo device at
the configured sample rate, starts playback with `SDL_PauseAudioDevice(..., 0)`,
queues the mixer bytes with `SDL_QueueAudio`, reports queued bytes for status,
and clears/closes the device during shutdown.

The default output contract is:
- 48,000 Hz
- stereo
- signed 16-bit little-endian SDL format on the opened device
- bytes supplied only by the deterministic `AudioMixer`

## Interactive Audio Verification

For human review, build the desktop frontend and launch the target ROM:

```bash
cmake --build cmake-build-debug
cmake-build-debug/src/vanguard8_frontend \
    /home/djglxxii/src/PacManV8/build/pacman.rom
```

The positional ROM form above is accepted for the milestone-32 listening flow;
`--rom /path/to/rom` remains accepted as an equivalent explicit form. The
window title and F10 status output include the current SDL queued-byte count.

The PacManV8 T016/T017 review target is the first 300 frames: PSG cues at
frames `0`, `12`, `36`, `72`, and `112`, then FM cues at frames `144`, `196`,
and `240`.

The milestone-32 bring-up attempt on 2026-04-19 proved that the previous
300-frame digest
`0d634fb059d15b12c4a8faf2412fbe08b85d187a31b1f22278ce3662f3b44390` was
deterministic silence (959,128 zero bytes) caused by a core CPU/audio
co-scheduling blocker, not an SDL queueing issue. Milestone 33 corrected that
by advancing audio hardware time between scheduled CPU instructions. The
current nonzero PacManV8 300-frame guard is:

```bash
cmake-build-debug/src/vanguard8_headless \
    --rom /home/djglxxii/src/PacManV8/build/pacman.rom \
    --frames 300 --hash-audio \
    --expect-audio-hash \
    24ce40791e466f9f686ee472b5798128065458e06a51f826666ae444ddfb5c75
```

---

## Verification Surface

Milestone-7 verification is locked by:
- `tests/test_audio.cpp`
- `tests/test_bus.cpp`
- `tests/test_scheduler.cpp`
Milestone-32 frontend playback verification is locked by:
- `tests/test_frontend_backends.cpp`
- `tests/test_frontend_runtime.cpp`

Covered rules:
- YM2151 busy/status reads are routed and deterministic
- AY latch/read/write behavior matches the documented two-port interface
- MSM5205 control writes change `/VCLK` cadence at the documented 4/6/8 kHz
  rates
- Repeated runs produce stable `/VCLK` counts and stable audio digests
- SDL audio devices open, queue, close, and reopen through the public frontend
  audio contract
- `AudioQueuePump` handles empty input, successful queueing, and queue
  back-pressure without consuming pending mixer bytes
- The fake frontend audio-device path records a byte-identical PCM stream to
  per-frame headless mixer consumption
- The PacManV8 300-frame T017 digest is nonzero and remains
  `24ce40791e466f9f686ee472b5798128065458e06a51f826666ae444ddfb5c75`

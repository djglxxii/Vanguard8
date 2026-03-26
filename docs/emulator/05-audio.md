# Vanguard 8 Emulator — Audio System

## Overview

Implemented against `docs/spec/03-audio.md` and milestone 7.

The current audio path is entirely core-side. `Bus` owns the three chip models,
routes the documented ports, and advances them on the master-cycle timeline.
`AudioMixer` samples the chip outputs onto the common 55,930 Hz timeline
(master ÷ 256), then emits a deterministic 48 kHz stereo stream and hash for
tests/headless verification.

Milestone note:
- Milestone 7 does not permit `src/frontend/` edits, so the current verified
  output is the core/headless digest path, not a live SDL callback.

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

The current mixer is deterministic and master-clock-rooted:

1. YM2151, AY-3-8910, and MSM5205 advance inside `Bus::run_audio()`
2. Every 256 master cycles, `AudioMixer` captures one common stereo sample
3. AY contributes the average of its two 128-cycle native samples
4. MSM5205 contributes its most recent decoded mono sample as a held value
5. The current common sample is resampled to 48 kHz using a rational master-
   clock stepper
6. The mixer folds the 48 kHz stereo output into a deterministic digest

The current implementation does not yet allocate an SDL ring buffer. That
frontend playback path can be added later once a milestone explicitly opens
`src/frontend/` again for audio output work.

---

## Verification Surface

Milestone-7 verification is locked by:
- `tests/test_audio.cpp`
- `tests/test_bus.cpp`
- `tests/test_scheduler.cpp`

Covered rules:
- YM2151 busy/status reads are routed and deterministic
- AY latch/read/write behavior matches the documented two-port interface
- MSM5205 control writes change `/VCLK` cadence at the documented 4/6/8 kHz
  rates
- Repeated runs produce stable `/VCLK` counts and stable audio digests

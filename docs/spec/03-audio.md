# Vanguard 8 — Audio System Reference

## Overview

The Vanguard 8 audio system combines three off-the-shelf chips:

| Chip       | Type              | Channels | Role                              |
|------------|-------------------|----------|-----------------------------------|
| YM2151     | 4-operator FM     | 8        | Music, leads, bass, pads          |
| AY-3-8910  | PSG (square wave) | 3 + noise| SFX, simple tones, background hum |
| MSM5205    | ADPCM             | 1        | Drum samples, voice, SFX accents  |

All three chips are mixed and output as **stereo**. The YM2151 provides
per-channel stereo panning natively. The AY-3-8910 and MSM5205 outputs are
summed and distributed across the stereo field via the output mixer.

---

## YM2151 (OPM — FM Operator Type M)

### Overview

The YM2151 is an 8-channel, 4-operator FM synthesis chip. Each channel uses
four sine-wave oscillators (operators) connected in one of eight configurable
algorithms, enabling a wide range of timbres from simple sine tones to complex
metallic and orchestral sounds.

- **Channels:** 8 independent FM voices
- **Operators per channel:** 4 (64 operators total)
- **Algorithms:** 8 (operator routing configurations)
- **Stereo panning:** Per channel, left/right/both via register
- **LFO:** 1 global LFO (vibrato, tremolo)
- **Noise:** 1 noise generator assignable to operator 4 of channel 7

### Clock

| Signal            | Derivation                        | Value          |
|-------------------|-----------------------------------|----------------|
| YM2151 input clock | Master ÷ 4 (NTSC color subcarrier) | 3.57954 MHz   |
| Audio sample rate  | YM2151 clock ÷ 64                 | ~55,930 Hz     |

The YM2151 is clocked from the same NTSC subcarrier signal (master ÷ 4) used
for composite video generation. Its internal sample rate is always input
clock ÷ 64, giving 3,579,545 ÷ 64 = **55,930.4 Hz** on the Vanguard 8.
In master clock terms: master ÷ 256.

### Port Reference

| Port  | Dir | Function                        |
|-------|-----|---------------------------------|
| 0x40  | W   | Address register                |
| 0x40  | R   | Status register (busy / IRQ)    |
| 0x41  | W   | Data register                   |

Always poll the busy flag (status register bit 7) before writing. The YM2151
requires ~68 master clock cycles between writes.

### Register Map

#### Global Registers

| Register | Function                                          |
|----------|---------------------------------------------------|
| 0x08     | Key on/off — channel (bits 2:0), operators (bits 6:3) |
| 0x0F     | Noise enable (bit 7), noise frequency (bits 4:0)  |
| 0x10     | Timer A high 8 bits                               |
| 0x11     | Timer A low 2 bits                                |
| 0x12     | Timer B                                           |
| 0x14     | Timer control / IRQ flags                         |
| 0x18     | LFO frequency                                     |
| 0x19     | LFO waveform / amplitude modulation depth         |
| 0x1B     | CT (control output) / LFO waveform select         |

#### Per-Channel Registers (base address + channel 0–7)

| Register     | Function                                         |
|--------------|--------------------------------------------------|
| 0x20 + ch    | Left/right enable (bits 7:6), feedback (bits 5:3), algorithm (bits 2:0) |
| 0x28 + ch    | Key code (octave + note)                         |
| 0x30 + ch    | Key fraction                                     |
| 0x38 + ch    | PMS (LFO phase mod sensitivity), AMS (amp mod sensitivity) |

#### Per-Operator Registers (base address + ch + op×8, op = 0–3)

| Register     | Function                                         |
|--------------|--------------------------------------------------|
| 0x40 + off   | DT1 (detune 1), MUL (frequency multiplier)      |
| 0x60 + off   | TL (total level / volume)                        |
| 0x80 + off   | KS (key scaling), AR (attack rate)               |
| 0xA0 + off   | AMS enable, D1R (first decay rate)               |
| 0xC0 + off   | DT2 (detune 2), D2R (second decay rate)          |
| 0xE0 + off   | D1L (decay level), RR (release rate)             |

Operator offset = channel + (operator × 8), operator order: M1=0, M2=1, C1=2, C2=3.

### FM Algorithms

```
Alg 0:  M1→M2→C1→C2 (out)            Full serial chain
Alg 1:  (M1+M2)→C1→C2 (out)          Two modulators into serial
Alg 2:  M1→(M2+C1)→C2 (out)
Alg 3:  M1→M2→(C1+C2) (out) [2 out]
Alg 4:  M1→C1 + M2→C2 (out) [2 out]  Two parallel FM pairs
Alg 5:  M1→(C1+C2+M2) (out) [3 out]  One modulator, 3 carriers
Alg 6:  M1→C1 + M2 + C2 (out) [3 out]
Alg 7:  M1 + M2 + C1 + C2 (out) [4 out] All carriers (additive)
```

### Timer / IRQ

The YM2151 has two internal timers (A and B) that generate an IRQ on the
CPU /INT line. Timer A is 10-bit (fine resolution); Timer B is 8-bit (coarser).
This IRQ is used to drive the audio sequencer at a regular tick rate independent
of the video frame.

---

## AY-3-8910 (PSG)

### Overview

The AY-3-8910 is a 3-channel programmable sound generator. Each channel
produces a square wave at a programmable frequency. A shared noise generator
can be mixed into any channel. Each channel has a volume register that can
either be set to a fixed level or follow the output of the built-in envelope
generator.

- **Square wave channels:** 3 (A, B, C)
- **Noise generator:** 1 (mixable into any channel)
- **Envelope generators:** 1 (shared across channels)
- **I/O ports:** 2 (not used on Vanguard 8; pins left unconnected)

### Port Reference

| Port  | Dir | Function          |
|-------|-----|-------------------|
| 0x50  | W   | Address latch     |
| 0x51  | W   | Register write    |
| 0x51  | R   | Register read     |

Write the register number to 0x50, then read/write data via 0x51.

### Register Map

| Register | Function                                              |
|----------|-------------------------------------------------------|
| 0        | Channel A fine tone period (bits 7:0)                 |
| 1        | Channel A coarse tone period (bits 3:0)               |
| 2        | Channel B fine tone period                            |
| 3        | Channel B coarse tone period                          |
| 4        | Channel C fine tone period                            |
| 5        | Channel C coarse tone period                          |
| 6        | Noise period (bits 4:0)                               |
| 7        | Mixer: enable/disable tone+noise per channel (active low) |
| 8        | Channel A volume (bits 3:0); bit 4 = use envelope     |
| 9        | Channel B volume                                      |
| 10       | Channel C volume                                      |
| 11       | Envelope fine period                                  |
| 12       | Envelope coarse period                                |
| 13       | Envelope shape / cycle                                |

### Tone Frequency Formula

```
Frequency (Hz) = Clock / (16 × Period)
Clock = 1.7897725 MHz (master ÷ 8)

Period range: 1–4095
Frequency range: ~27 Hz – ~112 kHz
```

---

## OKI MSM5205 (ADPCM)

### Overview

The MSM5205 is a low-cost 4-bit ADPCM (Adaptive Differential PCM) codec.
It decodes a stream of 4-bit nibbles from the CPU into a 12-bit PCM output,
driving the DAC directly. Samples are streamed from cartridge ROM by the CPU
(or via HD64180 DMA) one nibble at a time per sample clock tick.

- **Bit depth:** 4-bit ADPCM (effectively ~12-bit decoded)
- **Sample rates:** Selectable — 4 kHz, 6 kHz, 8 kHz
- **Channels:** 1 (mono; mixed into stereo at output stage)

### Clock

The MSM5205 is clocked from a dedicated **384 kHz ceramic resonator** on the
Vanguard 8 board. This clock is independent of the 14.31818 MHz master crystal
— there is no integer division relationship between them. The three selectable
/VCLK output rates are derived from the 384 kHz input by fixed internal
divisors:

| Port 0x60 bits 1:0 | Divisor | /VCLK rate | Best Use               |
|--------------------|---------|------------|------------------------|
| 00                 | ÷ 96    | 4,000 Hz   | Voice, low-BW SFX      |
| 01                 | ÷ 64    | 6,000 Hz   | General SFX            |
| 10                 | ÷ 48    | 8,000 Hz   | Drums, higher-quality  |
| 11                 | —       | Stop/reset | —                      |

Because the MSM5205 clock is independent of the master clock, the emulator
must time /VCLK events using the stated output rates (4/6/8 kHz) expressed as
master-cycle periods, not by deriving from the master clock through division.

### Port Reference

| Port  | Dir | Function                                      |
|-------|-----|-----------------------------------------------|
| 0x60  | W   | Control: sample rate select (bits 1:0), reset (bit 7) |
| 0x61  | W   | Data: 4-bit ADPCM nibble (bits 3:0)           |

The MSM5205 generates a **/VCLK** signal (sample clock strobe) at the selected
sample rate. On the Vanguard 8, **/VCLK connects to the CPU /INT1 line**. The
INT1 handler feeds one nibble to port 0x61 on each assertion. This gives ADPCM
a fully dedicated interrupt, independent of INT0 (video/audio sequencer).

### Sample Rate Selection (Port 0x60 bits 1:0)

| Value | Sample Rate | Best Use                        |
|-------|-------------|---------------------------------|
| 00    | 4 kHz       | Voice, low-bandwidth SFX        |
| 01    | 6 kHz       | General SFX                     |
| 10    | 8 kHz       | Drums, higher-quality SFX       |
| 11    | Reset / stop|                                 |

### ADPCM Format

The MSM5205 uses OKI ADPCM encoding (compatible with standard ADPCM tools).
Each nibble encodes a step value and direction relative to the previous sample.
The decoder maintains a step size that adapts based on the signal. Samples are
stored in cartridge ROM as packed 4-bit nibbles (2 nibbles per byte, high nibble
first).

---

## Stereo Output and Mixing

```
YM2151  ──[per-channel L/R pan]──┬──▶ Left  channel  ──▶ Output
                                  └──▶ Right channel  ──▶ Output

AY-3-8910 ──────────────────────── Summed and center-panned (L+R)

MSM5205 ─────────────────────────── Center-panned (L+R) or configurable
```

The final analog mix is a passive resistor network summing all three sources
before the output buffer. No custom mixer chip is required.

[Back to index](../index.md)

# Effects

This document describes the functionality and parameter structure of all available effects implemented in `Effects.h`. See `Defines.h` for enum definitions.

---

## `Distortion`

**Description:**  
Applies nonlinear distortion to an input signal. Includes several [algorithms](https://www.desmos.com/calculator/qrqipgp7r4): tube, soft/hard clipping, diode, bitcrush, rectify, and saturation. Optional FIR anti-aliasing filter available.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`MODE`** — Distortion algorithm [`DistortionMode` enum]
2. **`DRIVE`** — Input gain before nonlinearity `[0.0, 1.0]`
3. **`ENABLE_AAF`** — Enables Anti-Aliasing Filter (AAF) `true / false`

---

## `Delay`

**Description:**  
Implements a feedback delay line with adjustable delay time, feedback, and dry/wet mix.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`DELAY_TIME`** — Delay time in ms `[1.0, 500.0]`
2. **`FEEDBACK`** — Feedback coefficient `[-0.95, 0.95]`

---

## `Flanger`

**Description:**  
Combines a short modulated delay line with the dry signal to produce a sweeping comb-filter effect.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`RATE`** — LFO rate in Hz `[0.0, 20.0]`
2. **`DEPTH`** — LFO modulation depth `[0.0, 1.0]`
3. **`FEEDBACK`** — Feedback coefficient `[-0.95, 0.95]`

---

## `Phaser`

**Description:**  
Uses a cascade of modulated all-pass filters (`APF`) to create phase-cancellation notches in the frequency spectrum.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`RATE`** — LFO rate in Hz `[0.0, 20.0]`
2. **`CENTER_FREQ`** — Central `APF` cutoff frequency in Hz `[50.0, 8000.0]` 
3. **`SPREAD`** — Spread factor of `APF` centers `[0.1, 1.0]`
4. **`DEPTH`** — LFO modulation depth `[0.0, 1.0]`
5. **`FEEDBACK`** — Feedback coefficient `[-0.95, 0.95]`

---

## `Chorus`

**Description:**  
Creates a multi-voice detuned delay ensemble effect. Simulates multiple slightly detuned signals.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`RATE`** — Modulation rate in Hz `[0.0, 20.0]`
2. **`DEPTH`** — Modulation depth in ms `[0.0, 25.0]`
3. **`DELAY_TIME`** — Base delay time in ms `[0.0, 20.0]`
4. **`FEEDBACK`** — Feedback coefficient `[-0.95, 0.95]`

---

## `Reverb`

**Description:**  
Implements the [Dattorro reverb](https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf) algorithm (feedback delay network with diffusion, modulation, and damping filters).

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`PREDELAY_TIME`** — Predelay time in ms `[0.0, 100.0]`
2. **`DECAY_TIME`** — Decay time in ms `[100.0, 10000.0]`
3. **`MOD_RATE`** — Modulation rate in Hz `[0.05, 5.0]`
4. **`MOD_DEPTH`** — Modulation depth `[0.0, 1.0]`

---

## `Compressor`

**Description:**  
Dynamic range compressor, RMS detection, [gain reduction](https://www.desmos.com/calculator/wkmkrmn9le), soft knee, and optional auto makeup gain.
**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`  
1. **`THRESHOLD`** — Threshold in dB `[-200.0, 0.0]`
2. **`RATIO`** — Compression ratio `[1.0, 100.0]`
3. **`KNEE`** — Knee width in dB `[0.0, 40.0]`
4. **`ATTACK_TIME`** — Attack time in ms `[0.01, 250.0]`
5. **`RELEASE_TIME`** — Release time in ms `[10.0, 2500.0]`
6. **`MAKEUP_GAIN`** — Gain applied post compression in dB `[-72.0, 36.0]`
7. **`AUTO_MAKEUP`** — Enables automatic makeup gain `true / false`

---

## `Equalizer`

**Description:**  
Two-band parametric EQ using `Biquad` filters. Supports multiple filter types per band: LPF, HPF, low shelf, high shelf, peak, and notch.

**Parameters:**

0. **`EQ_MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`BAND1_TYPE`** — Filter type for band 1 (`BiquadType` enum)
2. **`BAND1_CUTOFF`** — Cutoff frequency in Hz `[20.0, 20000.0]`
3. **`BAND1_Q`** — Quality factor `[0.02, 40.0]`
4. **`BAND1_GAIN`** — Gain in dB `[-24.0, 24.0]`
5. **`BAND2_TYPE`** — Filter type for band 2 (`BiquadType` enum)
6. **`BAND2_CUTOFF`** — Cutoff frequency in Hz `[20.0, 20000.0]`
7. **`BAND2_Q`** — Quality factor `[0.02, 40.0]`
8. **`BAND2_GAIN`** — Gain in dB `[-24.0, 24.0]`

---

## `Granulator`

**Description:**  
Granular synthesis engine that generates small overlapping audio grains from a continuously running audio buffer.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`POSITION`** — Grain start position `[0.0, 1.0]`
2. **`POSITION_RAND`** — Amount of position randomization `[0.0, 1.0]`
3. **`RATE`** — Grain spawn rate in ms `[1.0, 500.0]`
4. **`RATE_RAND`** — Amount of spawn rate randomization `[0.0, 1.0]`
5. **`LENGTH`** — Grain length in ms `[5.0, 500.0]`
6. **`LENGTH_RAND`** — Amount of length randomization `[0.0, 1.0]`
7. **`LEVEL`** — Grain level `[0.0, 1.0]`
8. **`LEVEL_RAND`** — Amount of level randomization `[0.0, 1.0]`
9. **`REVERSE_CHANCE`** — Probability of reversed grain `[0.0, 1.0]`
10. **`ENVELOPE_TYPE`** — Grain envelope shape (`EnvelopeType` enum)

---

## `Freezer`

**Description:**  
Audio freezing effect with both **time-domain** and **spectral** looping modes. Loops over or spectrally re-synthesizes a section from a continuously running audio buffer.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`RATE`** — Playback rate `[-4.0, 4.0]`
2. **`SPECTRAL_MODE`** — Toggles spectral re-synthesis mode `true / false`
3. **`FFT_SIZE`** — (Spectral) Size of FFT window (power of 2) `[128, FFT_MAX_SIZE]`
4. **`HOP_SIZE`** — (Spectral) Overlap hop factor `[2, 8]`
5. **`LOOP_START`** — Normalized loop start position `[0.0, 1.0]`
6. **`LOOP_END`** — Normalized loop end position `[0.0, 1.0]`

---

## `SpectralGate`

**Description:**  
Spectral-domain gate that removes frequency bins below a certain magnitude threshold.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`FFT_SIZE`** — Size of FFT window (power of 2) `[128, FFT_MAX_SIZE]`
2. **`THRESHOLD`** — Magnitude threshold in dB; bins below this are gated `[-100.0, 0.0]`
3. **`TILT`** — Gate tilt bias (positive = more low-end gating, negative = more high-end gating) `[-1.0, 1.0]` 

---

## `FormantShifter`

**Description:**  
Alexander Panos' **GOATed** [formant shifter device](https://alexanderpanos.com/software) faithfully ported from Max/MSP (Gen) to C++! Because this processing is done in the frequency domain, it is possible to formant shift polyphonic audio.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`FFT_SIZE`** — Size of FFT window (power of 2) `[128, FFT_MAX_SIZE]`
2. **`FORMANT_SHIFT`** — Formant shift in semitones `[-12.0, 12.0]`
3. **`ENVELOPE_WIDTH`** — Adjust the smoothness of the spectral envelope `[0, 16]` 
    - Affects the balance between the input signal's pitch and its shifted formants. 
    - Note that very low values may cause leakage of inharmonic pitch information.

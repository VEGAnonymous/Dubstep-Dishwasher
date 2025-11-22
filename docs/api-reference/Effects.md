[Back to index](../index.md)

# Effects

This document describes the functionality and parameter structure of all available effects. All effects inherit from the `Effect` abstract base class or its `Phase_Vocoder` subclass. See `Defines.h` for enum definitions.

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

## `Compressor`

**Description:**  
Dynamic range compressor: RMS detection, [gain reduction](https://www.desmos.com/calculator/wkmkrmn9le), soft knee, and optional auto makeup gain.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`  
1. **`THRESHOLD`** — Threshold in dB `[-60.0, 0.0]`
2. **`RATIO`** — Compression ratio `[1.0, 100.0]`
3. **`KNEE`** — Knee width in dB `[0.0, 40.0]`
4. **`ATTACK_TIME`** — Attack time in ms `[0.01, 250.0]`
5. **`RELEASE_TIME`** — Release time in ms `[10.0, 2500.0]`
6. **`MAKEUP_GAIN`** — Gain applied post compression in dB `[-72.0, 36.0]`
7. **`AUTO_MAKEUP`** — Enables automatic makeup gain `true / false`

---

## `Delay`

**Description:**  
Implements a feedback delay line.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`DELAY_TIME`** — Delay time in ms `[1.0, 500.0]`
2. **`FEEDBACK`** — Feedback coefficient `[-0.95, 0.95]`

---

## `Distortion`

**Description:**  
Applies nonlinear distortion to an input signal. Includes several [algorithms](https://www.desmos.com/calculator/qrqipgp7r4): tube, soft/hard clipping, diode, bitcrush, rectify, and saturation.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`MODE`** — Distortion algorithm [`DistortionMode` enum]
2. **`DRIVE`** — Input gain before nonlinearity `[0.0, 1.0]`

---

## `Equalizer`

**Description:**  
Two-band parametric EQ using `Biquad` filters. Supports multiple filter types per band: LPF, HPF, low shelf, high shelf, peak, and notch.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`BAND1_TYPE`** — Filter type for band 1 (`BiquadType` enum)
2. **`BAND1_CUTOFF`** — Cutoff frequency in Hz `[20.0, 20000.0]`
3. **`BAND1_Q`** — Quality factor `[0.02, 40.0]`
4. **`BAND1_GAIN`** — Gain in dB `[-24.0, 24.0]`
5. **`BAND2_TYPE`** — Filter type for band 2 (`BiquadType` enum)
6. **`BAND2_CUTOFF`** — Cutoff frequency in Hz `[20.0, 20000.0]`
7. **`BAND2_Q`** — Quality factor `[0.02, 40.0]`
8. **`BAND2_GAIN`** — Gain in dB `[-24.0, 24.0]`

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

## `FormantShifter`

**Description:**  
Alexander Panos' **GOATed** [formant shifter device](https://alexanderpanos.com/software) faithfully ported from Max/MSP (Gen) to C++! Because this processing is done in the frequency domain, it is possible to formant shift polyphonic audio. Inherits from `Phase_Vocoder`.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`FFT_SIZE`** — Size of FFT window (power of 2) `[128, FFT_MAX_SIZE]`
2. **`FORMANT_SHIFT`** — Formant shift in semitones `[-12.0, 12.0]`
3. **`ENVELOPE_WIDTH`** — Adjust the smoothness of the spectral envelope `[0, 16]` 
    - Affects the balance between the input signal's pitch and its shifted formants. 
    - Note that very low values may cause leakage of inharmonic pitch information.

---

## `Freezer`

**Description:**  
Audio buffer looper with both time-domain and spectral modes. Loops over or spectrally re-synthesizes a section from a continuously running audio buffer.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`RATE`** — Playback rate `[-4.0, 4.0]`
2. **`SPECTRAL_MODE`** — Toggles spectral re-synthesis mode `true / false`
3. **`FFT_SIZE`** — (Spectral) Size of FFT window (power of 2) `[128, FFT_MAX_SIZE]`
5. **`LOOP_START`** — Normalized loop start position `[0.0, 1.0]`
6. **`LOOP_END`** — Normalized loop end position `[0.0, 1.0]`

---

## `Gain`

**Description:**  
Simple gain stage utility. Hard-clips the output in case of emergencies.

**Parameters:**

0. **`GAIN`** — Gain in dB `[-24.0, 24.0]`
1. **`CLIP`** — Whether to hard-clip the output to 0 dB `true / false`

---

## `Gate`

**Description:**  
Noise gate that attenuates signals below the threshold. Uses RMS detection and simple gain smoothing.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`  
1. **`THRESHOLD`** — Threshold in dB `[-60.0, 0.0]`
4. **`ATTACK_TIME`** — Attack time in ms `[0.01, 250.0]`
5. **`RELEASE_TIME`** — Release time in ms `[0.01, 1500.0]`
6. **`HOLD_TIME`** — Hold time in ms `[1.0, 1500.0]`
7. **`INVERT`** — Inverts gate logic `true / false`

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
7. **`TUNE`** — Grain tune (via resampling) in semitones `[-24.0, 24.0]`
8. **`TUNE_RAND`** — Amount of tune randomization `[0.0, 1.0]`
9. **`LEVEL`** — Grain level `[0.0, 1.0]`
10. **`LEVEL_RAND`** — Amount of level randomization `[0.0, 1.0]`
11. **`REVERSE_CHANCE`** — Probability of reversed grain `[0.0, 1.0]`
12. **`ENVELOPE_TYPE`** — Grain envelope shape (`EnvelopeType` enum)

---

## `Modulation`

**Description:**  
AM / RM modulation. Use low frequency AM for a tremolo effect.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`MODE`** — Type of modulation [`ModulationEffectMode` enum]
2. **`MODULATOR`** — Modulator wavetable [`WavetableType` enum]
3. **`FREQ`** — Modulator frequency in Hz `[1.0, 2000.0]`
4. **`DEPTH`** — Modulator depth (AM only) `[0.0, 1.0]`
5. **`BIAS`** — Modulator positive bias amount `[0.0, 1.0]`
6. **`RECTIFY`** — Modulator ± rectification amount `[-1.0, 1.0]`

---

## `Parallel`

**Description:**  
Processes two internal effect chains in parallel. Maximum of 5 effects per chain.

**Parameters:**

Uses a unique addressing scheme for internal control and parameters, assuming a `uint_8t` pid and `float` value:
``` 
CHAIN A
0-12:    Effect 1 parameter space
13-16:   Effect 1 command space
17-29:   Effect 2 parameter space
30-33:   Effect 2 command space
...
CHAIN B
85-97:   Effect 1 parameter space
98-101:   Effect 1 command space
102-114:  Effect 2 parameter space
115-118: Effect 2 command space
...
```

Where the command space is defined as:
```
ADD = 13,
REMOVE = 14,
REORDER = 15,
BYPASS = 16
```
Thus, pids 0-169 control the state of the internal chain. pids 170-253 are unused.

254. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
255. **`MODE`** — Mixdown mode [`ParallelMode` enum]

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

## `PitchShifter`

**Description:**  
Time-domain pitch shifter inspired by [Kilohearts' granular implementation](https://kilohearts.com/products/pitch_shifter). Uses vanilla OLA, artifacts and all, for resampling.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`PITCH_SHIFT`** — Pitch shift in semitones `[-24.0, 24.0]`
2. **`GRAIN_SIZE`** — Grain length in ms `[20.0, 500.0]`
    - Smaller values may introduce / emphasize inharmonic pitch content
    - Larger values work well for large shifts but have audible windowing artifacts
3. **`GRAIN_OVERLAP`** — Fraction of grain lengths which overlap `[0.25, 0.75]`
    - Smaller values may sound choppy and discontinuous
    - Larger values may improve pitch quality but also smear transients
4. **`JITTER`** — Amount of randomness to add to the pitch `[0.0, 1.0]`
    - Produces a unison-like effect 

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

## `Scrubby`

**Description:**  
Audio buffer scrubber; playback by zipping around to random points within a continuously running audio buffer. Based on [dFX Scrubby](http://destroyfx.org/docs/scrubby.html).

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`SEEK_RATE_LOW`** — Low bound of the random seek rate in Hz `[0.3, 810.0]`
2. **`SEEK_RATE_HIGH`** — High bound of the random seek rate in Hz `[0.3, 810.0]`
3. **`SEEK_RANGE`** — Maximum distance the scrubber can jump back into the buffer in ms `[0.3, 6000.0]`
4. **`SEEK_DUR_LOW`** — Low bound of the duration of the seek jump (as a ratio of the current seek interval) `[0.03, 1.0]`
5. **`SEEK_DUR_HIGH`** — High bound of the duration of the seek jump (as a ratio of the current seek interval) `[0.03, 1.0]`
6. **`OCTAVES_DOWN`** — Maximum pitch shift downward during a seek in octaves `[-4, 0]`
7. **`OCTAVES_UP`** — Maximum pitch shift upward during a seek in octaves `[0, 8]`

---

## `SpectralGate`

**Description:**  
Spectral-domain gate that removes frequency bins below a certain magnitude threshold. Inherits from `Phase_Vocoder`.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`FFT_SIZE`** — Size of FFT window (power of 2) `[128, FFT_MAX_SIZE]`
2. **`THRESHOLD`** — Magnitude threshold in dB; bins below this are gated `[-100.0, 0.0]`
3. **`TILT`** — Gate tilt bias (positive = more low-end gating, negative = more high-end gating) `[-1.0, 1.0]`
4. **`INVERT`** — Inverts gate logic `true / false`

---

## `Vocoder`

**Description:**  
Rough and ready vocoder effect. Only supports "self-modulation"; that is, the carrier is a phase-shifted version of the modulator (input). Supplies some gain compensation, but not reliably so. Loosely inspired by [IL Vocodex](https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/plugins/Vocodex.htm) and Ableton's Vocoder.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`N_BANDS`** — Number of bands `[4, 20]`
2. **`LOW_FREQ`** — Frequency of the lowest filter in the filterbank in Hz `[10.0, 16000.0]`
3. **`HIGH_FREQ`** — Frequency of the highest filter in the filterbank in Hz `[10.0, 16000.0]`
4. **`BANDWIDTH`** — Filterbank bandwidth factor; lower values result in narrower bands and vice versa `[0.05, 4.0]`
5. **`DEPTH`** — How much the spectral envelope of the modulator is imparted onto the carrier `[0.0, 2.0]`
6. **`ATTACK_TIME`** — Attack time in ms `[10.0, 1000.0]`
7. **`RELEASE_TIME`** — Release time in ms `[10.0, 2000.0]`

---

## `Wah`

**Description:**  
The classic Wah-Wah effect implemented with a resonant band-pass biquad. Automatically controlled by an envelope follower.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`MIN_FREQ`** — BPF min frequency in Hz `[20.0, 1000.0]`
2. **`MAX_FREQ`** — BPF max frequency in Hz `[1000.0, 8000.0]`
3. **`Q`** — BPF quality factor `[0.3, 6.0]`
[Back to index](../index.md)

# Utilities

This document describes the functionality and parameter structure of all utility functions and classes implemented in `Utilities.h`. These provide fundamental DSP building blocks prolific in this codebase.

---

## Functions

### `dbAmp`
**Description:**  
Converts decibel values to linear amplitude.

**Parameters:**
- **`dB`** — Decibel value `[-∞, ∞]`

**Returns:** Linear amplitude value

---

### `ampDB`
**Description:**  
Converts linear amplitude to decibel (dB) values.

**Parameters:**
- **`amp`** — Linear amplitude `[0.0, ∞]`

**Returns:** dB value

---

### `uniform`
**Description:**  
Generates a uniformly distributed random float.

**Returns:** Random float in range `[-1.0, 1.0]`

---

### `getEnvelopeValue`
**Description:**  
Computes [envelope/window function](https://www.desmos.com/calculator/j7vhnwaylq) values for various shapes.

**Parameters:**
- **`t`** — Normalized time position `[0.0, 1.0]`
- **`type`** — Envelope shape (`EnvelopeType` enum)

**Envelope Types:**
- **`HANN`** — Hann window: `0.5 × (1 - cos(2πt))`
- **`HAMMING`** — Hamming window: `0.54 - 0.46 × cos(2πt)`
- **`SINE`** — Sine window: `sin(πt)`
- **`TRI`** — Triangular window: `1 - |2t - 1|`
- **`PERC`** — Percussive envelope with 3% attack, exponential decay
- **`SMOOTH_RECT`** — Smooth rectangle with 5% fade in/out

**Returns:** Envelope amplitude at position `t`

---

### `lerp` (Buffer)
**Description:**  
Linearly interpolates between buffer samples for fractional indexing.

**Parameters:**
- **`buffer`** — Input buffer (any indexable container)
- **`index`** — Fractional index position
- **`size`** — Buffer size

**Returns:** Interpolated value

---

### `lerp` (Scalar)
**Description:**  
Linearly interpolates between two scalar values.

**Parameters:**
- **`a`** — Start value
- **`b`** — End value
- **`t`** — Interpolation factor `[0.0, 1.0]`

**Returns:** Interpolated value: `a + t(b - a)`

---

### `scale`
**Description:**  
Maps a scalar from an input range to an output range.

**Parameters:**
- **`x`** — Input to scale
- **`inLow`** — Low bound of input range
- **`inHigh`** — High bound of input range
- **`outLow`** — Low bound of output range
- **`outHigh`** — High bound of output range
- **`exponent`** — Exponential mapping factor, defaults to `1.0`

**Returns:** Rescaled value

---

### `dryWetMix`
**Description:**  
Mixes dry and wet signals with optional equal-power crossfading.

**Parameters:**
- **`dry`** — Dry (unprocessed) signal
- **`wet`** — Wet (processed) signal
- **`mix`** — Mix amount `[0.0, 1.0]`
- **`lin`** — Whether to use linear mixing or equal-power crossfade `true / false`

**Returns:** Mixed output signal

**Formulas:**
- Linear: `lerp(dry, wet, mix)`
- Equal-power: `dry × cos(mix × π/2) + wet × sin(mix × π/2)`

---

### `overlapAdd`
**Description:**  
Adds a frame to a target buffer with envelope windowing, wrapping around for circular buffers.

**Parameters:**
- **`target`** — Target buffer to add into (modified in-place)
- **`frame`** — Source frame to add
- **`type`** — Envelope shape for windowing (`EnvelopeType` enum)
- **`startPos`** — Starting position in target buffer

---

## Classes

### `DelayLine`

**Description:**  
Circular buffer implementing `z^-N` delay with fractional delay support via linear interpolation.

**Constructor:**
- **`delayTime`** — Initial delay time in ms
- **`maxDelayTime`** — Maximum delay time in ms (buffer size)

**Methods:**

**`setDelayTime(delayTime)`**
- Sets delay time in milliseconds `[0.0, maxDelayTime]`

**`setDelaySamples(delaySamples)`**
- Sets delay in samples directly

**`getSize()`**
- Returns buffer size in samples

**`read(offset)`**
- Reads from delay line with optional offset
- **`offset`** — Read offset in samples (default: `-1.0` uses current delay time)
- Returns delayed sample (with interpolation for fractional delays)

**`write(in)`**
- Writes sample to delay line
- **`in`** — Input sample

---

### `FFT`

**Description:**  
Fast Fourier Transform using [CMSIS-DSP](https://arm-software.github.io/CMSIS_5/DSP/html/index.html).

**Constructor:**
- **`fftSize`** — FFT size (default: `512`, must be a power of 2) `[128, FFT_MAX_SIZE]`

**Methods:**

**`setFFTSize(N)`**
- Sets the FFT size and reinitializes the internal FFT instance
- **`N`** — FFT length, must be power of 2 `[128, FFT_MAX_SIZE]`

**`forward(in, out)`**
- Performs a **forward real FFT**. Produces frequency-domain complex data in `fft_cpx`.
- **`in`** — Pointer to input time-domain sample buffer
- **`out`** — Pointer to complex frequency-domain buffer (`fft_cpx`)

**`inverse(in, out)`**
- Performs an **inverse real FFT**. Produces real-valued time-domain output.
- **`in`** — Pointer to complex frequency-domain buffer (`fft_cpx`)
- **`out`** — Pointer to output time-domain sample buffer

---

### `STFT`

**Description:**  
Short-Time Fourier Transform implementation with spectrogram storage. Provides streaming FFT analysis/synthesis with configurable overlap-add processing and frame interpolation capabilities.

**Constructor Parameters:**
- **`fftSize`** — FFT window size (must be power of 2) `[128, FFT_MAX_SIZE]`
- **`hopFactor`** — Hop size divisor (`hopSize = fftSize / hopFactor`) `[2, 8]`
- **`bufDur`** — Spectrogram buffer duration in seconds

**Methods:**

**`setFFTSize(N)`**
- Sets FFT size and reinitializes buffers `[128, FFT_MAX_SIZE]`

**`setHopSize(hopFactor)`**
- Sets hop size as fraction of FFT size `[2, 8]`
- Higher values = more overlap

**`setProcessCallback(callback)`**
- Registers callback function to process FFT frames
- **`callback`** — Function taking `FFTFrame&` parameter

**`forward(input)`**
- Processes single input sample, generates FFT frames at hop intervals
- **`input`** — Input sample
- Automatically windows, transforms, and stores frames in spectrogram

**`inverse()`**
- Reconstructs time-domain signal from spectrogram
- Returns: Output sample (with overlap-add reconstruction)

**`interpolateFrame(framePos)`**
- Interpolates between spectrogram frames for smooth time-stretching
- **`framePos`** — Fractional frame index
- Returns: Interpolated `FFTFrame` with lerped real and imaginary components


**`getSpectSize()`** — Returns number of frames in spectrogram buffer
**`getFFTSize()`** — Returns current FFT size
**`getHopSize()`** — Returns hop size in samples
**`getNumBins()`** — Returns number of frequency bins (fftSize/2 + 1)
**`getFFT()`** — Returns reference to internal FFT object
**`getFrame()`** — Returns reference to most recent FFT frame

**Notes:**
- Uses circular buffers for continuous streaming
- Applies Hann windowing for analysis and synthesis
- Maintains processing queue for accurate overlap-add timing
- Spectrogram size determined by buffer duration and hop rate
- **Very computationally and memory intensive**

---
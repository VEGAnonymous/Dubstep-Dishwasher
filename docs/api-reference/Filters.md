[Back to index](../index.md)

# Filters

This document describes the functionality and parameter structure of all available filters. All filters (except for `FIR_Filter` which inherits directly from `Effect`) currently inherit from the `IIR_Filter` abstract base class or its subclass `Biquad`.

---

## `FIR_Filter`

**Description:**  
Finite Impulse Response filter implementing convolution with a provided kernel. Uses efficient double-buffered circular state for real-time processing. Common-use filter kernels are stored in `LUTs.h`.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`h`** — Filter kernel (impulse response coefficients) *stored in PROGMEM array*
2. **`M`** — Filter order

**Notes:**
- Convolution implemented via inner product over kernel and double-buffered state
- Optimized implementation based on [CCRMA benchmarks](https://ccrma.stanford.edu/~jatin/Notebooks/FIRBenchmarks.html)

---

## `One-Pole`

**Description:**  
First-order IIR low-pass filter. Simple, efficient single-pole design with exponential frequency response.

**Parameters:**

0. **`MIX`** — Dry/wet balance `[0.0, 1.0]`
1. **`CUTOFF`** — Cutoff frequency in Hz `[0.0, SAMPLE_RATE/2]`
2. **`COEFF`** — Direct coefficient control `a` (alternative to cutoff)

**Transfer Function:**
```
H(z) = (1-a) / (1 - az^-1)
y[n] = (1-a)x[n] + ay[n-1]
```

**Variable Relationships:**
- `a = exp(-2π × cutoff / SAMPLE_RATE)`
- `cutoff = -(SAMPLE_RATE × ln(a)) / (2π)`

---

## `APF`

**Description:**  
First-order all-pass filter available in Direct Form I or Direct Form II implementations. Provides phase shift without affecting magnitude response. Supports delay-based operation and optional phase inversion.

**Parameters:**

0. **MIX** — Dry/wet balance `[0.0, 1.0]`
1. **CUTOFF** — Center frequency in Hz (translates to delay N)
2. **Q** — Quality factor (translates to coefficient g) `[0.0, ∞]`

**Constructor:**

- **`invert`** — Phase inversion flag `true / false`
- **`maxDelayTime`** — Maximum delay time in ms (determines buffer size)
- **`useDFII`** — Whether to implement Direct Form II instead of Direct Form I `true / false`
- **`usePSRAM`** — Whether to use PSRAM buffers `true / false`

**Transfer Functions:**

*Direct Form I:*
```
y[n] = -gy[n-N] + gx[n] + x[n-N]
```

*Direct Form II:*
```
v[n] = (1-g²)x[n] - gw[n-N]
y[n] = gx[n] + w[n-N]
```

**Variable Relationships:**

- `N = SAMPLE_RATE / (2 × cutoff)` (delay in samples)
- `g = 1 - (1/Q)`, `[-0.999, 0.999]`

**Methods:**

**`setDelay(N)`** — Set delay directly in samples
**`setInvert(invert)`** — Toggle phase inversion
**`readTap(offset)`** — Tap output delay line (Direct Form I only)

---

## Biquad Filters

All biquad filters are second-order IIR filters implementing the standard biquad LCCDE via Direct Form II-Transpose. Coefficient calculations are derived from the [Audio EQ Cookbook](https://www.w3.org/TR/audio-eq-cookbook/).

**Common Parameters:**
- **`CUTOFF`** — Cutoff/center frequency in Hz `[20.0, 20000.0]`
- **`Q`** — Quality factor `[0.02, 40.0]`
- **`GAIN`** — Gain in dB `[-24.0, 24.0]` (for shelving and peak filters)

**Transfer Function:**
```
H(z) = (b0 + b1z^-1 + b2z^-2) / (1 + a1z^-1 + a2z^-2)
w[n] = x[n] - a1z1 - a2z2
y[n] = b0w[n] + b1z1 + b2z2
z2 = z1, z1 = w[n]
```

---

### `LPF_Biquad`

**Description:**  
Second-order low-pass filter. Attenuates frequencies above the cutoff.

**Parameters:**
- **`CUTOFF`** — -3dB cutoff frequency in Hz `[20.0, 20000.0]`
- **`Q`** — Filter resonance `[0.02, 40.0]` (default: `0.707` for Butterworth response)

---

### `HPF_Biquad`

**Description:**  
Second-order high-pass filter. Attenuates frequencies below the cutoff.

**Parameters:**
- **`CUTOFF`** — -3dB cutoff frequency in Hz `[20.0, 20000.0]`
- **`Q`** — Filter resonance `[0.02, 40.0]` (default: `0.707` for Butterworth response)

---

### `BPF_Biquad`

**Description:**  
Second-order band-pass filter. Attenuates frequencies around the center frequency band.

**Constructor:**
- ...
- **`flatGain`** — Whether to use 0 dB constant passband gain *or* constant skirt gain with peak gain set by `Q` `true / false`

**Parameters:**
- **`CUTOFF`** — Center frequency in Hz `[20.0, 20000.0]`
- **`Q`** — Bandwidth / peak gain `[0.02, 40.0]`

---

### `LowShelf_Biquad`

**Description:**  
Shelving filter that boosts or attenuates frequencies below the cutoff. Automatically bypasses when gain is near 0 dB.

**Parameters:**
- **`CUTOFF`** — Shelf transition frequency in Hz `[20.0, 20000.0]`
- **`Q`** — Shelf slope `[0.02, 40.0]`
- **`GAIN`** — Shelf gain in dB `[-24.0, 24.0]`

---

### `HighShelf_Biquad`

**Description:**  
Shelving filter that boosts or cuts frequencies above the cutoff. Automatically bypasses when gain is near 0 dB.

**Parameters:**
- **`CUTOFF`** — Shelf transition frequency in Hz `[20.0, 20000.0]`
- **`Q`** — Shelf slope `[0.02, 40.0]`
- **`GAIN`** — Shelf gain in dB `[-24.0, 24.0]`

---

### `Peak_Biquad`

**Description:**  
Parametric peak/bell filter that boosts or cuts a narrow band of frequencies around the center frequency. Automatically bypasses when gain is near 0 dB.

**Parameters:**
- **`CUTOFF`** — Center frequency in Hz `[20.0, 20000.0]`
- **`Q`** — Bandwidth (higher = narrower band) `[0.02, 40.0]`
- **`GAIN`** — Peak gain in dB `[-24.0, 24.0]`

---

### `Notch_Biquad`

**Description:**  
Notch/band-reject filter that cuts a narrow band of frequencies around the center frequency.

**Parameters:**
- **`CUTOFF`** — Center frequency to reject in Hz `[20.0, 20000.0]`
- **`Q`** — Notch width (higher Q = narrower notch) `[0.02, 40.0]`
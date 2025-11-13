[Back to index](../index.md)

# Generators

This document describes the functionality and parameter structure of all available generators. All generators inherit from the `Generator` abstract base class. See `Defines.h` for enum definitions and `LUTs.h` for wavetable lookup tables.

---

## `Wavetable`

**Description:**  
Band-limited wavetable oscillator using fixed-point phase accumulation for efficient and accurate waveform generation.

**Parameters:**

0. **`FREQ`** — Oscillator frequency in Hz `[0.0, SAMPLE_RATE/2]`
1. **`TABLE`** — Wavetable type (`WavetableType` enum)

**Notes:**
- Uses 11-bit tables (2048 samples) stored in PROGMEM
- Fixed-point phase accumulator (32-bit) for sub-sample precision
- Linear indexing (no interpolation)
- Output range is `[-1.0, 1.0]` for all modes

---

## `Random`

**Description:**  
Random signal generator with multiple noise algorithms. Produces pseudo-random values at a specified rate.

**Parameters:**

0. **`FREQ`** — Update/transition rate in Hz `[0.0, SAMPLE_RATE]`
1. **`MODE`** — Random algorithm type (`RandomMode` enum)

**Modes:**

- **`PERLIN`** — Smooth interpolated noise using smoothstep function `(3x² - 2x³)` for natural transitions between random values
- **`SAMPLE_HOLD`** — Stepped random values that hold until the next update
- **`BINARY`** — Random binary signal alternating between `-1.0` and `+1.0`

**Notes:** Output range is `[-1.0, 1.0]` for all modes
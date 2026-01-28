[Back to index](../index.md)

# Control

This document describes the functionality and structure of the control classes. These classes manage audio effect chains and integration with the [Teensy Audio Library](https://www.pjrc.com/teensy/td_libs_Audio.html).

---

### `AudioChain`

**Description:**  
Manages a dynamic chain of audio effects with support for adding, removing, reordering, and processing effects in series. Effects are identified by unique IDs and can be bypassed individually.

**Methods:**

**`addEffect(EffectName id)`**

- Adds a new effect to the end of the chain
- **`id`** — Effect type to instantiate (`EffectName` enum)

**`removeEffect(EffectID id)`**

- Removes an effect from the chain
- **`id`** — `EffectID` of effect to remove

**`getEffect(EffectID id)`**
- Retrieves a pointer to an effect by its ID
- **`id`** — `EffectID` of effect to get
- Returns a pointer to `Effect` object, or `nullptr` if not found

**`reorderEffect(EffectID id, size_t pos)`**
- Changes the position of an effect in the processing chain.
- **`id`** — `EffectID` of effect to move
- **`pos`** — Position index to move to

**`processChain(float* input, float* output, size_t n)`**
- Processes an audio buffer serially through the entire effect chain.
- **`input`** — Input buffer
- **`output`** — Output buffer
- **`n`** — Number of samples to process (default: `BUFFER_SIZE`)

---

## `AudioChainStream`

**Description:**  
Teensy Audio Library wrapper that integrates `AudioChain` and `ModulationEngine` into the Audio Library's block-based processing system. Handles sample format conversion between int16 and float32. Extends `AudioStream` from Teensy Audio Library.

**Constructor:**
- **`chain`** — Reference to `AudioChain` object to process

**Methods:**
- **`getModEngine`** - Returns the internal `ModulationEngine` instance

**Notes:**
- Converts int16 input to float32 for processing, then back
- Processes one block of `AUDIO_BLOCK_SAMPLES` per `update()` call
- Applies any modulations 
- Calls to `update()` automatically handled by Teensy Audio Library

**Example Usage:**

```
/* IO */
AudioInputI2S adcIn; // ADC input
AudioOutputI2S dacOut; // DAC output

/* DSP */
std::unique_ptr<AudioChain> chain; // FX chain
std::unique_ptr<AudioChainStream> stream; // Audio stream
std::unique_ptr<AudioConnection> patch1, patch2; // Connections

...

void setup() {

/* Instantiate DSP chain */
chain = std::make_unique<AudioChain>();
stream = std::make_unique<AudioChainStream>(*chain);

patch1 = std::make_unique<AudioConnection>(adcIn, 0, *stream, 0);
patch2 = std::make_unique<AudioConnection>(*stream, 0, dacOut, 0);

...

}
```

---

## `LogMelStream`

**Description:**  
Teensy Audio Library wrapper that computes a running log-mel spectrogram of the upstream using `Log_Mel`. Extends `AudioStream` from Teensy Audio Library.

**Notes:**
- Converts int16 input to float32 for processing
- Processes one block of `AUDIO_BLOCK_SAMPLES` per `update()` call
- Calls to `update()` automatically handled by Teensy Audio Library

----

## `ModulationEngine`

**Description:**  
Manages a dynamic modulation system that allows for control-rate modulation of `AudioChain` `Effect` parameters with an arbitrary number of many-to-one mappings. Instantiates 12 `Modulator` instances: the first six are LFOs and the latter six are mappings.

**Constructor:**
- **`chain`** — Reference to `AudioChain` object; effect and parameter ids will refer to this instance.

**Methods:**

**`getModulator(id)`**
- Retrieve a `Modulator` from the engine.
- **`id`** — Numeric `ModulatorID` id of the `Modulator` to retrieve
- Returns a pointer to the `Modulator` instance.

**`addAssignment(assignment)`**
- Adds a modulation assignment from a `Modulator` to an `Effect` parameter.
- **`assignment`** — An instance of `ModAssignment`, defined as:
```
struct ModAssignment {
    ModulatorID modId;
    EffectID effectId;
    ParamID paramId;
    float amount; // [0.0, 1.0]
    ModPolarity polarity;
    ...
}
```

**`removeAssignment(modId, effectId, paramId)`**
- Removes a modulation assignment. Restores the base value if the removed assignment was the only one remaining on the given effect parameter.
- **`modId`** — Numeric `ModulatorID` id of the `Modulator`
- **`effectId`** — Numeric `EffectID` id of the `Effect`
- **`paramId`** — Numeric `ParamID` id of the effect parameter

**`removeEffect(effectId)`**
- Removes all stored assignments and base values for an effect.
- **`effectId`** — Numeric `EffectID` id of the `Effect`

**`setAssignment(modId, effectId, paramId, amount, polarity)`**
- Sets the amount and/or polarity for a modulation assignment. If the assignment does not exist, one is created via `addAssignment`.
- **`modId`** — Numeric `ModulatorID` id of the `Modulator`
- **`effectId`** — Numeric `EffectID` id of the `Effect`
- **`paramId`** — Numeric `ParamID` id of the effect parameter
- **`amount`** — Depth of modulation `[0.0, 1.0]`
- **`polarity`** — Whether the modulation should be unipolar/bipolar [`ModPolarity` enum]
    - Unipolar ranges `[0, 1]` from the normalized base value
    - Bipolar ranges `[-0.5, 0.5]` from the normalized base value

**`clearAssignments()`**
- Clears all modulation assignments and resets modulation output.

**`setBaseValue(effectId, paramId, normalized)`**
- Sets the base value (i.e., normalized value) for an `Effect` parameter that modulation assignments to deviate from. This should be called alongside calls to `setParam()` in `AudioChain`.
- **`effectId`** — Numeric `EffectID` id of the `Effect`
- **`paramId`** — Numeric `ParamID` id of the effect parameter
- **`normalized`** — Normalized base value for the parameter

**`update(dt)`**
- Advances phase, accumulates output, then applies offsets for all LFO/Random modulators and their respective parameter assignments.
- **`dt`** — Real time between calls to update() in seconds, typically `AUDIO_BLOCK_SAMPLES / SAMPLE_RATE` for control rate processing.

----

## `ParameterRegistry`

**Description:**  
Utilities for bridging between normalized and actual effect parameter values. Specifically, allows mapping normalized ranges `[0.0, 1.0]` to actual parameter ranges defined in `createParameterRegistry()` and back. A single global instance of `ParameterMap` is provided for this. 

```
struct ParameterRange {
    ParamUnit unit;
    float min, max, exponent;
    float initialValue;
    
    float fromNormalized(float norm);
    float toNormalized(float actual) const;
}

using ParameterMap = std::map<EffectName, std::map<ParamID, ParameterRange>>; // Map: EffectName -> ParamID -> Range
...
static ParameterMap parameterRegistry = createParameterRegistry(); // Global instance
```

**Methods:**

**`getParameterRange(effect, param)`**
- Searches `parameterRegistry` for a given effect parameter's range
- **`effect`** — `EffectName` of the effect
- **`param`** — Numeric `ParamID` of the effect parameter to find the range of
- Returns a pointer to the `ParameterRange` if found, `nullptr` if not
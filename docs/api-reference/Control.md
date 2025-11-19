[Back to index](../index.md)

# Control

This document describes the functionality and structure of the control classes. These classes manage audio effect chains and integration with the [Teensy Audio Library](https://www.pjrc.com/teensy/td_libs_Audio.html).

---

### `AudioChain`

**Description:**  
Manages a dynamic chain of audio effects with support for adding, removing, reordering, and processing effects in series. Effects are identified by unique IDs and can be bypassed individually.

#### Public Methods

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
Teensy Audio Library wrapper that integrates `AudioChain` into the Audio Library's block-based processing system. Handles sample format conversion between int16 and float32. Extends `AudioStream` from Teensy Audio Library.

**Constructor:**
- **`chain`** — Reference to `AudioChain` object to process

**Notes:**
- Converts int16 input to float32 for processing, then back
- Processes one block of `AUDIO_BLOCK_SAMPLES` per `update()` call
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
- *For this class to be practically useful, a callback function must be set*
- Converts int16 input to float32 for processing
- Processes one block of `AUDIO_BLOCK_SAMPLES` per `update()` call
- Calls to `update()` automatically handled by Teensy Audio Library

**Example Usage:**

```
/* IO */
AudioInputI2S adcIn; // ADC input

/* DSP */
std::unique_ptr<LogMelStream> logMelStream; // Log-mel stream
std::unique_ptr<AudioConnection> patch1; // Connection

...

void setup() {

logMelStream = std::make_unique<LogMelStream>();
// Set callback - send frames over UART via Handler when ready
logMelStream->setMelCallback([&](const float* melEnergies, size_t numMels) {
    MelFrame frame{};
    frame.index = frameCounter++;
    frame.numMels = numMels;
    memcpy(frame.mel, melEnergies, numMels * sizeof(float));
    handler.send(frame);
});

patch1 = std::make_unique<AudioConnection>(adcIn, 0, *logMelStream, 0);

...

}
```
#ifndef DEFINES
#define DEFINES

/* DEFINES */

#include <Audio.h>

#define SAMPLE_RATE AUDIO_SAMPLE_RATE_EXACT
#define BUFFER_SIZE AUDIO_BLOCK_SAMPLES
#define FFT_MAX_SIZE 1024.0f

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// Datatypes
using EffectID = uint8_t;
using ParamID = uint8_t;

/* ENUMS */

enum class CommandType : uint8_t { ADD, REMOVE, REORDER, SET, BYPASS };

enum class EffectName : uint8_t {
    DISTORTION,
    DELAY,
    FLANGER,
    PHASER,
    CHORUS,
    REVERB,
    COMPRESSOR,
    EQUALIZER,
    GRANULATOR,
    FREEZER,
    SPECTRAL_GATE,
    LIMITER
};

enum class EnvelopeType : uint8_t { HANN, HAMMING, SINE, TRI, PERC, SMOOTH_RECT };
enum class WavetableType : uint8_t { SINE, TRI, SAW, SQUARE };
enum class RandomMode : uint8_t { PERLIN, SAMPLE_HOLD, BINARY };
enum class DistortionMode : uint8_t { TUBE, SOFT_CLIP, HARD_CLIP, DIODE, BITCRUSH, RECTIFY, SATURATE };
enum class BiquadType : uint8_t { LOW_PASS, HIGH_PASS, LOW_SHELF, HIGH_SHELF, PEAK, NOTCH };

#endif // DEFINES
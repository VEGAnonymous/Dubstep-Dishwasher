#pragma once

#include <Arduino.h>
#include <cstdint>
#include <vector>

/* DEFINES */

#define SAMPLE_RATE 44100.0f
#define BUFFER_SIZE 128
#define FFT_MAX_SIZE 2048.0f

#ifndef M_PI
    #define M_PI 3.14159265358979323846
    #define M_PI_2 1.57079632679489661923
#endif

// Log-mel spectrogram
#define NUM_BINS 257
#define NUM_MELS 64

/* TYPES */
using EffectID = uint8_t;
using ModulatorID = uint8_t;
using ParamID = uint8_t;

struct fft_cpx { float r; float i; };

/* ENUMS */

enum class CommandType : uint8_t { 
    EFFECT_ADD, 
    EFFECT_REMOVE, 
    EFFECT_REORDER, 
    EFFECT_SET_PARAMETER, 
    EFFECT_BYPASS, 
    EFFECT_CLEAR,
    MOD_SET_PARAMETER,
    MOD_CLEAR_CURVE,
    MOD_SET_CURVE_POINT,
    MOD_ASSIGNMENT_ADD,
    MOD_ASSIGNMENT_REMOVE,
    MOD_ASSIGNMENT_SET,
    MOD_MAPPING_SET_INPUT
};

// Effects
enum class EffectName : uint8_t {
    CHORUS,
    COMPRESSOR,
    DELAY,
    DISTORTION,
    EQUALIZER,
    FLANGER,
    FORMANT_SHIFTER,
    FREEZER,
    GAIN,
    GATE,
    GRANULATOR,
    MODULATION,
    PARALLEL,
    PHASER,
    PITCH_SHIFTER,
    REVERB,
    SCRUBBY,
    SPECTRAL_GATE,
    VOCODER,
    WAH,
    LIMITER
};

enum class EnvelopeType : uint8_t { HANN, HAMMING, SINE, TRI, PERC, SMOOTH_RECT };

enum class ParallelMode : uint8_t { SUM, CROSSFADE };

enum class ModulationEffectMode : uint8_t { AM, RM };
enum class DistortionMode : uint8_t { TUBE, SOFT_CLIP, HARD_CLIP, DIODE, BITCRUSH, RECTIFY, SATURATE };
enum class BiquadType : uint8_t { LOW_PASS, HIGH_PASS, LOW_SHELF, HIGH_SHELF, PEAK, NOTCH };

// Modulators
enum class ModulatorType : uint8_t { LFO_CURVE, LFO_RANDOM, MAPPING };
enum class ModPolarity : uint8_t {UNIPOLAR, BIPOLAR};

// Generators
enum class WavetableType : uint8_t { SINE, TRI, SAW, SQUARE };
enum class RandomMode : uint8_t { PERLIN, SAMPLE_HOLD, BINARY }; // Also a Modulator

/* STRUCTS */
struct CurvePoint {
    float x;
    float y;
    float curve; // exp
    
    CurvePoint(float x = 0.0f, float y = 0.0f, float curve = 0.0f) : x(x), y(y), curve(curve) {}
};

struct ModAssignment {
    ModulatorID modId;
    EffectID effectId;
    ParamID paramId;
    float amount; // [0.0, 1.0]
    ModPolarity polarity;
    
    ModAssignment(ModulatorID modId, EffectID effectId, ParamID paramId, 
                  float amount, ModPolarity polarity = ModPolarity::BIPOLAR)
        : modId(modId), effectId(effectId), paramId(paramId), 
          amount(amount), polarity(polarity) {}
};

struct ParamKey { // Parameter key for tracking base values
    EffectID effectId;
    ParamID paramId;
    
    bool operator<(const ParamKey& other) const {
        if (effectId != other.effectId) return effectId < other.effectId;
        return paramId < other.paramId;
    }
    bool operator==(const ParamKey& other) const {
        return effectId == other.effectId && paramId == other.paramId;
    }
};
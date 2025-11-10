#pragma once

#include <Arduino.h>

/* DEFINES */

#define SAMPLE_RATE 44100.0f
#define BUFFER_SIZE 128
#define FFT_MAX_SIZE 2048.0f

#ifndef M_PI
    #define M_PI 3.14159265358979323846
    #define M_PI_2 1.57079632679489661923
#endif

// Datatypes
using EffectID = uint8_t;
using ParamID = uint8_t;

struct fft_cpx { float r; float i; };

/* ENUMS */

enum class CommandType : uint8_t { ADD, REMOVE, REORDER, SET, BYPASS };

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

enum class WavetableType : uint8_t { SINE, TRI, SAW, SQUARE };
enum class RandomMode : uint8_t { PERLIN, SAMPLE_HOLD, BINARY };

enum class ParallelMode : uint8_t { SUM, CROSSFADE };

enum class ModulationEffectMode : uint8_t { AM, RM };
enum class DistortionMode : uint8_t { TUBE, SOFT_CLIP, HARD_CLIP, DIODE, BITCRUSH, RECTIFY, SATURATE };
enum class BiquadType : uint8_t { LOW_PASS, HIGH_PASS, LOW_SHELF, HIGH_SHELF, PEAK, NOTCH };
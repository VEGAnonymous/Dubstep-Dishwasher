package lol.pony.dubstepdishwasher.model.core

/* DEFINES */
/* ENUMS */

enum class EffectType {
    DISTORTION,
    DELAY,
    FLANGER,
    PHASER,
    CHORUS,
    REVERB,
    COMPRESSOR,
    EQUALIZER,
    GRANULATOR,
    SPECTRAL_GATE
}

enum class EnvelopeType { HANN, HAMMING, SINE, TRI, PERC, SMOOTH_RECT }
enum class DistortionMode { TUBE, SOFT_CLIP, HARD_CLIP, DIODE, BITCRUSH, RECTIFY, SATURATE }
enum class BiquadType { LOW_PASS, HIGH_PASS, LOW_SHELF, HIGH_SHELF, PEAK, NOTCH }

enum class ParamUnit { PERCENT, ENUM, MS, HZ, DB, DIMENSIONLESS }
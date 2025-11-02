package lol.pony.dubstepdishwasher.model.core

/* DEFINES */
/* ENUMS */

enum class EffectType(val uiName: String) {
    DISTORTION("Distortion"),
    DELAY("Delay"),
    FLANGER("Flanger"),
    PHASER("Phaser"),
    CHORUS("Chorus"),
    REVERB("Reverb"),
    COMPRESSOR("Compressor"),
    EQUALIZER("Equalizer"),
    GRANULATOR("Granulator"),
    SPECTRAL_GATE("Spectral Gate")
}

enum class CommandType(val value: Byte) {
    ADD(0),
    REMOVE(1),
    REORDER(2),
    SET_PARAM(3),
    BYPASS(4)
}

enum class EnvelopeType { HANN, HAMMING, SINE, TRI, PERC, SMOOTH_RECT }
enum class DistortionMode { TUBE, SOFT_CLIP, HARD_CLIP, DIODE, BITCRUSH, RECTIFY, SATURATE }
enum class BiquadType { LOW_PASS, HIGH_PASS, LOW_SHELF, HIGH_SHELF, PEAK, NOTCH }

enum class ParamUnit { PERCENT, ENUM, MS, HZ, DB, DIMENSIONLESS }
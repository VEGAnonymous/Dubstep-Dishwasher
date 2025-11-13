package lol.pony.dubstepdishwasher.model.core

/* DEFINES */
/* ENUMS */

enum class EffectType(val uiName: String) {
    CHORUS("Chorus"),
    COMPRESSOR("Compressor"),
    DELAY("Delay"),
    DISTORTION("Distortion"),
    EQUALIZER("Equalizer"),
    FLANGER("Flanger"),
    FORMANT_SHIFTER("Formant Shifter"),
    FREEZER("Freezer"),
    GAIN("Gain"),
    GATE("Gate"),
    GRANULATOR("Granulator"),
    MODULATION("Modulation"),
    PARALLEL("Parallel"),
    PHASER("Phaser"),
    PITCH_SHIFTER("Pitch Shifter"),
    REVERB("Reverb"),
    SCRUBBY("Scrubby"),
    SPECTRAL_GATE("Spectral Gate"),
    VOCODER("Vocoder"),
    WAH("Wah")
}

enum class CommandType(val value: Byte) {
    ADD(0),
    REMOVE(1),
    REORDER(2),
    SET_PARAM(3),
    BYPASS(4)
}

enum class ParamUnit { PERCENT, ENUM, MS, HZ, DB, SEMITONES, DIMENSIONLESS }

enum class EnvelopeType { HANN, HAMMING, SINE, TRI, PERC, SMOOTH_RECT }

enum class WavetableType { SINE, TRI, SAW, SQUARE };
enum class RandomMode { PERLIN, SAMPLE_HOLD, BINARY };

enum class ParallelMode { SUM, CROSSFADE };

enum class ModulationEffectMode { AM, RM }
enum class DistortionMode { TUBE, SOFT_CLIP, HARD_CLIP, DIODE, BITCRUSH, RECTIFY, SATURATE }
enum class BiquadType { LOW_PASS, HIGH_PASS, LOW_SHELF, HIGH_SHELF, PEAK, NOTCH }
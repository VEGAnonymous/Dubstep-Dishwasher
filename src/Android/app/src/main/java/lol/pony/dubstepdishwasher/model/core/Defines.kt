package lol.pony.dubstepdishwasher.model.core

import androidx.compose.ui.tooling.preview.UiMode

/* DEFINES */
/* ENUMS */

enum class EffectType(override val uiName: String) : UIEnum {
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
    BYPASS(4),
    CLEAR(5)
}

enum class ParamUnit { PERCENT, ENUM, MS, HZ, DB, SEMITONES, DIMENSIONLESS }

enum class EnvelopeType(override val uiName: String) : UIEnum {
    HANN("Hann"),
    HAMMING("Hamming"),
    SINE("Sine"),
    TRI("Triangle"),
    PERC("Perc"),
    SMOOTH_RECT("Smooth Rect") }

enum class WavetableType(override val uiName: String) : UIEnum {
    SINE("Sine"),
    TRI("Triangle"),
    SAW("Saw"),
    SQUARE("Square")
}
enum class RandomMode(override val uiName: String) : UIEnum {
    PERLIN("Perlin"),
    SAMPLE_HOLD("Sample & Hold"),
    BINARY("Binary")
}

enum class ParallelMode(override val uiName: String) : UIEnum {
    SUM("Sum"),
    CROSSFADE("Crossfade")
}

enum class ModulationEffectMode(override val uiName: String) : UIEnum {
    AM("AM"),
    RM("RM")
}
enum class DistortionMode(override val uiName: String) : UIEnum {
    TUBE("Tube"),
    SOFT_CLIP("Soft Clip"),
    HARD_CLIP("Hard Clip"),
    DIODE("Diode"),
    BITCRUSH("Bitcrush"),
    RECTIFY("Rectify"),
    SATURATE("Saturate")
}
enum class BiquadType(override val uiName: String) : UIEnum {
    LOW_PASS("Low Pass"),
    HIGH_PASS("High Pass"),
    LOW_SHELF("Low Shelf"),
    HIGH_SHELF("High Shelf"),
    PEAK("Peak"),
    NOTCH("Notch")
}
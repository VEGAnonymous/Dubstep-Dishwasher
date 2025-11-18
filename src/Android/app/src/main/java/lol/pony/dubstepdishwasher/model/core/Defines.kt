package lol.pony.dubstepdishwasher.model.core

import androidx.compose.ui.tooling.preview.UiMode

/* DEFINES */
/* ENUMS */

enum class EffectType(override val uiName: String, val cpuUsage: Float?) : UIEnum {
    CHORUS("Chorus", 5.0f),
    COMPRESSOR("Compressor", 5.0f),
    DELAY("Delay", 2.0f),
    DISTORTION("Distortion", 2.0f),
    EQUALIZER("Equalizer", 1.0f),
    FLANGER("Flanger", 1.0f),
    FORMANT_SHIFTER("Formant Shifter", 19.0f),
    FREEZER("Freezer", 16.0f),
    GAIN("Gain", 0.5f),
    GATE("Gate", 4.0f),
    GRANULATOR("Granulator", 13.0f),
    MODULATION("Modulation", 1.0f),
    PARALLEL("Parallel", null),
    PHASER("Phaser", 8.0f),
    PITCH_SHIFTER("Pitch Shifter", 11.0f),
    REVERB("Reverb", 13.0f),
    SCRUBBY("Scrubby", 4.0f),
    SPECTRAL_GATE("Spectral Gate", 16.0f),
    VOCODER("Vocoder", null),
    WAH("Wah", 6.0f)
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
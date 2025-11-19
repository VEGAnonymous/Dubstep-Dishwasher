package lol.pony.dubstepdishwasher.model.core

/* DEFINES */

const val MAX_COMPUTE_USAGE = 0.90f // 90%
const val MAX_MEMORY_USAGE  = 450   // 400 kB
const val CONTROL_RATE = 20 // Hz

/* ENUMS */

data class ResourceUsage(
    val compute: Float, // Worst-case CPU usage, 0.0–1.0
    val memory: Int = 0 // Worst-case RAM usage in kB
)

enum class EffectType(
    override val uiName: String,
    val resourceUsage: ResourceUsage
) : UIEnum {
    CHORUS("Chorus", ResourceUsage(0.05f)),
    COMPRESSOR("Compressor", ResourceUsage(0.05f)),
    DELAY("Delay", ResourceUsage(0.02f)),
    DISTORTION("Distortion", ResourceUsage(0.02f)),
    EQUALIZER("Equalizer", ResourceUsage(0.01f)),
    FLANGER("Flanger", ResourceUsage(0.01f)),
    FORMANT_SHIFTER("Formant Shifter", ResourceUsage(0.19f, 30)),
    FREEZER("Freezer", ResourceUsage(0.16f, 200)),
    GAIN("Gain", ResourceUsage(0.005f)),
    GATE("Gate", ResourceUsage(0.04f)),
    GRANULATOR("Granulator", ResourceUsage(0.13f)),
    MODULATION("Modulation", ResourceUsage(0.01f)),
    PARALLEL("Parallel", ResourceUsage(0f)), // Will vary
    PHASER("Phaser", ResourceUsage(0.08f)),
    PITCH_SHIFTER("Pitch Shifter", ResourceUsage(0.11f)),
    REVERB("Reverb", ResourceUsage(0.13f, 150)),
    SCRUBBY("Scrubby", ResourceUsage(0.04f)),
    SPECTRAL_GATE("Spectral Gate", ResourceUsage(0.16f, 20)),
    VOCODER("Vocoder", ResourceUsage(0.22f)),
    WAH("Wah", ResourceUsage(0.06f))
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
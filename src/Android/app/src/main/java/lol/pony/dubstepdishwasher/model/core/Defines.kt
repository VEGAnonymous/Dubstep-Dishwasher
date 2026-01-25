package lol.pony.dubstepdishwasher.model.core

import kotlinx.serialization.Serializable

/* DEFINES */

const val SAMPLE_RATE = 44100.0
const val MAX_COMPUTE_USAGE = 0.90f // 90%
const val MAX_MEMORY_USAGE  = 450   // 450 kB
const val CONTROL_RATE = 20 // Hz
const val LFO_UPDATE_RATE = 100 // Hz

const val MAX_ACK_RETRIES = 3
const val ACK_TIMEOUT = 1000L // 1s
const val HEARTBEAT_TIMEOUT = 5000L // 3s

const val MAX_AWAIT_RETRIES = 10
const val AWAIT_TIMEOUT = 500L

/* ENUMS */

data class ResourceUsage(
    val compute: Float, // Worst-case CPU usage, 0.0–1.0
    val memory: Int = 0 // Worst-case RAM usage in kB
)

enum class EffectType(
    override val uiName: String,
    val resourceUsage: ResourceUsage
) : UIEnum {
    CHORUS("Chorus", ResourceUsage(0.05f, 1)),
    COMPRESSOR("Compressor", ResourceUsage(0.05f, 1)),
    DELAY("Delay", ResourceUsage(0.02f, 1)),
    DISTORTION("Distortion", ResourceUsage(0.02f, 1)),
    EQUALIZER("Equalizer", ResourceUsage(0.01f, 1)),
    FLANGER("Flanger", ResourceUsage(0.01f, 1)),
    FORMANT_SHIFTER("Formant Shifter", ResourceUsage(0.19f, 30)),
    FREEZER("Freezer", ResourceUsage(0.16f, 200)),
    GAIN("Gain", ResourceUsage(0.005f, 1)),
    GATE("Gate", ResourceUsage(0.04f, 1)),
    GRANULATOR("Granulator", ResourceUsage(0.13f, 1)),
    MODULATION("Modulation", ResourceUsage(0.01f, 1)),
    PARALLEL("Parallel", ResourceUsage(0.005f, 1)), // Will vary
    PHASER("Phaser", ResourceUsage(0.08f, 1)),
    PITCH_SHIFTER("Pitch Shifter", ResourceUsage(0.11f, 1)),
    REVERB("Reverb", ResourceUsage(0.13f, 150)),
    SCRUBBY("Scrubby", ResourceUsage(0.04f, 1)),
    SPECTRAL_GATE("Spectral Gate", ResourceUsage(0.16f, 20)),
    VOCODER("Vocoder", ResourceUsage(0.22f, 5)),
    WAH("Wah", ResourceUsage(0.06f, 1))
}

enum class ParamUnit { PERCENT, ENUM, MS, HZ, DB, SEMITONES, DIMENSIONLESS }

@Serializable
enum class EnvelopeType(override val uiName: String) : UIEnum {
    HANN("Hann"),
    HAMMING("Hamming"),
    SINE("Sine"),
    TRI("Triangle"),
    PERC("Perc"),
    SMOOTH_RECT("Smooth Rect") }
@Serializable
enum class WavetableType(override val uiName: String) : UIEnum {
    SINE("Sine"),
    TRI("Triangle"),
    SAW("Saw"),
    SQUARE("Square")
}
@Serializable
enum class ParallelMode(override val uiName: String) : UIEnum {
    SUM("Sum"),
    CROSSFADE("Crossfade")
}
@Serializable
enum class ModulationEffectMode(override val uiName: String) : UIEnum {
    AM("AM"),
    RM("RM")
}
@Serializable
enum class DistortionMode(override val uiName: String) : UIEnum {
    TUBE("Tube"),
    SOFT_CLIP("Soft Clip"),
    HARD_CLIP("Hard Clip"),
    DIODE("Diode"),
    BITCRUSH("Bitcrush"),
    RECTIFY("Rectify"),
    SATURATE("Saturate")
}
@Serializable
enum class BiquadType(override val uiName: String) : UIEnum {
    LOW_PASS("Low Pass"),
    HIGH_PASS("High Pass"),
    LOW_SHELF("Low Shelf"),
    HIGH_SHELF("High Shelf"),
    PEAK("Peak"),
    NOTCH("Notch")
}
@Serializable
enum class FFTSize(override val uiName: String, val value: Float) : UIEnum {
    SIZE_128("128", 128f),
    SIZE_256("256", 256f),
    SIZE_512("512", 512f),
    SIZE_1024("1024", 1024f)
}
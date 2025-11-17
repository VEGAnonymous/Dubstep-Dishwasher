package lol.pony.dubstepdishwasher.model.core

// TODO: GLOBAL PRESETS!

/* Curve presets */
data class CurvePreset (
    override val name: String,
    override val data: List<CurvePoint>
) : Preset<List<CurvePoint>>

fun defaultCurvePresets() = listOf(
    CurvePreset("Triangle", triCurve()),
    CurvePreset("Ramp", rampCurve())
)

// Default curves
fun triCurve(): List<CurvePoint> = listOf(
    CurvePoint(0f, 0.5f),
    CurvePoint(0.25f, 1f),
    CurvePoint(0.5f, 0.5f),
    CurvePoint(0.75f, 0f),
    CurvePoint(1f, 0.5f)
)

fun rampCurve(): List<CurvePoint> = listOf(
    CurvePoint(0f, 0f),
    CurvePoint(1f, 1f)
)
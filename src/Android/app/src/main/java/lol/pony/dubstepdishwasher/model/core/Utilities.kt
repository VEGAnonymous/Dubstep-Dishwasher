package lol.pony.dubstepdishwasher.model.core

import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.Dp
import java.math.BigDecimal
import java.math.RoundingMode
import kotlin.math.pow

/* UTILS */

fun lerp(a: Float, b: Float, t: Float): Float = a + (b - a) * t
fun lerp(a: Dp, b: Dp, t: Float): Dp = a + (b - a) * t // dp overload
fun lerpColor(a: Color, b: Color, t: Float): Color =
    Color(
        red = androidx.compose.ui.graphics.lerp(a, b, t).red,
        green = androidx.compose.ui.graphics.lerp(a, b, t).green,
        blue = androidx.compose.ui.graphics.lerp(a, b, t).blue,
        alpha = 1f
    )

fun Number.mapRange(inRange: ClosedRange<Float>, outRange: ClosedRange<Float>): Float {
    return ((this.toFloat() - inRange.start) / (inRange.endInclusive - inRange.start)) * (outRange.endInclusive - outRange.start) + outRange.start
}

fun applyCurve(t: Float, p0: CurvePoint): Float { // Exponential curve
    val curved = if (p0.curve == 0f) { t } // Linear
    else {
        val exp = 2f.pow(kotlin.math.abs(p0.curve) * 3f)
        if (p0.curve < 0f) 1f - (1f - t).pow(exp) // Ease-in
        else t.pow(exp) // Ease-out
    }
    return curved
}

fun snapValue(snapToGrid: Boolean = true, value: Float, divisions: Int = 8): Float {
    if (!snapToGrid || divisions <= 0) return value
    val step = 1f / divisions
    return (kotlin.math.round(value / step) * step).coerceIn(0f, 1f)
}

// Normalized mod offset to real value
fun modValue(effect: Effect, param: EffectParameter.Range, modOffsets: Map<ParamKey, Float>): Float {
    val key = ParamKey(effect.effectId, param.id)
    val offset = modOffsets[key] ?: 0f
    val baseNorm = param.normalized()
    val modNorm = (baseNorm + offset).coerceIn(0f, 1f)
    return param.normalizedTo(modNorm) // Real shit
}

fun formatParamValue(value: Double, unit: ParamUnit, step: Float, displayUnits: Boolean = true) : String {
    val decimalPlaces = when { // Determine rounding precision
        step >= 1.0f -> 0
        step >= 0.1f -> 1
        step >= 0.01f -> 2
        step >= 0.001f -> 3
        else -> 4
    }

    // Build unit string
    val (displayValue, displayDecimals, unitStr) =
        if (displayUnits) {
            when (unit) {
                ParamUnit.PERCENT -> Triple(value * 100.0, (decimalPlaces - 2).coerceAtLeast(0), "%")
                ParamUnit.MS -> {
                    val v = value
                    if (v >= 1000.0) Triple(v / 1000.0, (decimalPlaces + 3).coerceAtMost(4), " s")
                    else Triple(v, decimalPlaces, " ms")
                }
                ParamUnit.HZ -> Triple(value, decimalPlaces, " Hz")
                ParamUnit.DB -> Triple(value, decimalPlaces, " dB")
                ParamUnit.SEMITONES -> Triple(value, decimalPlaces, " st")
                ParamUnit.ENUM, ParamUnit.DIMENSIONLESS -> Triple(value, decimalPlaces, "")
            }
        } else Triple(value, decimalPlaces, "")

    val rounded = BigDecimal(displayValue).setScale(displayDecimals, RoundingMode.HALF_UP)
    return "$rounded$unitStr"
}
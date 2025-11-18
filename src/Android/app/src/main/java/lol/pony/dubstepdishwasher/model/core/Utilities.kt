package lol.pony.dubstepdishwasher.model.core

import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.Dp
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
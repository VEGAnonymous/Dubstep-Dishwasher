package lol.pony.dubstepdishwasher.model.core

import kotlin.math.pow

/* UTILS */

fun lerp(a: Float, b: Float, t: Float) = a + (b - a) * t

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
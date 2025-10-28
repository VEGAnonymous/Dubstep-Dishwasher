package lol.pony.dubstepdishwasher.model.core

/* UTILS */

fun Number.mapRange(inRange: ClosedRange<Float>, outRange: ClosedRange<Float>): Float {
    return ((this.toFloat() - inRange.start) / (inRange.endInclusive - inRange.start)) * (outRange.endInclusive - outRange.start) + outRange.start
}
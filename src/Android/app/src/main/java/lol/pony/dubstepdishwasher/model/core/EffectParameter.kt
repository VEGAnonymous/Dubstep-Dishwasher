package lol.pony.dubstepdishwasher.model.core

import kotlin.math.pow

sealed class EffectParameter<T> (
    open val id: Int,
    open val name: String
) {
    data class Range<T : Number>(
        override val id: Int,
        override val name: String,
        override val unit: ParamUnit,
        val range: Pair<T, T>,
        val exp: Float,
        val step: T,
        var value: T
    ) : EffectParameter<T>(id, name), NumericParam, NormalizableParam {
        override fun normalized() : Float {
            val lin = value.mapRange(range.first.toFloat()..range.second.toFloat(), 0.0f..1.0f)
            return lin.pow(1 / exp)
        }
        @Suppress("UNCHECKED_CAST")
        override fun fromNormalized(x: Float) {
            val lin = x.pow(exp)
            value = lin.mapRange(0.0f..1.0f, range.first.toFloat()..range.second.toFloat()) as T
        }
    }

    data class Discrete<T>(
        override val id: Int,
        override val name: String,
        override val unit: ParamUnit,
        val possibleValues: List<T>,
        var value: T
    ) : EffectParameter<T>(id, name), NumericParam

    data class Toggle(
        override val id: Int,
        override val name: String,
        var value: Boolean
    ) : EffectParameter<Boolean>(id, name)
}
package lol.pony.dubstepdishwasher.model.core

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import kotlin.math.pow

sealed class ModulatorParameter<T>(
    override val id: Int,
    override val name: String
) : Parameter<T> {
    abstract override var value: T
    abstract override val unit: ParamUnit

    data class Range<T : Number>(
        val modId: String,
        override val id: Int,
        override val name: String,
        override val unit: ParamUnit,
        val range: Pair<T, T>,
        val exp: Float,
        val step: T,
        val initialValue: T
    ) : ModulatorParameter<T>(id, name), NormalizableParam {
        override var value by mutableStateOf(initialValue)

        // Copy normalized/fromNormalized logic from EffectParameter.Range
        override fun normalized(): Float {
            val min = range.first.toFloat()
            val max = range.second.toFloat()
            val v = value.toFloat()
            val lin = if (min < 0f && max <= 0f) {
                val absV = -v
                val absMin = -max
                val absMax = -min
                1f - absV.mapRange(absMin..absMax, 0f..1f)
            } else v.mapRange(min..max, 0f..1f)
            return lin.pow(1f / exp)
        }

        @Suppress("UNCHECKED_CAST")
        override fun fromNormalized(x: Float) {
            val min = range.first.toFloat(); val max = range.second.toFloat()
            val lin = x.pow(exp)
            val mapped = if (min < 0f && max <= 0f) {
                val absMin = -max; val absMax = -min
                val absValue = (1f - lin).mapRange(0f..1f, absMin..absMax)
                -absValue
            } else lin.mapRange(0f..1f, min..max)
            value = (kotlin.math.round(mapped / step.toFloat()) * step.toFloat()) as T
        }

        override fun formatValue(): String {
            return formatParamValue(value.toDouble(), unit, step.toFloat())
        }
    }

    data class Discrete<T>(
        val modId: String,
        override val id: Int,
        override val name: String,
        override val unit: ParamUnit,
        val possibleValues: List<T>,
        val initialValue: T
    ) : ModulatorParameter<T>(id, name) {
        override var value by mutableStateOf(initialValue)
        override fun formatValue(): String = ""
    }
}
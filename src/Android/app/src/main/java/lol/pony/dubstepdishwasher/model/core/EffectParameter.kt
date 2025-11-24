package lol.pony.dubstepdishwasher.model.core

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import kotlin.math.pow

sealed class EffectParameter<T> (
    override val id: Int,
    override val name: String
) : Parameter<T> {
    abstract override var value: T
    abstract override val unit: ParamUnit

    data class Range<T : Number>(
        val effectId: Int,
        override val id: Int,
        override val name: String,
        override val unit: ParamUnit,
        val range: Pair<T, T>,
        val exp: Float,
        val step: T,
        val initialValue: T,
        val isModulatable: Boolean = true
    ) : EffectParameter<T>(id, name), NumericParam, NormalizableParam, ModulatableParam {
        override var value by mutableStateOf(initialValue)

        override val key get() = ParamKey(effectId, id)

        @Suppress("UNCHECKED_CAST")
        private fun roundToStep(v: Float): T {
            val rounded = kotlin.math.round(v / step.toFloat()) * step.toFloat()
            return rounded as T
        }

        override fun normalized(): Float {
            val min = range.first.toFloat()
            val max = range.second.toFloat()
            val v = value.toFloat()

            val lin = if (min < 0f && max <= 0f) {
                // Handle negative ranges
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
                // Handle negative ranges
                val absMin = -max; val absMax = -min
                val absValue = (1f - lin).mapRange(0f..1f, absMin..absMax)
                -absValue
            } else lin.mapRange(0f..1f, min..max)

            value = roundToStep(mapped)
        }

        fun normalizedTo(norm: Float): Float {
            val min = range.first.toFloat(); val max = range.second.toFloat()
            val lin = norm.pow(exp)
            return if (min < 0f && max <= 0f) {
                val absMin = -max; val absMax = -min
                val absValue = (1f - lin).mapRange(0f..1f, absMin..absMax)
                -absValue
            } else lin.mapRange(0f..1f, min..max)
        }

        override fun formatValue(): String {
            return formatParamValue(value.toDouble(), unit, step.toFloat())
        }
    }

    data class Discrete<T>(
        val effectId: Int,
        override val id: Int,
        override val name: String,
        override val unit: ParamUnit,
        val possibleValues: List<T>,
        val initialValue: T
    ) : EffectParameter<T>(id, name), NumericParam {
        override var value by mutableStateOf(initialValue)

        override fun formatValue(): String = ""
    }

    data class Toggle(
        val effectId: Int,
        override val id: Int,
        override val name: String,
        val initialValue: Boolean
    ) : EffectParameter<Boolean>(id, name) {
        override var value by mutableStateOf(initialValue)
        override val unit get() = ParamUnit.DIMENSIONLESS

        override fun formatValue(): String = if (value) "On" else "Off"
    }
}
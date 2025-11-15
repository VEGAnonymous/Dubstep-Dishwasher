package lol.pony.dubstepdishwasher.model.core

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import java.math.BigDecimal
import java.math.RoundingMode
import kotlin.math.pow
import kotlin.math.roundToInt

sealed class EffectParameter<T> (
    open val id: Int,
    open val name: String
) {
    abstract var value: T

    fun formatValue(): String {
        return when (this) {
            is Range<*> -> {
                val decimalPlaces = when { // Determine rounding precision
                    step.toFloat() >= 1.0f -> 0
                    step.toFloat() >= 0.1f -> 1
                    step.toFloat() >= 0.01f -> 2
                    step.toFloat() >= 0.001f -> 3
                    else -> 4
                }

                // Build unit string
                val (displayValue, displayDecimals, unitStr) = when (unit) {
                    ParamUnit.PERCENT -> Triple(value.toDouble() * 100.0, (decimalPlaces - 2).coerceAtLeast(0), "%")
                    ParamUnit.MS -> {
                        val v = value.toDouble()
                        if (v >= 1000.0) Triple(v / 1000.0, (decimalPlaces + 3).coerceAtMost(4), " s")
                        else Triple(v, decimalPlaces, " ms")
                    }
                    ParamUnit.HZ -> Triple(value.toDouble(), decimalPlaces, " Hz")
                    ParamUnit.DB -> Triple(value.toDouble(), decimalPlaces, " dB")
                    ParamUnit.SEMITONES -> Triple(value.toDouble(), decimalPlaces, " st")
                    ParamUnit.ENUM, ParamUnit.DIMENSIONLESS -> Triple(value.toDouble(), decimalPlaces, "")
                }

                val rounded = BigDecimal(displayValue).setScale(displayDecimals, RoundingMode.HALF_UP)
                "$rounded$unitStr"
            }

            is Discrete<*> -> ""
            is Toggle -> if (value) "On" else "Off"
        }
    }

    data class Range<T : Number>(
        override val id: Int,
        override val name: String,
        override val unit: ParamUnit,
        val range: Pair<T, T>,
        val exp: Float,
        val step: T,
        val initialValue: T
    ) : EffectParameter<T>(id, name), NumericParam, NormalizableParam {
        override var value by mutableStateOf(initialValue)

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
            val min = range.first.toFloat()
            val max = range.second.toFloat()

            val lin = x.pow(exp)

            val mapped = if (min < 0f && max <= 0f) {
                // Handle negative ranges
                val absMin = -max
                val absMax = -min
                val absValue = (1f - lin).mapRange(0f..1f, absMin..absMax)
                -absValue
            } else lin.mapRange(0f..1f, min..max)

            value = roundToStep(mapped)
        }
    }

    data class Discrete<T>(
        override val id: Int,
        override val name: String,
        override val unit: ParamUnit,
        val possibleValues: List<T>,
        val initialValue: T
    ) : EffectParameter<T>(id, name), NumericParam {
        override var value by mutableStateOf(initialValue)
    }

    data class Toggle(
        override val id: Int,
        override val name: String,
        val initialValue: Boolean
    ) : EffectParameter<Boolean>(id, name) {
        override var value by mutableStateOf(initialValue)
    }
}
package lol.pony.dubstepdishwasher.model.core

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import kotlinx.serialization.KSerializer
import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable
import kotlinx.serialization.builtins.ListSerializer
import kotlinx.serialization.builtins.serializer
import kotlinx.serialization.descriptors.buildClassSerialDescriptor
import kotlinx.serialization.descriptors.element
import kotlinx.serialization.encoding.CompositeDecoder
import kotlinx.serialization.encoding.Decoder
import kotlinx.serialization.encoding.Encoder
import kotlin.math.pow

@Serializable
sealed class EffectParameter() : Parameter {
    abstract val effectId: Int
    abstract override val id: Int
    abstract override val name: String
    abstract override val unit: ParamUnit
    abstract override fun formatValue(): String

    abstract fun getValueAny(): Any
    abstract fun setValueAny(newValue: Any)

    @Serializable(with = RangeEffectSerializer::class)
    @SerialName("range")
    data class Range(
        override val effectId: Int,
        override val id: Int,
        override val name: String,
        override val unit: ParamUnit,
        val range: Pair<Float, Float>,
        val exp: Float,
        val step: Float,
        val initialValue: Float,
        val isModulatable: Boolean = true
    ) : EffectParameter(), NumericParam, NormalizableParam, ModulatableParam {
        var value by mutableFloatStateOf(initialValue)
        override fun getValueAny(): Any = value
        override fun setValueAny(newValue: Any) { value = newValue as Float }

        override val key get() = ParamKey(effectId, id)

        @Suppress("UNCHECKED_CAST")
        private fun roundToStep(v: Float): Float {
            return kotlin.math.round(v / step) * step
        }

        override fun normalized(): Float {
            val min = range.first
            val max = range.second
            val v = value

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
            val min = range.first; val max = range.second
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
            val min = range.first; val max = range.second
            val lin = norm.pow(exp)
            return if (min < 0f && max <= 0f) {
                val absMin = -max; val absMax = -min
                val absValue = (1f - lin).mapRange(0f..1f, absMin..absMax)
                -absValue
            } else lin.mapRange(0f..1f, min..max)
        }

        override fun formatValue(): String {
            return formatParamValue(value.toDouble(), unit, step)
        }
    }

    @Serializable(with = DiscreteEffectSerializer::class)
    @SerialName("discrete")
    data class Discrete<T>(
        override val effectId: Int,
        override val id: Int,
        override val name: String,
        override val unit: ParamUnit,
        val possibleValues: List<T>,
        val initialValue: T
    ) : EffectParameter(), NumericParam {
        var value by mutableStateOf(initialValue)
        override fun getValueAny(): Any = value as Any
        @Suppress("UNCHECKED_CAST")
        override fun setValueAny(newValue: Any) { value = newValue as T }

        override fun formatValue(): String = ""
    }

    @Serializable(with = ToggleEffectSerializer::class)
    @SerialName("toggle")
    data class Toggle(
        override val effectId: Int,
        override val id: Int,
        override val name: String,
        val initialValue: Boolean
    ) : EffectParameter() {
        var value by mutableStateOf(initialValue)
        override fun getValueAny(): Any = value
        override fun setValueAny(newValue: Any) { value = newValue as Boolean }

        override val unit get() = ParamUnit.DIMENSIONLESS

        override fun formatValue(): String = if (value) "On" else "Off"
    }
}

object DiscreteEffectSerializer : KSerializer<EffectParameter.Discrete<out Enum<*>>> {

    override val descriptor = buildClassSerialDescriptor("discrete") {
        element<Int>("effectId")
        element<Int>("id")
        element<String>("name")
        element<String>("unit")
        element<String>("enumType")
        element<List<String>>("possibleValues")
        element<String>("initialValue")
        element<String>("value")
    }

    @Suppress("UNCHECKED_CAST")
    override fun serialize(encoder: Encoder, value: EffectParameter.Discrete<out Enum<*>>) {
        val composite = encoder.beginStructure(descriptor)

        composite.encodeIntElement(descriptor, 0, value.effectId)
        composite.encodeIntElement(descriptor, 1, value.id)
        composite.encodeStringElement(descriptor, 2, value.name)
        composite.encodeStringElement(descriptor, 3, value.unit.name)
        composite.encodeStringElement(descriptor, 4, value.initialValue::class.qualifiedName!!)

        composite.encodeSerializableElement(
            descriptor,
            5,
            ListSerializer(String.serializer()),
            value.possibleValues.map { it.name }
        )

        composite.encodeStringElement(descriptor, 6, value.initialValue.name)
        composite.encodeStringElement(descriptor, 7, value.value.name)

        composite.endStructure(descriptor)
    }

    @Suppress("UNCHECKED_CAST")
    override fun deserialize(decoder: Decoder): EffectParameter.Discrete<out Enum<*>> {
        val dec = decoder.beginStructure(descriptor)

        var effectId = 0
        var id = 0
        lateinit var name: String
        lateinit var unit: String
        lateinit var enumTypeName: String
        lateinit var possibleValues: List<String>
        lateinit var initialValue: String
        lateinit var value: String

        loop@ while (true) {
            when (dec.decodeElementIndex(descriptor)) {
                CompositeDecoder.DECODE_DONE -> break@loop
                0 -> effectId = dec.decodeIntElement(descriptor, 0)
                1 -> id = dec.decodeIntElement(descriptor, 1)
                2 -> name = dec.decodeStringElement(descriptor, 2)
                3 -> unit = dec.decodeStringElement(descriptor, 3)
                4 -> enumTypeName = dec.decodeStringElement(descriptor, 4)
                5 -> possibleValues = dec.decodeSerializableElement(
                    descriptor,
                    5,
                    ListSerializer(String.serializer())
                )
                6 -> initialValue = dec.decodeStringElement(descriptor, 6)
                7 -> value = dec.decodeStringElement(descriptor, 7)
            }
        }
        dec.endStructure(descriptor)

        // Load enum class dynamically
        val enumClass = Class.forName(enumTypeName).asSubclass(Enum::class.java)

        @Suppress("UNCHECKED_CAST")
        fun enumFromName(enumClass: Class<out Enum<*>>, name: String): Enum<*> {
            return java.lang.Enum.valueOf(enumClass, name)
        }

        val tPossible = possibleValues.map { enumFromName(enumClass, it) }
        val tInitial  = enumFromName(enumClass, initialValue)
        val tValue    = enumFromName(enumClass, value)

        return EffectParameter.Discrete(
            effectId = effectId,
            id = id,
            name = name,
            unit = ParamUnit.valueOf(unit),
            possibleValues = tPossible,
            initialValue = tInitial
        ).apply {
            this.value = tValue
        }
    }
}
object RangeEffectSerializer : KSerializer<EffectParameter.Range> {

    override val descriptor = buildClassSerialDescriptor("range") {
        element<Int>("effectId")
        element<Int>("id")
        element<String>("name")
        element<String>("unit")

        element<Float>("rangeMin")
        element<Float>("rangeMax")
        element<Float>("exp")
        element<Float>("step")
        element<Float>("initialValue")
        element<Boolean>("isModulatable")

        element<Float>("value") // CURRENT VALUE
    }

    override fun serialize(encoder: Encoder, value: EffectParameter.Range) {
        val composite = encoder.beginStructure(descriptor)

        composite.encodeIntElement(descriptor, 0, value.effectId)
        composite.encodeIntElement(descriptor, 1, value.id)
        composite.encodeStringElement(descriptor, 2, value.name)
        composite.encodeStringElement(descriptor, 3, value.unit.name)

        composite.encodeFloatElement(descriptor, 4, value.range.first)
        composite.encodeFloatElement(descriptor, 5, value.range.second)
        composite.encodeFloatElement(descriptor, 6, value.exp)
        composite.encodeFloatElement(descriptor, 7, value.step)
        composite.encodeFloatElement(descriptor, 8, value.initialValue)
        composite.encodeBooleanElement(descriptor, 9, value.isModulatable)

        composite.encodeFloatElement(descriptor, 10, value.value)

        composite.endStructure(descriptor)
    }

    override fun deserialize(decoder: Decoder): EffectParameter.Range {
        val dec = decoder.beginStructure(descriptor)

        var effectId = 0
        var id = 0
        lateinit var name: String
        lateinit var unit: String

        var rangeMin = 0f
        var rangeMax = 0f
        var exp = 1f
        var step = 0f
        var initialValue = 0f
        var isModulatable = true

        var currentValue = 0f

        loop@ while (true) {
            when (dec.decodeElementIndex(descriptor)) {
                CompositeDecoder.DECODE_DONE -> break@loop
                0 -> effectId = dec.decodeIntElement(descriptor, 0)
                1 -> id = dec.decodeIntElement(descriptor, 1)
                2 -> name = dec.decodeStringElement(descriptor, 2)
                3 -> unit = dec.decodeStringElement(descriptor, 3)

                4 -> rangeMin = dec.decodeFloatElement(descriptor, 4)
                5 -> rangeMax = dec.decodeFloatElement(descriptor, 5)
                6 -> exp = dec.decodeFloatElement(descriptor, 6)
                7 -> step = dec.decodeFloatElement(descriptor, 7)
                8 -> initialValue = dec.decodeFloatElement(descriptor, 8)
                9 -> isModulatable = dec.decodeBooleanElement(descriptor, 9)

                10 -> currentValue = dec.decodeFloatElement(descriptor, 10)
            }
        }

        dec.endStructure(descriptor)

        return EffectParameter.Range(
            effectId = effectId,
            id = id,
            name = name,
            unit = ParamUnit.valueOf(unit),
            range = rangeMin to rangeMax,
            exp = exp,
            step = step,
            initialValue = initialValue,
            isModulatable = isModulatable
        ).apply {
            this.value = currentValue
        }
    }
}
object ToggleEffectSerializer : KSerializer<EffectParameter.Toggle> {

    override val descriptor = buildClassSerialDescriptor("toggle") {
        element<Int>("effectId")
        element<Int>("id")
        element<String>("name")

        element<Boolean>("initialValue")

        element<Boolean>("value")  // CURRENT VALUE
    }

    override fun serialize(encoder: Encoder, value: EffectParameter.Toggle) {
        val composite = encoder.beginStructure(descriptor)

        composite.encodeIntElement(descriptor, 0, value.effectId)
        composite.encodeIntElement(descriptor, 1, value.id)
        composite.encodeStringElement(descriptor, 2, value.name)

        composite.encodeBooleanElement(descriptor, 3, value.initialValue)

        composite.encodeBooleanElement(descriptor, 4, value.value)

        composite.endStructure(descriptor)
    }

    override fun deserialize(decoder: Decoder): EffectParameter.Toggle {
        val dec = decoder.beginStructure(descriptor)

        var effectId = 0
        var id = 0
        lateinit var name: String
        var initialValue = false
        var currentValue = false

        loop@ while (true) {
            when (dec.decodeElementIndex(descriptor)) {
                CompositeDecoder.DECODE_DONE -> break@loop
                0 -> effectId = dec.decodeIntElement(descriptor, 0)
                1 -> id = dec.decodeIntElement(descriptor, 1)
                2 -> name = dec.decodeStringElement(descriptor, 2)
                3 -> initialValue = dec.decodeBooleanElement(descriptor, 3)
                4 -> currentValue = dec.decodeBooleanElement(descriptor, 4)
            }
        }

        dec.endStructure(descriptor)

        return EffectParameter.Toggle(
            effectId = effectId,
            id = id,
            name = name,
            initialValue = initialValue
        ).apply {
            this.value = currentValue
        }
    }
}

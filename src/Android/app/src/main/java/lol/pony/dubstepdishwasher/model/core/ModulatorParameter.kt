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
sealed class ModulatorParameter() : Parameter {
    abstract val modId: String
    abstract override val id: Int
    abstract override val name: String
    abstract override val unit: ParamUnit
    abstract override fun formatValue(): String

    abstract fun getValueAny(): Any
    abstract fun setValueAny(newValue: Any)

    @Serializable
    @SerialName("Range")
    data class Range(
        override val modId: String,
        override val id: Int,
        override val name: String,
        override val unit: ParamUnit,
        val range: Pair<Float, Float>,
        val exp: Float,
        val step: Float,
        val initialValue: Float
    ) : ModulatorParameter(), NormalizableParam {
        var value by mutableFloatStateOf(initialValue)
        override fun getValueAny(): Any = value
        override fun setValueAny(newValue: Any) { value = newValue as Float }

        // Copy normalized/fromNormalized logic from EffectParameter.Range
        override fun normalized(): Float {
            val min = range.first
            val max = range.second
            val v = value
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
            val min = range.first; val max = range.second
            val lin = x.pow(exp)
            val mapped = if (min < 0f && max <= 0f) {
                val absMin = -max; val absMax = -min
                val absValue = (1f - lin).mapRange(0f..1f, absMin..absMax)
                -absValue
            } else lin.mapRange(0f..1f, min..max)
            value = (kotlin.math.round(mapped / step) * step)
        }

        override fun formatValue(): String {
            return formatParamValue(value.toDouble(), unit, step)
        }
    }
    @Serializable(with = DiscreteModSerializer::class)
    @SerialName("Discrete")
    data class Discrete<T>(
        override val modId: String,
        override val id: Int,
        override val name: String,
        override val unit: ParamUnit,
        val possibleValues: List<T>,
        val initialValue: T
    ) : ModulatorParameter() {
        var value by mutableStateOf(initialValue)
        override fun getValueAny(): Any = value as Any
        @Suppress("UNCHECKED_CAST")
        override fun setValueAny(newValue: Any) { value = newValue as T }

        override fun formatValue(): String = ""
    }
}

object DiscreteModSerializer : KSerializer<ModulatorParameter.Discrete<out Enum<*>>> {

    override val descriptor = buildClassSerialDescriptor("discrete") {
        element<String>("modId")
        element<Int>("id")
        element<String>("name")
        element<String>("unit")
        element<String>("enumType")
        element<List<String>>("possibleValues")
        element<String>("initialValue")
        element<String>("value")
    }

    @Suppress("UNCHECKED_CAST")
    override fun serialize(encoder: Encoder, value: ModulatorParameter.Discrete<out Enum<*>>) {
        val composite = encoder.beginStructure(descriptor)

        composite.encodeStringElement(descriptor, 0, value.modId)
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
    override fun deserialize(decoder: Decoder): ModulatorParameter.Discrete<out Enum<*>> {
        val dec = decoder.beginStructure(descriptor)

        lateinit var modId: String
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
                0 -> modId = dec.decodeStringElement(descriptor, 0)
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

        return ModulatorParameter.Discrete(
            modId = modId,
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
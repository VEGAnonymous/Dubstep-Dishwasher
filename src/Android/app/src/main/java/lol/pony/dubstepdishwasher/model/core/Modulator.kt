package lol.pony.dubstepdishwasher.model.core

import kotlinx.serialization.Serializable

enum class LFOMode(override val uiName: String) : UIEnum {
    NORMAL("Normal"),
    RANDOM("Random")
}

enum class RandomMode(override val uiName: String) : UIEnum {
    PERLIN("Perlin"),
    SAMPLE_HOLD("S&H"),
    BINARY("Binary")
}
@Serializable
data class CurvePoint(val x: Float, val y: Float, val curve: Float = 0f) // curve = exp

sealed class Modulator(open val id: String) {
    abstract val parameters: MutableList<ModulatorParameter<*>>
    abstract var curve: List<CurvePoint>
    @Suppress("UNCHECKED_CAST")
    fun setParam(paramId: Int, value: Any) {
        val param = parameters.find { it.id == paramId } as? ModulatorParameter<Any> ?: return
        param.value = value
    }
    fun updateCurve(curve: List<CurvePoint>) { this.curve = curve }

    data class LFO(
        override val id: String,
        var phase: Float = 0f, // 0.0-1.0
        override var curve: List<CurvePoint> = triUPCurve()
    ) : Modulator(id) {
        override val parameters = mutableListOf<ModulatorParameter<*>>(
            // Rate
            ModulatorParameter.Range(modId = id, id = 0, name = "Rate", unit = ParamUnit.HZ, range = 0.0f to 20.0f, exp = 3f, step = 0.01f, initialValue = 0.621f),
            // Modulator mode
            ModulatorParameter.Discrete(modId = id, id = 1, name = "Mode", unit = ParamUnit.ENUM, possibleValues = LFOMode.entries, initialValue = LFOMode.NORMAL),
            // Random mode
            ModulatorParameter.Discrete(modId = id, id = 2, name = "Random", unit = ParamUnit.ENUM, possibleValues = RandomMode.entries, initialValue = RandomMode.PERLIN)
        )
    }

    data class Mapping(
        override val id: String,
        var inputValue: Float = 0f,
        override var curve: List<CurvePoint> = rampCurve()
    ) : Modulator(id) {
        override val parameters = mutableListOf<ModulatorParameter<*>>()
    }
}
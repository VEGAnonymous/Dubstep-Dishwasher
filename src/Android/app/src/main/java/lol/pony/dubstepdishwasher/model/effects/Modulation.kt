package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class Modulation(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.MODULATION
    override val parameters = mutableListOf<EffectParameter<*>>(
        EffectParameter.Range(id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, value = 1.0f),
        EffectParameter.Discrete(id = 1, name = "Mode", unit = ParamUnit.ENUM, possibleValues = ModulationEffectMode.entries.toList(), value = ModulationEffectMode.AM),
        EffectParameter.Discrete(id = 2, name = "Modulator", unit = ParamUnit.ENUM, possibleValues = WavetableType.entries.toList(), value = WavetableType.SINE),
        EffectParameter.Range(id = 3, name = "Frequency", unit = ParamUnit.HZ, range = 1.0f to 2000.0f, exp = 2.5f, step = 0.1f, value = 5.0f),
        EffectParameter.Range(id = 4, name = "Depth", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, value = 0.5f),
        EffectParameter.Range(id = 5, name = "Bias", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, value = 0.0f),
        EffectParameter.Range(id = 6, name = "Rectify", unit = ParamUnit.PERCENT, range = -1.0f to 1.0f, exp = 1f, step = 0.01f, value = 0.0f)
    )
}
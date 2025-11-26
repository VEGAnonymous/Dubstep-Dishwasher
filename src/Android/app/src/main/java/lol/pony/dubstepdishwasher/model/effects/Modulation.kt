package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class Modulation(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.MODULATION
    override val resourceUsage = effectType.resourceUsage
    override val parameters = mutableListOf<EffectParameter>(
        EffectParameter.Range(effectId = id, id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Discrete(effectId = id, id = 1, name = "Mode", unit = ParamUnit.ENUM, possibleValues = ModulationEffectMode.entries.toList(), initialValue = ModulationEffectMode.AM),
        EffectParameter.Discrete(effectId = id, id = 2, name = "Modulator", unit = ParamUnit.ENUM, possibleValues = WavetableType.entries.toList(), initialValue = WavetableType.SINE),
        EffectParameter.Range(effectId = id, id = 3, name = "Frequency", unit = ParamUnit.HZ, range = 1.0f to 2000.0f, exp = 4f, step = 0.1f, initialValue = 5.0f),
        EffectParameter.Range(effectId = id, id = 4, name = "Depth", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 0.5f),
        EffectParameter.Range(effectId = id, id = 5, name = "Bias", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 0.0f),
        EffectParameter.Range(effectId = id, id = 6, name = "Rectify", unit = ParamUnit.PERCENT, range = -1.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 0.0f)
    )
}
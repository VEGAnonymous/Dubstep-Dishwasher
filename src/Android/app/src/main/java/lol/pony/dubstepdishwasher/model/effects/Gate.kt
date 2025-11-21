package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class Gate(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.GATE
    override val resourceUsage = effectType.resourceUsage
    override val parameters = mutableListOf<EffectParameter<*>>(
        EffectParameter.Range(effectId = id, id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Range(effectId = id, id = 1, name = "Threshold", unit = ParamUnit.DB, range = -60.0f to 0.0f, exp = 0.5f, step = 0.1f, initialValue = -18.0f),
        EffectParameter.Range(effectId = id, id = 2, name = "Attack", unit = ParamUnit.MS, range = 0.01f to 250.0f, exp = 2f, step = 0.01f, initialValue = 25.0f),
        EffectParameter.Range(effectId = id, id = 3, name = "Release", unit = ParamUnit.MS, range = 0.01f to 1500.0f, exp = 3f, step = 0.01f, initialValue = 25.0f),
        EffectParameter.Range(effectId = id, id = 4, name = "Hold", unit = ParamUnit.MS, range = 1.0f to 1500.0f, exp = 3f, step = 0.01f, initialValue = 50.0f),
        EffectParameter.Toggle(effectId = id, id = 5, name = "Invert", initialValue = false)
    )
}
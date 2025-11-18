package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class Reverb(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.REVERB
    override val resourceUsage = effectType.resourceUsage
    override val parameters = mutableListOf<EffectParameter<*>>(
        EffectParameter.Range(effectId = id, id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 0.2f),
        EffectParameter.Range(effectId = id, id = 1, name = "Predelay", unit = ParamUnit.MS, range = 0.0f to 100.0f, exp = 2f, step = 0.1f, initialValue = 0.0f),
        EffectParameter.Range(effectId = id, id = 2, name = "Decay Time", unit = ParamUnit.MS, range = 100.0f to 10000.0f, exp = 3f, step = 1.0f, initialValue = 3000.0f),
        EffectParameter.Range(effectId = id, id = 3, name = "Mod Rate", unit = ParamUnit.HZ, range = 0.05f to 5.0f, exp = 2f, step = 0.01f, initialValue = 0.5f),
        EffectParameter.Range(effectId = id, id = 4, name = "Mod Depth", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 0.2f)
    )
}
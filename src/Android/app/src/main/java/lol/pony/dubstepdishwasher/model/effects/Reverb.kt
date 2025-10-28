package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class Reverb(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.CHORUS
    override val parameters = mutableListOf<EffectParameter<*>>(
        EffectParameter.Range(id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, value = 0.2f),
        EffectParameter.Range(id = 1, name = "Predelay Time", unit = ParamUnit.MS, range = 0.0f to 100.0f, exp = 2f, step = 0.1f, value = 0.0f),
        EffectParameter.Range(id = 2, name = "Delay Time", unit = ParamUnit.MS, range = 100.0f to 10000.0f, exp = 3f, step = 1.0f, value = 3000.0f),
        EffectParameter.Range(id = 3, name = "Mod Rate", unit = ParamUnit.HZ, range = 0.05f to 5.0f, exp = 2f, step = 0.01f, value = 0.5f),
        EffectParameter.Range(id = 4, name = "Mod Depth", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, value = 0.2f)
    )
}
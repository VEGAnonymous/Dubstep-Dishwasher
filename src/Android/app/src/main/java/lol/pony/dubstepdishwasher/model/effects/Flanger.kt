package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class Flanger(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.FLANGER
    override val parameters = mutableListOf<EffectParameter<*>>(
        EffectParameter.Range(id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, value = 1.0f),
        EffectParameter.Range(id = 1, name = "Rate", unit = ParamUnit.HZ, range = 0f to 20.0f, exp = 2f, step = 0.01f, value = 0.08f),
        EffectParameter.Range(id = 2, name = "Depth", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, value = 1.0f),
        EffectParameter.Range(id = 3, name = "Feedback", unit = ParamUnit.PERCENT, range = -0.95f to 0.95f, exp = 1f, step = 0.01f, value = 0.5f)
    )
}
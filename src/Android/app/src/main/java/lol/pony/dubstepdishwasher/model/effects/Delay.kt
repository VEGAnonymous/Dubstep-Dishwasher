package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class Delay(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.DELAY
    override val parameters = mutableListOf<EffectParameter<*>>(
        EffectParameter.Range(effectId = id, id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 0.3f),
        EffectParameter.Range(effectId = id, id = 1, name = "Delay Time", unit = ParamUnit.MS, range = 1.0f to 500.0f, exp = 2f, step = 0.01f, initialValue = 200.0f),
        EffectParameter.Range(effectId = id, id = 2, name = "Feedback", unit = ParamUnit.PERCENT, range = -0.95f to 0.95f, exp = 1f, step = 0.01f, initialValue = 0.4f)
    )
}
package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class Gain(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.GAIN
    override val parameters = mutableListOf<EffectParameter<*>>(
        EffectParameter.Range(id = 0, name = "Gain", unit = ParamUnit.DB, range = -60.0f to 24.0f, exp = 1f, step = 0.01f, initialValue = 0.0f),
        EffectParameter.Toggle(id = 1, name = "Clip", initialValue = false)
    )
}
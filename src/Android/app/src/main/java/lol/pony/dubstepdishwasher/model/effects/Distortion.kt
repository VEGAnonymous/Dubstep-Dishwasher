package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class Distortion(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.DISTORTION
    override val parameters = mutableListOf<EffectParameter<*>>(
        EffectParameter.Range(id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, value = 1.0f),
        EffectParameter.Discrete(id = 1, name = "Mode", unit = ParamUnit.ENUM, possibleValues = DistortionMode.entries.toList(), value = DistortionMode.TUBE),
        EffectParameter.Range(id = 2, name = "Drive", unit = ParamUnit.PERCENT, range = 0.0 to 1.0f, exp = 1f, step = 0.01f, value = 1.0f)
    )
}
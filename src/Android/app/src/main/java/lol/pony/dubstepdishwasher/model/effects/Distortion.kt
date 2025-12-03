package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.DistortionMode
import lol.pony.dubstepdishwasher.model.core.Effect
import lol.pony.dubstepdishwasher.model.core.EffectParameter
import lol.pony.dubstepdishwasher.model.core.EffectType
import lol.pony.dubstepdishwasher.model.core.ParamUnit

class Distortion(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.DISTORTION
    override val resourceUsage = effectType.resourceUsage
    override val parameters = mutableListOf<EffectParameter>(
        EffectParameter.Range(effectId = id, id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Discrete(effectId = id, id = 1, name = "Mode", unit = ParamUnit.ENUM, possibleValues = DistortionMode.entries.toList(), initialValue = DistortionMode.TUBE),
        EffectParameter.Range(effectId = id, id = 2, name = "Drive", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 0.25f),
    )
}
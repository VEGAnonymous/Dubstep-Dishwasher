package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.Effect
import lol.pony.dubstepdishwasher.model.core.EffectParameter
import lol.pony.dubstepdishwasher.model.core.EffectType
import lol.pony.dubstepdishwasher.model.core.ParamUnit

class Gain(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.GAIN
    override val resourceUsage = effectType.resourceUsage
    override val parameters = mutableListOf<EffectParameter>(
        EffectParameter.Range(effectId = id, id = 0, name = "Gain", unit = ParamUnit.DB, range = -24.0f to 24.0f, exp = 1f, step = 0.01f, initialValue = 0.0f),
        EffectParameter.Toggle(effectId = id, id = 1, name = "Clip", initialValue = false)
    )
}
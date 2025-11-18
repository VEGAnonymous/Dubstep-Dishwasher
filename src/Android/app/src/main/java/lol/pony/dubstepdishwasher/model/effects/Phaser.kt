package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class Phaser(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.PHASER
    override val resourceUsage = effectType.resourceUsage
    override val parameters = mutableListOf<EffectParameter<*>>(
        EffectParameter.Range(effectId = id, id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Range(effectId = id, id = 1, name = "Rate", unit = ParamUnit.HZ, range = 0.0f to 20.0f, exp = 3f, step = 0.01f, initialValue = 0.08f),
        EffectParameter.Range(effectId = id, id = 2, name = "Center", unit = ParamUnit.HZ, range = 50.0f to 8000.0f, exp = 2f, step = 0.1f, initialValue = 600.0f),
        EffectParameter.Range(effectId = id, id = 3, name = "Spread", unit = ParamUnit.PERCENT, range = 0.1f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Range(effectId = id, id = 4, name = "Depth", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 0.5f),
        EffectParameter.Range(effectId = id, id = 5, name = "Feedback", unit = ParamUnit.PERCENT, range = -0.95f to 0.95f, exp = 1f, step = 0.01f, initialValue = 0.8f)
    )
}
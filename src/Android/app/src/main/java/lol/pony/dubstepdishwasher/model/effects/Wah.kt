package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class Wah(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.WAH
    override val parameters = mutableListOf<EffectParameter<*>>(
        EffectParameter.Range(id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Range(id = 1, name = "Minimum Frequency", unit = ParamUnit.HZ, range = 20.0f to 1000.0f, exp = 2f, step = 0.1f, initialValue = 350.0f),
        EffectParameter.Range(id = 2, name = "Maximum Frequency", unit = ParamUnit.HZ, range = 1000.0f to 8000.0f, exp = 2f, step = 0.1f, initialValue = 2500.0f),
        EffectParameter.Range(id = 3, name = "Q", unit = ParamUnit.DIMENSIONLESS, range = 0.3f to 6.0f, exp = 2f, step = 0.01f, initialValue = 1.6f)
    )
}
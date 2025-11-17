package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class Freezer(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.FREEZER
    override val parameters = mutableListOf<EffectParameter<*>>(
        EffectParameter.Range(effectId = id, id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Range(effectId = id, id = 1, name = "Rate", unit = ParamUnit.DIMENSIONLESS, range = -4.0f to 4.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Toggle(effectId = id, id = 2, name = "Spectral Mode", initialValue = false),
        EffectParameter.Discrete(effectId = id, id = 3, name = "FFT Size", unit = ParamUnit.DIMENSIONLESS, possibleValues = listOf(128, 256, 512, 1024), initialValue = 1024),
        EffectParameter.Range(effectId = id, id = 4, name = "Loop Start", unit = ParamUnit.DIMENSIONLESS, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 0.0f),
        EffectParameter.Range(effectId = id, id = 5, name = "Loop End", unit = ParamUnit.DIMENSIONLESS, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f)
    )
}
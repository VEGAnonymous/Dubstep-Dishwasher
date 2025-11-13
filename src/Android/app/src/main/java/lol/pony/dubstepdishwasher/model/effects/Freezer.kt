package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class Freezer(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.FREEZER
    override val parameters = mutableListOf<EffectParameter<*>>(
        EffectParameter.Range(id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, value = 1.0f),
        EffectParameter.Range(id = 1, name = "Rate", unit = ParamUnit.DIMENSIONLESS, range = -4.0f to 4.0f, exp = 1f, step = 0.01f, value = 1.0f),
        EffectParameter.Toggle(id = 2, name = "Spectral Mode", value = false),
        EffectParameter.Discrete(id = 3, name = "FFT Size", unit = ParamUnit.DIMENSIONLESS, possibleValues = listOf(128, 256, 512, 1024), value = 512),
        EffectParameter.Discrete(id = 4, name = "Hop Size", unit = ParamUnit.DIMENSIONLESS, possibleValues = listOf(2, 4, 8), value = 4),
        EffectParameter.Range(id = 5, name = "Loop Start", unit = ParamUnit.DIMENSIONLESS, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, value = 0.0f),
        EffectParameter.Range(id = 6, name = "Loop End", unit = ParamUnit.DIMENSIONLESS, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, value = 1.0f)
    )
}
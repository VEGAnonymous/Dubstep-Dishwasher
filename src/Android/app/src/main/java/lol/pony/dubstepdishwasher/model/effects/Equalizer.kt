package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class Equalizer(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.EQUALIZER
    override val parameters = mutableListOf<EffectParameter<*>>(
        EffectParameter.Range(id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Discrete(id = 1, name = "Band 1 Type", unit = ParamUnit.ENUM, possibleValues = BiquadType.entries.toList(), initialValue = BiquadType.LOW_SHELF),
        EffectParameter.Range(id = 2, name = "Band 1 Cutoff", unit = ParamUnit.HZ, range = 20.0f to 20000.0f, exp = 3f, step = 1.0f, initialValue = 200.0f),
        EffectParameter.Range(id = 3, name = "Band 1 Q", unit = ParamUnit.DIMENSIONLESS, range = 0.02f to 40.0f, exp = 2f, step = 0.01f, initialValue = 0.707f),
        EffectParameter.Range(id = 4, name = "Band 1 Gain", unit = ParamUnit.DB, range = -24.0f to 24.0f, exp = 1f, step = 0.1f, initialValue = 0.0f),
        EffectParameter.Discrete(id = 5, name = "Band 2 Type", unit = ParamUnit.ENUM, possibleValues = BiquadType.entries.toList(), initialValue = BiquadType.HIGH_SHELF),
        EffectParameter.Range(id = 6, name = "Band 2 Cutoff", unit = ParamUnit.HZ, range = 20.0f to 20000.0f, exp = 3f, step = 1.0f, initialValue = 2000.0f),
        EffectParameter.Range(id = 7, name = "Band 2 Q", unit = ParamUnit.DIMENSIONLESS, range = 0.02f to 40.0f, exp = 2f, step = 0.01f, initialValue = 0.707f),
        EffectParameter.Range(id = 8, name = "Band 2 Gain", unit = ParamUnit.DB, range = -24.0f to 24.0f, exp = 1f, step = 0.1f, initialValue = 0.0f)
    )
}
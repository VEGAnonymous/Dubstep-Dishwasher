package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class Granulator(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.GRANULATOR
    override val parameters = mutableListOf<EffectParameter<*>>(
        EffectParameter.Range(id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, value = 1.0f),
        EffectParameter.Range(id = 1, name = "Position", unit = ParamUnit.DIMENSIONLESS, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, value = 0.5f),
        EffectParameter.Range(id = 2, name = "Position Random", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, value = 0.5f),
        EffectParameter.Range(id = 3, name = "Rate", unit = ParamUnit.MS, range = 1.0f to 500.0f, exp = 2f, step = 1.0f, value = 50.0f),
        EffectParameter.Range(id = 4, name = "Rate Random", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, value = 0.0f),
        EffectParameter.Range(id = 5, name = "Length", unit = ParamUnit.MS, range = 5.0f to 500.0f, exp = 2f, step = 1.0f, value = 200.0f),
        EffectParameter.Range(id = 6, name = "Length Random", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, value = 0.0f),
        EffectParameter.Range(id = 7, name = "Level", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, value = 0.8f),
        EffectParameter.Range(id = 8, name = "Level Random", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, value = 0.0f),
        EffectParameter.Range(id = 9, name = "Reverse Chance", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, value = 0.0f),
        EffectParameter.Discrete(id = 10, name = "Envelope", unit = ParamUnit.ENUM, possibleValues = EnvelopeType.entries.toList(), value = EnvelopeType.HANN)
    )
}
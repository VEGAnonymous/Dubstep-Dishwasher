package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.Effect
import lol.pony.dubstepdishwasher.model.core.EffectParameter
import lol.pony.dubstepdishwasher.model.core.EffectType
import lol.pony.dubstepdishwasher.model.core.EnvelopeType
import lol.pony.dubstepdishwasher.model.core.ParamUnit

class Granulator(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.GRANULATOR
    override val resourceUsage = effectType.resourceUsage
    override val parameters = mutableListOf<EffectParameter>(
        EffectParameter.Range(effectId = id, id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Range(effectId = id, id = 1, name = "Position", unit = ParamUnit.DIMENSIONLESS, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 0.5f),
        EffectParameter.Range(effectId = id, id = 2, name = "Position Rand", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 0.5f),
        EffectParameter.Range(effectId = id, id = 3, name = "Rate", unit = ParamUnit.MS, range = 1.0f to 500.0f, exp = 2f, step = 1.0f, initialValue = 50.0f),
        EffectParameter.Range(effectId = id, id = 4, name = "Rate Rand", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 0.0f),
        EffectParameter.Range(effectId = id, id = 5, name = "Length", unit = ParamUnit.MS, range = 5.0f to 500.0f, exp = 2f, step = 1.0f, initialValue = 200.0f),
        EffectParameter.Range(effectId = id, id = 6, name = "Length Rand", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 0.0f),
        EffectParameter.Range(effectId = id, id = 7, name = "Tune", unit = ParamUnit.SEMITONES, range = -24.0f to 24.0f, exp = 1f, step = 0.01f, initialValue = 0.0f),
        EffectParameter.Range(effectId = id, id = 8, name = "Tune Rand", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 0.0f),
        EffectParameter.Range(effectId = id, id = 9, name = "Level", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 0.8f),
        EffectParameter.Range(effectId = id, id = 10, name = "Level Rand", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 0.0f),
        EffectParameter.Range(effectId = id, id = 11, name = "Reverse", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 0.0f),
        EffectParameter.Discrete(effectId = id, id = 12, name = "Envelope", unit = ParamUnit.ENUM, possibleValues = EnvelopeType.entries.toList(), initialValue = EnvelopeType.HANN)
    )
}
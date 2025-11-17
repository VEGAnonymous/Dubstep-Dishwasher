package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class Vocoder(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.VOCODER
    override val parameters = mutableListOf<EffectParameter<*>>(
        EffectParameter.Range(effectId = id, id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Range(effectId = id, id = 1, name = "Bands", unit = ParamUnit.DIMENSIONLESS, range = 4 to 40, exp = 1f, step = 1, initialValue = 20),
        EffectParameter.Range(effectId = id, id = 2, name = "Min Freq", unit = ParamUnit.HZ, range = 10.0f to 16000.0f, exp = 4f, step = 1.0f, initialValue = 80.0f),
        EffectParameter.Range(effectId = id, id = 3, name = "Max Freq", unit = ParamUnit.HZ, range = 10.0f to 16000.0f, exp = 4f, step = 1.0f, initialValue = 12000.0f),
        EffectParameter.Range(effectId = id, id = 4, name = "Bandwidth", unit = ParamUnit.PERCENT, range = 0.05f to 4.0f, exp = 1f, step = 0.01f, initialValue = 0.5f),
        EffectParameter.Range(effectId = id, id = 5, name = "Depth", unit = ParamUnit.PERCENT, range = 0.0f to 2.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Range(effectId = id, id = 6, name = "Attack", unit = ParamUnit.MS, range = 10.0f to 2000.0f, exp = 2.5f, step = 0.1, initialValue = 2.0f),
        EffectParameter.Range(effectId = id, id = 7, name = "Release", unit = ParamUnit.MS, range = 10.0f to 2000.0f, exp = 2.5f, step = 0.1, initialValue = 35.0f)
    )
}
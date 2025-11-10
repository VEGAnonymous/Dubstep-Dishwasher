package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class Compressor(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.COMPRESSOR
    override val parameters = mutableListOf<EffectParameter<*>>(
        EffectParameter.Range(id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, value = 1.0f),
        EffectParameter.Range(id = 1, name = "Threshold", unit = ParamUnit.DB, range = -100.0f to 0.0f, exp = 2f, step = 0.1f, value = -18.0f),
        EffectParameter.Range(id = 2, name = "Ratio", unit = ParamUnit.DIMENSIONLESS, range = 1.0f to 100.0f, exp = 2f, step = 0.01f, value = 4.0f),
        EffectParameter.Range(id = 3, name = "Knee", unit = ParamUnit.DB, range = 0.0f to 40.0f, exp = 2f, step = 0.1f, value = 10.0f),
        EffectParameter.Range(id = 4, name = "Attack Time", unit = ParamUnit.MS, range = 0.01f to 250.0f, exp = 2f, step = 0.01f, value = 100.0f),
        EffectParameter.Range(id = 5, name = "Release Time", unit = ParamUnit.MS, range = 10.0f to 2500.0f, exp = 3f, step = 0.1f, value = 100.0f),
        EffectParameter.Range(id = 6, name = "Makeup Gain", unit = ParamUnit.MS, range = -72.0f to 36.0f, exp = 1f, step = 0.1f, value = 0.0f),
        EffectParameter.Toggle(id = 7, name = "Auto-Makeup", value = true)
    )
}
package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class Scrubby(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.SCRUBBY
    override val parameters = mutableListOf<EffectParameter<*>>(
        EffectParameter.Range(effectId = id, id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Range(effectId = id, id = 1, name = "Seek Rate Low", unit = ParamUnit.HZ, range = 0.3f to 810.0f, exp = 2.5f, step = 0.1f, initialValue = 9.0f),
        EffectParameter.Range(effectId = id, id = 2, name = "Seek Rate High", unit = ParamUnit.HZ, range = 0.3f to 810.0f, exp = 2.5f, step = 0.1f, initialValue = 9.0f),
        EffectParameter.Range(effectId = id, id = 3, name = "Seek Range", unit = ParamUnit.MS, range = 0.3f to 6000.0f, exp = 3.5f, step = 0.1f, initialValue = 333.0f),
        EffectParameter.Range(effectId = id, id = 4, name = "Seek Dur Low", unit = ParamUnit.PERCENT, range = 0.03f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Range(effectId = id, id = 5, name = "Seek Dur High", unit = ParamUnit.PERCENT, range = 0.03f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Range(effectId = id, id = 6, name = "Octaves Down", unit = ParamUnit.DIMENSIONLESS, range = -4 to 0, exp = 1f, step = 1, initialValue = -4),
        EffectParameter.Range(effectId = id, id = 7, name = "Octaves Up", unit = ParamUnit.DIMENSIONLESS, range = 0 to 8, exp = 1f, step = 1, initialValue = 8)
    )
}
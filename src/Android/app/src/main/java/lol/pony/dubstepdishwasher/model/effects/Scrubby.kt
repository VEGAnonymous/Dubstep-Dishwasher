package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class Scrubby(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.SCRUBBY
    override val resourceUsage = effectType.resourceUsage
    override val parameters = mutableListOf<EffectParameter>(
        EffectParameter.Range(effectId = id, id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Range(effectId = id, id = 1, name = "Rate L", unit = ParamUnit.HZ, range = 0.3f to 810.0f, exp = 2.5f, step = 0.1f, initialValue = 9.0f),
        EffectParameter.Range(effectId = id, id = 2, name = "Rate H", unit = ParamUnit.HZ, range = 0.3f to 810.0f, exp = 2.5f, step = 0.1f, initialValue = 9.0f),
        EffectParameter.Range(effectId = id, id = 3, name = "Range", unit = ParamUnit.MS, range = 0.3f to 6000.0f, exp = 3.5f, step = 0.1f, initialValue = 333.0f),
        EffectParameter.Range(effectId = id, id = 4, name = "Dur Low", unit = ParamUnit.PERCENT, range = 0.03f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Range(effectId = id, id = 5, name = "Dur High", unit = ParamUnit.PERCENT, range = 0.03f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Range(effectId = id, id = 6, name = "Octaves -", unit = ParamUnit.DIMENSIONLESS, range = -4f to 0f, exp = 1f, step = 1f, initialValue = -4f, isModulatable = false),
        EffectParameter.Range(effectId = id, id = 7, name = "Octaves +", unit = ParamUnit.DIMENSIONLESS, range = 0f to 8f, exp = 1f, step = 1f, initialValue = 8f, isModulatable = false)
    )
}
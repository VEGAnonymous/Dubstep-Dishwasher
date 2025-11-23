package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class FormantShifter(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.FORMANT_SHIFTER
    override val resourceUsage = effectType.resourceUsage
    override val parameters = mutableListOf<EffectParameter<*>>(
        EffectParameter.Range(effectId = id, id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Discrete(effectId = id, id = 1, name = "FFT Size", unit = ParamUnit.DIMENSIONLESS, possibleValues = listOf(128, 256, 512, 1024), initialValue = 1024),
        EffectParameter.Range(effectId = id, id = 2, name = "Shift", unit = ParamUnit.SEMITONES, range = -12.0f to 12.0f, exp = 1f, step = 0.01f, initialValue = 0.0f),
        EffectParameter.Range(effectId = id, id = 3, name = "Envelope Width", unit = ParamUnit.DIMENSIONLESS, range = 0f to 16f, exp = 1f, step = 1f, initialValue = 16f, isModulatable = false)
    )
}
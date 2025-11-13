package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class FormantShifter(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.FORMANT_SHIFTER
    override val parameters = mutableListOf<EffectParameter<*>>(
        EffectParameter.Range(id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Discrete(id = 1, name = "FFT Size", unit = ParamUnit.DIMENSIONLESS, possibleValues = listOf(128, 256, 512, 1024), initialValue = 1024),
        EffectParameter.Range(id = 2, name = "Formant Shift", unit = ParamUnit.SEMITONES, range = -12.0f to 12.0f, exp = 1f, step = 0.01f, initialValue = 0.0f),
        EffectParameter.Range(id = 3, name = "Envelope Width", unit = ParamUnit.DIMENSIONLESS, range = 0 to 16, exp = 1f, step = 1, initialValue = 16)
    )
}
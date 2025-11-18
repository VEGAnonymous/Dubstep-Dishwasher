package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class SpectralGate(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.SPECTRAL_GATE
    override val resourceUsage = effectType.resourceUsage
    override val parameters = mutableListOf<EffectParameter<*>>(
        EffectParameter.Range(effectId = id, id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Discrete(effectId = id, id = 1, name = "FFT Size", unit = ParamUnit.DIMENSIONLESS, possibleValues = listOf(128, 256, 512, 1024), initialValue = 512),
        EffectParameter.Range(effectId = id, id = 2, name = "Threshold", unit = ParamUnit.DB, range = -100.0f to 0.0f, exp = 0.33f, step = 0.1f, initialValue = -10.0f),
        EffectParameter.Range(effectId = id, id = 3, name = "Tilt", unit = ParamUnit.DIMENSIONLESS, range = -1.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 0.5f),
        EffectParameter.Toggle(effectId = id, id = 4, name = "Invert", initialValue = false)
    )
}
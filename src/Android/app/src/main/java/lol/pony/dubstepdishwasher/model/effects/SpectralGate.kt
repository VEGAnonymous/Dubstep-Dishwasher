package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class SpectralGate(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.SPECTRAL_GATE
    override val parameters = mutableListOf<EffectParameter<*>>(
        EffectParameter.Range(id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Discrete(id = 1, name = "FFT Size", unit = ParamUnit.DIMENSIONLESS, possibleValues = listOf(128, 256, 512, 1024), initialValue = 512),
        EffectParameter.Range(id = 2, name = "Threshold", unit = ParamUnit.DB, range = -100.0f to 0.0f, exp = 2f, step = 0.1f, initialValue = -10.0f),
        EffectParameter.Range(id = 3, name = "Tilt", unit = ParamUnit.DIMENSIONLESS, range = -1.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 0.5f)
    )
}
package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class PitchShifter(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.PITCH_SHIFTER
    override val parameters = mutableListOf<EffectParameter<*>>(
        EffectParameter.Range(id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, value = 1.0f),
        EffectParameter.Range(id = 1, name = "Pitch Shift", unit = ParamUnit.SEMITONES, range = -24.0f to 24.0f, exp = 1f, step = 0.01f, value = 0.0f),
        EffectParameter.Range(id = 2, name = "Grain Size", unit = ParamUnit.MS, range = 20.0f to 500.0f, exp = 2f, step = 0.1f, value = 200.0f),
        EffectParameter.Range(id = 3, name = "Grain Overlap", unit = ParamUnit.PERCENT, range = 0.25f to 0.75f, exp = 1f, step = 0.01f, value = 0.5f),
        EffectParameter.Range(id = 4, name = "Jitter", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, value = 0.0f)
    )
}
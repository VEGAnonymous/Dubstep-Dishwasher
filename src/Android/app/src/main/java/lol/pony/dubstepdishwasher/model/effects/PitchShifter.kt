package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.*

class PitchShifter(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.PITCH_SHIFTER
    override val resourceUsage = effectType.resourceUsage
    override val parameters = mutableListOf<EffectParameter>(
        EffectParameter.Range(effectId = id, id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Range(effectId = id, id = 1, name = "Shift", unit = ParamUnit.SEMITONES, range = -24.0f to 24.0f, exp = 1f, step = 0.01f, initialValue = 0.0f),
        EffectParameter.Range(effectId = id, id = 2, name = "Grain Size", unit = ParamUnit.MS, range = 20.0f to 500.0f, exp = 2f, step = 0.1f, initialValue = 200.0f),
        EffectParameter.Range(effectId = id, id = 3, name = "Grain Overlap", unit = ParamUnit.PERCENT, range = 0.25f to 0.75f, exp = 1f, step = 0.01f, initialValue = 0.5f),
        EffectParameter.Range(effectId = id, id = 4, name = "Jitter", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 0.0f)
    )
}
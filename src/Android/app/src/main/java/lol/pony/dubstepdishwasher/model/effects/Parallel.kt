package lol.pony.dubstepdishwasher.model.effects

import lol.pony.dubstepdishwasher.model.core.Effect
import lol.pony.dubstepdishwasher.model.core.EffectParameter
import lol.pony.dubstepdishwasher.model.core.EffectType
import lol.pony.dubstepdishwasher.model.core.ParallelMode
import lol.pony.dubstepdishwasher.model.core.ParamUnit

class Parallel(id: Int) : Effect() {
    override val effectId = id
    override val effectType = EffectType.PARALLEL
    override val resourceUsage = effectType.resourceUsage // TEMP
    override val parameters = mutableListOf<EffectParameter>(
        EffectParameter.Range(effectId = id, id = 0, name = "Mix", unit = ParamUnit.PERCENT, range = 0.0f to 1.0f, exp = 1f, step = 0.01f, initialValue = 1.0f),
        EffectParameter.Discrete(effectId = id, id = 1, name = "Mode", unit = ParamUnit.ENUM, possibleValues = ParallelMode.entries.toList(), initialValue = ParallelMode.SUM),
    )
}
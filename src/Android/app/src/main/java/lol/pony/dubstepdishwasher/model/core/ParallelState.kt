package lol.pony.dubstepdishwasher.model.core

import lol.pony.dubstepdishwasher.model.EffectChain

enum class ParallelChain { A, B }
data class ParallelChainState(
    val chainA: EffectChain = EffectChain(),
    val chainB: EffectChain = EffectChain(),
    val chainAEffects: List<Effect> = emptyList(),
    val chainBEffects: List<Effect> = emptyList(),
    val chainAUsage: ResourceUsage = ResourceUsage(0f, 0),
    val chainBUsage: ResourceUsage = ResourceUsage(0f, 0)
)
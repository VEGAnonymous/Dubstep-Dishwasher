package lol.pony.dubstepdishwasher.viewmodel

import android.util.Log
import kotlinx.coroutines.flow.update
import lol.pony.dubstepdishwasher.model.EffectChain
import lol.pony.dubstepdishwasher.model.core.*
import lol.pony.dubstepdishwasher.model.core.CommandType.PARALLEL_CHAIN_COMMAND

internal fun encodeChainParam(chain: ParallelChain, paramId: Int): Int {
    val chainBit = if (chain == ParallelChain.B) 1 else 0
    return (chainBit shl 4) or (paramId and 0x0F)
}

internal fun MainViewModel.getParallelEffectChain(parallelId: Int, chain: ParallelChain): EffectChain? {
    val state = _parallelChains.value[parallelId] ?: return null
    return when (chain) {
        ParallelChain.A -> state.chainA
        ParallelChain.B -> state.chainB
    }
}

internal fun MainViewModel.updateParallelChainState(parallelId: Int) {
    _parallelChains.update { chains ->
        val state = chains[parallelId] ?: return@update chains
        chains + (parallelId to state.copy(
            chainAEffects = state.chainA.getAll(),
            chainBEffects = state.chainB.getAll(),
            chainAUsage = state.chainA.totalUsage(),
            chainBUsage = state.chainB.totalUsage()
        ))
    }
}

fun MainViewModel.parallelAddEffect(parallelId: Int, chain: ParallelChain, effectType: EffectType) {
    val effectChain = getParallelEffectChain(parallelId, chain) ?: return

    val projected = effectChain.projectedUsage(effectType)
    val total = calculateTotalUsage()
    val projectedTotal = ResourceUsage(
        compute = total.compute + projected.compute - effectChain.totalUsage().compute,
        memory = total.memory + projected.memory - effectChain.totalUsage().memory
    )
    if (projectedTotal.compute > MAX_COMPUTE_USAGE || projectedTotal.memory > MAX_MEMORY_USAGE) {
        _resourceError.value = "Could not add ${effectType.uiName}: resource limit exceeded"
        return
    }

    effectChain.addEffect(effectType)
    updateParallelChainState(parallelId)
    _resourceUsage.value = calculateTotalUsage()

    controlQueue.enqueue(PARALLEL_CHAIN_COMMAND,
        parallelId, encodeChainParam(chain, 0), 0f, 0f, effectType.ordinal.toFloat())
    if (LOG_DEBUG) Log.d("CMD", "PARALLEL id=$parallelId - EFFECT_ADD: effectType=$effectType")
}

fun MainViewModel.parallelRemoveEffect(parallelId: Int, chain: ParallelChain, effectId: Int) {
    val effectChain = getParallelEffectChain(parallelId, chain) ?: return
    effectChain.removeEffect(effectId)
    updateParallelChainState(parallelId)
    _resourceUsage.value = calculateTotalUsage()

    controlQueue.enqueue(PARALLEL_CHAIN_COMMAND,
        parallelId, encodeChainParam(chain, 0), 1f, effectId.toFloat(), 0f)
    if (LOG_DEBUG) Log.d("CMD", "PARALLEL id=$parallelId - EFFECT_REMOVE: effectId=$effectId")
}

fun MainViewModel.parallelReorderEffect(parallelId: Int, chain: ParallelChain, effectId: Int, toIndex: Int) {
    val effectChain = getParallelEffectChain(parallelId, chain) ?: return
    effectChain.reorderEffect(effectId, toIndex)
    updateParallelChainState(parallelId)

    controlQueue.enqueue(PARALLEL_CHAIN_COMMAND,
        parallelId, encodeChainParam(chain, 0), 2f, effectId.toFloat(), toIndex.toFloat())
    if (LOG_DEBUG) Log.d("CMD", "PARALLEL id=$parallelId - EFFECT_REORDER: effectId=$effectId, toIndex=$toIndex")
}

fun MainViewModel.parallelSetParam(parallelId: Int, chain: ParallelChain, effectId: Int, paramId: Int, value: Any) {
    val effectChain = getParallelEffectChain(parallelId, chain) ?: return
    effectChain.setParam(effectId, paramId, value)
    updateParallelChainState(parallelId)

    // Casting to float for value
    val sendValue = when (value) {
        is Float -> value
        is Int -> value.toFloat()
        is Boolean -> if (value) 1.0f else 0.0f

        is EnvelopeType, is ModulationEffectMode, is DistortionMode, is BiquadType, is ParallelMode, is WavetableType, is ParamUnit
            -> value.ordinal.toFloat()

        else -> throw IllegalArgumentException("Unsupported value type")
    }

    controlQueue.enqueue(PARALLEL_CHAIN_COMMAND,
        parallelId, encodeChainParam(chain, paramId), 3f, effectId.toFloat(), sendValue)
    if (LOG_DEBUG) Log.d("CMD", "PARALLEL id=$parallelId - EFFECT_SET_PARAMETER: effectId=$effectId, paramId=$paramId, value=$value")
}

fun MainViewModel.parallelBypassEffect(parallelId: Int, chain: ParallelChain, effectId: Int) {
    val effectChain = getParallelEffectChain(parallelId, chain) ?: return
    val value = if (effectChain.get(effectId)!!.isBypassed) 0.0f else 1.0f
    effectChain.setBypass(effectId, !effectChain.get(effectId)!!.isBypassed)
    updateParallelChainState(parallelId)

    controlQueue.enqueue(PARALLEL_CHAIN_COMMAND,
        parallelId, encodeChainParam(chain, 0), 4f, effectId.toFloat(), value)

    if (LOG_DEBUG) Log.d("CMD", "PARALLEL id=$parallelId - EFFECT_BYPASS: effectId=$effectId, bypass=$value")
}
package lol.pony.dubstepdishwasher.viewmodel

import android.util.Log
import kotlinx.coroutines.flow.update
import lol.pony.dubstepdishwasher.model.core.*
import lol.pony.dubstepdishwasher.model.core.CommandType.*

fun MainViewModel.addEffect(type: EffectType) {
    val projected = chain.projectedUsage(type)
    val total = calculateTotalUsage()
    val projectedTotal = ResourceUsage(
        compute = total.compute + projected.compute - chain.totalUsage().compute,
        memory = total.memory + projected.memory - chain.totalUsage().memory
    )
    if (projectedTotal.compute > MAX_COMPUTE_USAGE || projectedTotal.memory > MAX_MEMORY_USAGE) {
        _resourceError.value = "Could not add ${type.uiName}: resource limit exceeded"
        return
    }

    chain.addEffect(type)
    _effects.value = chain.getAll()
    _resourceUsage.value = calculateTotalUsage()
    controlQueue.enqueue(EFFECT_ADD, type.ordinal, 0, 0.0f)

    // Init parallel state if applicable
    if (type == EffectType.PARALLEL) { _parallelChains.update { chains -> chains + (_effects.value.last().effectId to ParallelChainState()) } }
    if (LOG_DEBUG) Log.d("CMD", "EFFECT_ADD: effectType=${type.name}")
}

fun MainViewModel.removeEffect(effectId: Int) {
    val effect = chain.get(effectId)
    chain.removeEffect(effectId)
    _modAssignments.value = _modAssignments.value.filterNot { it.target.effectId == effectId } // Also remove mod assignments
    if (effect?.effectType == EffectType.PARALLEL) { _parallelChains.update { chains -> chains - effectId } } // Clean up any parallel state

    _effects.value = chain.getAll()
    _resourceUsage.value = calculateTotalUsage()

    controlQueue.enqueue(EFFECT_REMOVE, effectId, 0, 0.0f)
    _modAssignments.value.filter { it.target.effectId == effectId }.forEach { assignment -> // And downstream assignments
        val modIndex = modulatorIdToIndex(assignment.modId)
        controlQueue.enqueue(
            MOD_ASSIGNMENT_REMOVE, modIndex, assignment.target.effectId, assignment.target.paramId.toFloat()
        )
    }
    if (LOG_DEBUG) Log.d("CMD", "EFFECT_REMOVE: effectId=$effectId")
}

fun MainViewModel.reorderEffect(effectId: Int, toIndex: Int) {
    chain.reorderEffect(effectId, toIndex)
    _effects.value = chain.getAll()
    controlQueue.enqueue(EFFECT_REORDER, effectId, toIndex, 0.0f)
    if (LOG_DEBUG) Log.d("CMD", "EFFECT_REORDER: effectId=$effectId, toIndex=$toIndex")
}

fun MainViewModel.setParam(effectId: Int, paramId: Int, value: Any) {
    chain.setParam(effectId, paramId, value)
    _effects.value = chain.getAll()

    val sendValue = when (value) {
        is Float -> value
        is Int -> value.toFloat()
        is Boolean -> if (value) 1.0f else 0.0f

        is FFTSize -> value.value
        is EnvelopeType, is ModulationEffectMode, is DistortionMode, is BiquadType, is ParallelMode, is WavetableType, is ParamUnit
            -> value.ordinal.toFloat()

        else -> return
    }

    controlQueue.enqueue(EFFECT_SET_PARAMETER, effectId, paramId, sendValue)
    if (LOG_DEBUG) Log.d("CMD", "EFFECT_SET_PARAMETER: effectId=$effectId, paramId=$paramId, value=$value")
}

fun MainViewModel.toggleBypass(effectId: Int) {
    val value = if (chain.get(effectId)!!.isBypassed) 0.0f else 1.0f
    chain.setBypass(effectId, !chain.get(effectId)!!.isBypassed)
    _effects.value = chain.getAll()
    controlQueue.enqueue(EFFECT_BYPASS, effectId, 0, value)
    if (LOG_DEBUG) Log.d("CMD", "EFFECT_BYPASS: effectId=$effectId, bypass=$value")
}

fun MainViewModel.clearChain() {
    chain.clear()
    _modAssignments.value = emptyList()
    _effects.value = chain.getAll()
    _parallelChains.value = emptyMap()
    _resourceUsage.value = ResourceUsage(0f, 0)
    controlQueue.enqueue(EFFECT_CLEAR, 0, 0, 0.0f)
    if (LOG_DEBUG) Log.d("CMD", "EFFECT_CLEAR")
}
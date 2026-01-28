package lol.pony.dubstepdishwasher.viewmodel

import android.util.Log
import lol.pony.dubstepdishwasher.model.core.*
import lol.pony.dubstepdishwasher.model.core.CommandType.*

internal fun MainViewModel.modulatorIdToIndex(modId: String): Int { return _modulators.value.indexOfFirst { it.id == modId }.coerceIn(0, 11) }

fun MainViewModel.setModulatorParam(modId: String, paramId: Int, value: Any) {
    val mod = _modulators.value.find { it.id == modId } ?: return
    mod.setParam(paramId, value)

    // Casting to float for value
    val modIndex = modulatorIdToIndex(modId)
    val sendValue = when (value) {
        is Float -> value
        is Int -> value.toFloat()
        is LFOMode -> value.ordinal.toFloat()
        is RandomMode -> value.ordinal.toFloat()
        else -> throw IllegalArgumentException("Unsupported value type")
    }

    controlQueue.enqueue(MOD_SET_PARAMETER,
        modIndex, paramId, sendValue)
    if (LOG_DEBUG) Log.d("CMD", "MOD_SET_PARAMETER: modId=$modIndex, paramId=$paramId, value=$sendValue")
}

fun MainViewModel.updateModulatorCurve(modId: String, curve: List<CurvePoint>) {
    _modulators.value = _modulators.value.map { mod ->
        if (mod.id == modId) {
            mod.updateCurve(curve)
            when (mod) {
                is Modulator.LFO -> mod.copy()
                is Modulator.Mapping -> mod.copy()
            }
        } else mod
    }

    val modIndex = modulatorIdToIndex(modId)

    // Clear existing curve
    controlQueue.enqueue(MOD_CLEAR_CURVE,
        modIndex, 0, 0f)

    // Send each point
    curve.forEachIndexed { index, point ->
        controlQueue.enqueue(MOD_SET_CURVE_POINT,
            modIndex, index, point.x, point.y, point.curve)
    }

    // Reset LFO phase to sync with downstream
    val mod = _modulators.value.find { it.id == modId }
    if (mod is Modulator.LFO) mod.phase = 0f
    if (LOG_DEBUG) Log.d("CMD", "MOD_UPDATE_CURVE")
}

fun MainViewModel.addAssignment(modId: String, effectId: Int, paramId: Int) {
    val key = ParamKey(effectId, paramId)
    val assignList = _modAssignments.value.toMutableList()
    val exists = assignList.any { it.modId == modId && it.target == key } // Prevent duplicate assignments

    if (!exists) {
        val assignment = ModAssignment(
            modId = modId,
            target = key,
            amount = 0.5f,
            polarity = ModPolarity.Bipolar
        )
        assignList += assignment
        _modAssignments.value = assignList

        val modIndex = modulatorIdToIndex(modId)
        controlQueue.enqueue(MOD_ASSIGNMENT_ADD,
            modIndex, effectId, paramId.toFloat(), 0.5f, ModPolarity.Bipolar.ordinal.toFloat())
        if (LOG_DEBUG) Log.d("CMD", "MOD_ASSIGNMENT_ADD: effectId=$effectId, paramId=$paramId, amount=0.5, polarity=${ModPolarity.Bipolar.ordinal}")
    }
}

fun MainViewModel.removeAssignment(modId: String, effectId: Int, paramId: Int) {
    _modAssignments.value = _modAssignments.value.filterNot {
        it.modId == modId && it.target.effectId == effectId && it.target.paramId == paramId
    }

    val modIndex = modulatorIdToIndex(modId)
    controlQueue.enqueue(MOD_ASSIGNMENT_REMOVE,
        modIndex, effectId, paramId.toFloat())
    if (LOG_DEBUG) Log.d("CMD", "MOD_ASSIGNMENT_REMOVE: effectId=$effectId, paramId=$paramId")
}

fun MainViewModel.updateAssignmentAmount(modId: String, effectId: Int, paramId: Int, amount: Float) {
    _modAssignments.value = _modAssignments.value.map {
        if (it.modId == modId && it.target.effectId == effectId && it.target.paramId == paramId)
            it.copy(amount = amount)
        else it
    }

    val modIndex = modulatorIdToIndex(modId)
    val assignment = _modAssignments.value.find {
        it.modId == modId && it.target.effectId == effectId && it.target.paramId == paramId
    } ?: return

    controlQueue.enqueue(MOD_ASSIGNMENT_SET,
        modIndex, effectId, paramId.toFloat(), assignment.amount, assignment.polarity.ordinal.toFloat())
    if (LOG_DEBUG) Log.d("CMD", "MOD_ASSIGNMENT_SET: effectId=$effectId, paramId=$paramId, amount=${assignment.amount}, polarity=${assignment.polarity.ordinal}")
}

fun MainViewModel.updateAssignmentPolarity(modId: String, effectId: Int, paramId: Int) {
    _modAssignments.value = _modAssignments.value.map {
        if (it.modId == modId && it.target.effectId == effectId && it.target.paramId == paramId) {
            val newPolarity = when (it.polarity) {
                ModPolarity.Bipolar -> ModPolarity.Unipolar
                ModPolarity.Unipolar -> ModPolarity.Bipolar
            }
            it.copy(polarity = newPolarity)
        } else it
    }

    val modIndex = modulatorIdToIndex(modId)
    val assignment = _modAssignments.value.find {
        it.modId == modId && it.target.effectId == effectId && it.target.paramId == paramId
    } ?: return

    controlQueue.enqueue(MOD_ASSIGNMENT_SET,
        modIndex, effectId, paramId.toFloat(), assignment.amount, assignment.polarity.ordinal.toFloat())
    if (LOG_DEBUG) Log.d("CMD", "MOD_ASSIGNMENT_SET: effectId=$effectId, paramId=$paramId, amount=${assignment.amount}, polarity=${assignment.polarity.ordinal}")
}

fun MainViewModel.setMappingInput(modId: String, normalizedInput: Float) {
    val mod = _modulators.value.find { it.id == modId }
    if (mod is Modulator.Mapping) mod.inputValue = normalizedInput
    if (LOG_DEBUG) Log.d("status", "SET_MAPPING_INPUT: $modId, $normalizedInput")
}
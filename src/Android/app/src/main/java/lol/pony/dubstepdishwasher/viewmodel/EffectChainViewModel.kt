package lol.pony.dubstepdishwasher.viewmodel

import lol.pony.dubstepdishwasher.model.*
import lol.pony.dubstepdishwasher.model.core.*

import androidx.lifecycle.ViewModel
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.MutableStateFlow

class EffectChainViewModel : ViewModel() {
    private val chain = EffectChain()
    private val _effects = MutableStateFlow<List<Effect>>(emptyList())
    val effects : StateFlow<List<Effect>> = _effects

    fun addEffect(type: EffectType) {
        chain.addEffect(type)
        _effects.value = chain.getAll()
    }

    fun removeEffect(effectId: Int) {
        chain.removeEffect(effectId)
        _effects.value = chain.getAll()
    }

    fun reorderEffect(effectId: Int, toIndex: Int) {
        chain.reorderEffect(effectId, toIndex)
        _effects.value = chain.getAll()
    }

    fun setParam(effectId: Int, paramId: Int, value: Any) {
        chain.setParam(effectId, paramId, value)
        _effects.value = chain.getAll() // FIXME: Probably also doesn't fucking update Compose
    }

    fun toggleBypass(effectId: Int) {
        chain.setBypass(effectId, !chain.get(effectId)!!.isBypassed)
        _effects.value = chain.getAll() // FIXME: Doesn't fucking update Compose
    }
}
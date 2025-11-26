package lol.pony.dubstepdishwasher.model.core

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue

abstract class Effect {
    abstract val effectId: Int
    abstract val effectType: EffectType
    abstract val resourceUsage: ResourceUsage
    abstract val parameters: MutableList<EffectParameter>
    var isBypassed by mutableStateOf(false)
        private set

    @Suppress("UNCHECKED_CAST")
    fun setParam(paramId: Int, value: Any) {
        val param = parameters.find { it.id == paramId } ?: return
        param.setValueAny(value)
    }
    fun setBypass(state: Boolean) { isBypassed = state }

    fun getParam(paramId: Int) : EffectParameter? { return parameters.find { it.id == paramId }
    }
}
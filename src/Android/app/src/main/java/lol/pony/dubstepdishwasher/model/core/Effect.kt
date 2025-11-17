package lol.pony.dubstepdishwasher.model.core

abstract class Effect {
    abstract val effectId: Int
    abstract val effectType: EffectType
    abstract val parameters: MutableList<EffectParameter<*>>
    var isBypassed = false

    @Suppress("UNCHECKED_CAST")
    fun setParam(paramId: Int, value: Any) {
        val param = parameters.find { it.id == paramId } as? EffectParameter<Any> ?: return
        param.value = value
    }
    fun setBypass(state: Boolean) { isBypassed = state }

    fun getParam(paramId: Int) : EffectParameter<*>? { return parameters.find { it.id == paramId } as? EffectParameter<*> }
}
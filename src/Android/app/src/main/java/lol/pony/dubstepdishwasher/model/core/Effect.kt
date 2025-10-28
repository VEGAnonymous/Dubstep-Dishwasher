package lol.pony.dubstepdishwasher.model.core

abstract class Effect {
    abstract val effectId: Int
    abstract val effectType: EffectType
    abstract val parameters: MutableList<EffectParameter<*>>
    var isBypassed = false

    @Suppress("UNCHECKED_CAST")
    fun setParam(paramId: Int, value: Any) {
        val param = parameters.find { it.id == paramId } ?: return
        when (param) {
            is EffectParameter.Range<*> -> (param as EffectParameter.Range<Float>).value = value as Float
            is EffectParameter.Discrete<*> -> (param as EffectParameter.Discrete<Any>).value = value
            is EffectParameter.Toggle -> param.value = value as Boolean
        }
    }
    fun setBypass(state: Boolean) { isBypassed = state }
}
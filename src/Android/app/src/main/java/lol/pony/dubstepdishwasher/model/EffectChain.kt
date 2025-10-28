package lol.pony.dubstepdishwasher.model

import lol.pony.dubstepdishwasher.model.core.*
import lol.pony.dubstepdishwasher.model.effects.*

class EffectChain {
    private var nextIdx = 0
    private val effects = mutableListOf<Effect>()
    private val effectInits = mapOf(
        EffectType.DISTORTION to { id: Int -> Distortion(id) },
        EffectType.DELAY to { id: Int -> Delay(id) },
        EffectType.FLANGER to { id: Int -> Flanger(id) },
        EffectType.PHASER to { id: Int -> Phaser(id) },
        EffectType.CHORUS to { id: Int -> Chorus(id) },
        EffectType.REVERB to { id: Int -> Reverb(id) },
        EffectType.COMPRESSOR to { id: Int -> Compressor(id) },
        EffectType.EQUALIZER to { id: Int -> Equalizer(id) },
        EffectType.GRANULATOR to { id: Int -> Granulator(id) },
        EffectType.SPECTRAL_GATE to { id: Int -> SpectralGate(id) }
    )

    fun get(effectId: Int): Effect? = effects.find { it.effectId == effectId }
    fun getAll(): List<Effect> = effects
    fun indexOf(effectId: Int): Int = effects.indexOfFirst { it.effectId == effectId }
    fun reorder(posFrom: Int, posTo: Int) {
        val effect = effects.removeAt(posFrom)
        effects.add(posTo.coerceIn(0, effects.size), effect)
    }

    fun addEffect(type: EffectType) { effectInits[type]?.invoke(nextIdx++)?.let { effects.add(it) } }
    fun removeEffect(effectId: Int) = effects.removeIf { it.effectId == effectId }
    fun reorderEffect(effectId: Int, toIndex: Int) { reorder(indexOf(effectId), toIndex) }
    fun setParam(effectId: Int, paramId: Int, value: Any) { effects.find { it.effectId == effectId }?.setParam(paramId, value) }
    fun setBypass(effectId: Int, state: Boolean) { effects.find { it.effectId == effectId }?.isBypassed = state }
}
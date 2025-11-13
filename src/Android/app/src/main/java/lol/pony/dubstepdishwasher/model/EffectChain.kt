package lol.pony.dubstepdishwasher.model

import androidx.compose.ui.text.Paragraph
import androidx.compose.runtime.toMutableStateList
import lol.pony.dubstepdishwasher.model.core.*
import lol.pony.dubstepdishwasher.model.effects.*

class EffectChain {
    private var nextIdx = 0
    private val effects = mutableListOf<Effect>()
    private val effectInits = mapOf(
        EffectType.CHORUS to { id: Int -> Chorus(id) },
        EffectType.COMPRESSOR to { id: Int -> Compressor(id) },
        EffectType.DELAY to { id: Int -> Delay(id) },
        EffectType.DISTORTION to { id: Int -> Distortion(id) },
        EffectType.EQUALIZER to { id: Int -> Equalizer(id) },
        EffectType.FLANGER to { id: Int -> Flanger(id) },
        EffectType.FORMANT_SHIFTER to { id: Int -> FormantShifter(id) },
        EffectType.FREEZER to { id: Int -> Freezer(id) },
        EffectType.GAIN to { id: Int -> Gain(id) },
        EffectType.GATE to { id: Int -> Gate(id) },
        EffectType.GRANULATOR to { id: Int -> Granulator(id) },
        EffectType.MODULATION to { id: Int -> Modulation(id) },
        EffectType.PARALLEL to { id: Int -> Parallel(id) },
        EffectType.PHASER to { id: Int -> Phaser(id) },
        EffectType.PITCH_SHIFTER to { id: Int -> PitchShifter(id) },
        EffectType.REVERB to { id: Int -> Reverb(id) },
        EffectType.SCRUBBY to { id: Int -> Scrubby(id) },
        EffectType.SPECTRAL_GATE to { id: Int -> SpectralGate(id) },
        EffectType.VOCODER to { id: Int -> Vocoder(id) },
        EffectType.WAH to { id: Int -> Wah(id) }
    )

    fun get(effectId: Int): Effect? = effects.find { it.effectId == effectId }
    fun getAll(): List<Effect> = effects.toMutableStateList()
    fun indexOf(effectId: Int): Int = effects.indexOfFirst { it.effectId == effectId }
    fun reorder(posFrom: Int, posTo: Int) {
        val effect = effects.removeAt(posFrom)
        effects.add(posTo.coerceIn(0, effects.size), effect)
    }

    fun addEffect(type: EffectType) { effectInits[type]?.invoke(nextIdx++)?.let { effects.add(it) } }
    fun removeEffect(effectId: Int) = effects.removeIf { it.effectId == effectId }
    fun reorderEffect(effectId: Int, toIndex: Int) { reorder(indexOf(effectId), toIndex) }
    fun setParam(effectId: Int, paramId: Int, value: Any) { effects.find { it.effectId == effectId }?.setParam(paramId, value) }
    fun setBypass(effectId: Int, state: Boolean) { effects.find { it.effectId == effectId }?.setBypass(state) }
}
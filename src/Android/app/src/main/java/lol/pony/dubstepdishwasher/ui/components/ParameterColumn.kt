package lol.pony.dubstepdishwasher.ui.components

import androidx.compose.animation.core.tween
import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.dp
import lol.pony.dubstepdishwasher.R
import lol.pony.dubstepdishwasher.model.core.Effect
import lol.pony.dubstepdishwasher.model.core.EffectType
import lol.pony.dubstepdishwasher.model.core.ModAssignment
import lol.pony.dubstepdishwasher.model.core.Modulator
import lol.pony.dubstepdishwasher.model.core.ParamKey
import lol.pony.dubstepdishwasher.ui.components.subcomponents.CompressorParameterList
import lol.pony.dubstepdishwasher.ui.components.subcomponents.DefaultParameterList
import lol.pony.dubstepdishwasher.ui.components.subcomponents.DistortionParameterList
import lol.pony.dubstepdishwasher.ui.components.subcomponents.EqualizerParameterList
import lol.pony.dubstepdishwasher.ui.components.subcomponents.GateParameterList
import lol.pony.dubstepdishwasher.ui.components.subcomponents.ParallelParameterList

@Composable
fun ParameterColumn(
    effects: List<Effect>,
    assignments: List<ModAssignment>,
    selectedModulator: Modulator?,
    currentModOffsets: Map<ParamKey, Float>,
    // Callbacks
    onSetParam: (Int, Int, Any) -> Unit,
    onAssignMod: (String, Int, Int) -> Unit,
    onRemoveMod: (String, Int, Int) -> Unit,
    onModAmountChange: (String, Int, Int, Float) -> Unit,
    onTogglePolarity: (String, Int, Int) -> Unit,
    onOpenParallelEditor: (Int) -> Unit // Parallel editor
) {
    LazyColumn {
        items(effects, key = { it.effectId }) { effect ->
            Row (
                modifier = Modifier
                    .fillMaxWidth()
                    .height(100.dp)
                    .padding(horizontal = 8.dp, vertical = 4.dp)
                    .animateItem(
                        fadeInSpec = tween(durationMillis = 200),
                        fadeOutSpec = tween(durationMillis = 200),
                    ),
                verticalAlignment = Alignment.CenterVertically
            ) {
                val iconRes = effectLabel(effect.effectType)

                if (iconRes != null) {
                    Image(
                        painter = painterResource(iconRes),
                        contentDescription = effect.effectType.uiName,
                        modifier = Modifier
                            .width(150.dp)
                            .fillMaxHeight()
                            .padding(end = 8.dp)
                    )
                } else {
                    Text(
                        text = effect.effectType.uiName,
                        style = MaterialTheme.typography.headlineMedium,
                        modifier = Modifier.width(150.dp)
                    )
                }

                when (effect.effectType) {
                    EffectType.COMPRESSOR -> CompressorParameterList(
                        effect = effect,
                        assignments = assignments,
                        selectedModulator = selectedModulator,
                        currentModOffsets = currentModOffsets,
                        onSetParam = onSetParam,
                        onAssignMod = onAssignMod,
                        onRemoveMod = onRemoveMod,
                        onModAmountChange = onModAmountChange,
                        onTogglePolarity = onTogglePolarity
                    )

                    EffectType.DISTORTION -> DistortionParameterList(
                        effect = effect,
                        assignments = assignments,
                        selectedModulator = selectedModulator,
                        currentModOffsets = currentModOffsets,
                        onSetParam = onSetParam,
                        onAssignMod = onAssignMod,
                        onRemoveMod = onRemoveMod,
                        onModAmountChange = onModAmountChange,
                        onTogglePolarity = onTogglePolarity
                    )

                    EffectType.EQUALIZER -> EqualizerParameterList (
                        effect = effect,
                        assignments = assignments,
                        selectedModulator = selectedModulator,
                        currentModOffsets = currentModOffsets,
                        onSetParam = onSetParam,
                        onAssignMod = onAssignMod,
                        onRemoveMod = onRemoveMod,
                        onModAmountChange = onModAmountChange,
                        onTogglePolarity = onTogglePolarity
                    )

                    EffectType.GATE -> GateParameterList(
                        effect = effect,
                        assignments = assignments,
                        selectedModulator = selectedModulator,
                        currentModOffsets = currentModOffsets,
                        onSetParam = onSetParam,
                        onAssignMod = onAssignMod,
                        onRemoveMod = onRemoveMod,
                        onModAmountChange = onModAmountChange,
                        onTogglePolarity = onTogglePolarity
                    )

                    EffectType.PARALLEL -> ParallelParameterList(
                        effect = effect,
                        assignments = assignments,
                        selectedModulator = selectedModulator,
                        currentModOffsets = currentModOffsets,
                        onSetParam = onSetParam,
                        onAssignMod = onAssignMod,
                        onRemoveMod = onRemoveMod,
                        onModAmountChange = onModAmountChange,
                        onTogglePolarity = onTogglePolarity,
                        onOpenEditor = { onOpenParallelEditor(effect.effectId) }
                    )

                    // Default parameter list rows
                    else -> DefaultParameterList(
                        effect = effect,
                        assignments = assignments,
                        selectedModulator = selectedModulator,
                        currentModOffsets = currentModOffsets,
                        onSetParam = onSetParam,
                        onAssignMod = onAssignMod,
                        onRemoveMod = onRemoveMod,
                        onModAmountChange = onModAmountChange,
                        onTogglePolarity = onTogglePolarity
                    )
                }
            }

            HorizontalDivider()
        }
    }
}

@Composable
fun effectLabel(effectType: EffectType): Int? {
    return when (effectType) {
        EffectType.CHORUS -> R.drawable.label_fx_chorus
        EffectType.COMPRESSOR -> R.drawable.label_fx_compressor
        EffectType.DELAY -> R.drawable.label_fx_delay
        EffectType.DISTORTION -> R.drawable.label_fx_distortion
        EffectType.EQUALIZER -> R.drawable.label_fx_equalizer
        EffectType.FLANGER -> R.drawable.label_fx_flanger
        EffectType.FORMANT_SHIFTER -> R.drawable.label_fx_formantshifter
        EffectType.FREEZER -> R.drawable.label_fx_freezer
        EffectType.GAIN -> R.drawable.label_fx_gain
        EffectType.GATE -> R.drawable.label_fx_gate
        EffectType.GRANULATOR -> R.drawable.label_fx_granulator
        EffectType.MODULATION -> R.drawable.label_fx_modulation
        EffectType.PARALLEL -> R.drawable.label_fx_parallel
        EffectType.PHASER -> R.drawable.label_fx_phaser
        EffectType.PITCH_SHIFTER -> R.drawable.label_fx_pitchshifter
        EffectType.REVERB -> R.drawable.label_fx_reverb
        EffectType.SCRUBBY -> R.drawable.label_fx_scrubby
        EffectType.SPECTRAL_GATE -> R.drawable.label_fx_spectralgate
        EffectType.VOCODER -> R.drawable.label_fx_vocoder
        EffectType.WAH -> R.drawable.label_fx_wah
    }
}
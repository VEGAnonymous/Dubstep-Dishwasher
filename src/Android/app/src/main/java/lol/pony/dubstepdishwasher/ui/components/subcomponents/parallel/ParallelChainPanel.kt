package lol.pony.dubstepdishwasher.ui.components.subcomponents.parallel

import android.media.MediaPlayer
import androidx.compose.animation.core.tween
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Close
import androidx.compose.material.icons.filled.KeyboardArrowDown
import androidx.compose.material.icons.filled.KeyboardArrowUp
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.scale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import lol.pony.dubstepdishwasher.R
import lol.pony.dubstepdishwasher.model.core.Effect
import lol.pony.dubstepdishwasher.model.core.EffectType
import lol.pony.dubstepdishwasher.model.core.ParallelChain
import lol.pony.dubstepdishwasher.model.core.lerp
import lol.pony.dubstepdishwasher.ui.components.resourceColor
import lol.pony.dubstepdishwasher.ui.components.subcomponents.CompressorParameterList
import lol.pony.dubstepdishwasher.ui.components.subcomponents.DefaultParameterList
import lol.pony.dubstepdishwasher.ui.components.subcomponents.DistortionParameterList
import lol.pony.dubstepdishwasher.ui.components.subcomponents.EqualizerParameterList
import lol.pony.dubstepdishwasher.ui.components.subcomponents.GateParameterList
import lol.pony.dubstepdishwasher.ui.controls.DDSwitch

val MAX_PARALLEL_EFFECTS : Int? = null // Arbitrary limit, deprecated

@Composable
fun ParallelChainPanel(
    modifier: Modifier = Modifier,
    chain: ParallelChain,
    effects: List<Effect>,
    id: Int,
    onParallelAdd: (Int, ParallelChain, EffectType) -> Unit,
    onParallelRemove: (Int, ParallelChain, Int) -> Unit,
    onParallelReorder: (Int, ParallelChain, Int, Int) -> Unit,
    onParallelBypass: (Int, ParallelChain, Int) -> Unit,
    onParallelSetParam: (Int, ParallelChain, Int, Int, Any) -> Unit,
    resourceError: String?,
    onDismissError: () -> Unit
) {
    val count = effects.size

    Column(
        modifier = modifier.fillMaxHeight(),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        /* HEADER */
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(start = 16.dp, end = 8.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text(
                text = "Chain ${if (chain == ParallelChain.A) "A" else "B"}",
                style = MaterialTheme.typography.headlineSmall
            )

            /* ADD FX */
            var expanded by remember { mutableStateOf(false) }

            Box {
                IconButton(
                    onClick = { expanded = !expanded },
                    enabled = MAX_PARALLEL_EFFECTS?.let { count < it } ?: true
                ) {
                    Icon(Icons.Filled.Add, contentDescription = "Add FX")
                }
                DropdownMenu(expanded = expanded, onDismissRequest = { expanded = false }) {
                    val entries = EffectType.entries.filterNot { it == EffectType.PARALLEL }
                    entries.forEach { type ->
                        val compute = type.resourceUsage.compute; val memory = type.resourceUsage.memory

                        // Compute
                        val computeRatio = (compute / 0.20f).coerceIn(0f, 1f)
                        val computeDotColor = resourceColor(computeRatio)
                        val computeDotSize = lerp(8.dp, 16.dp, computeRatio)
                        // Memory
                        val memoryRatio = (memory.toFloat() / 200f).coerceIn(0f, 1f)
                        val memoryDotColor = resourceColor(memoryRatio)
                        val memoryDotSize = lerp(8.dp, 16.dp, memoryRatio)

                        DropdownMenuItem(
                            text = {
                                Row(verticalAlignment = Alignment.CenterVertically) {
                                    Text(
                                        text = type.uiName,
                                        style = MaterialTheme.typography.bodyMedium,
                                        modifier = Modifier.width(130.dp)
                                    )
                                    Spacer(Modifier.width(30.dp))
                                    Box(
                                        modifier = Modifier
                                            .size(computeDotSize)
                                            .background(computeDotColor, shape = CircleShape)
                                    )
                                    Spacer(Modifier.width(8.dp))
                                    Box(
                                        modifier = Modifier
                                            .size(memoryDotSize)
                                            .background(memoryDotColor, shape = CircleShape)
                                    )
                                }
                            },
                            onClick = {
                                onParallelAdd(id, chain, type)
                                expanded = false
                            }
                        )
                    }
                }
            }
        }

        HorizontalDivider(modifier = Modifier.padding(bottom = 4.dp))

        /* SLOTS */
        LazyColumn(modifier = Modifier.fillMaxHeight()) {
            items(
                count = count,
                key = { index -> effects[index].effectId }
            ) { index ->
                val effect = effects[index]
                val effectId = effect.effectId
                Column (modifier = Modifier
                    .fillMaxSize()
                    .animateItem(
                        fadeInSpec = tween(durationMillis = 200),
                        fadeOutSpec = tween(durationMillis = 200),
                    )
                ) {
                    SlotEditor(
                        index = index,
                        effect = effect,
                        effects = effects,
                        onRemove = { onParallelRemove(id, chain, effectId) },
                        onReorder = { toIndex -> onParallelReorder(id, chain, effectId, toIndex) },
                        onBypass = { fxId -> onParallelBypass(id, chain, fxId) },
                        onSetParam = { paramId, value -> onParallelSetParam(id, chain, effectId, paramId, value) }
                    )
                    HorizontalDivider(modifier = Modifier.padding(bottom = 2.dp))
                }
            }
        }

        if (resourceError != null) { // Exceeded resource limit
            val context = LocalContext.current
            LaunchedEffect(Unit) {
                val mediaPlayer = MediaPlayer.create(context, R.raw.ohno) // :pinkiesad:
                mediaPlayer.setOnCompletionListener { it.release() }
                mediaPlayer.start()
            }
            AlertDialog(
                onDismissRequest = { onDismissError() },
                title = { Text(text = "Oh no!", style = MaterialTheme.typography.bodyMedium) }, // :(
                text = { Text(text = resourceError, style = MaterialTheme.typography.bodyMedium) },
                confirmButton = {
                    TextButton(onClick = { onDismissError() }) {
                        Text(text = "OK", style = MaterialTheme.typography.bodyMedium)
                    }
                }
            )
        }
    } // Column
} // ParallelChainPanel

@Composable
private fun SlotEditor(
    index: Int,
    effects: List<Effect>,
    effect: Effect,
    onRemove: () -> Unit,
    onReorder: (Int) -> Unit,
    onBypass: (Int) -> Unit,
    onSetParam: (Int, Any) -> Unit
) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(6.dp)
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .height(30.dp)
                .padding(start = 8.dp, end = 4.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {

            /* FX NAME */
            Text(
                text = effect.effectType.uiName,
                style = MaterialTheme.typography.headlineSmall,
                modifier = Modifier.width(140.dp),
                maxLines = 1
            )

            Spacer(modifier = Modifier.width(30.dp))

            /* SLOT CONTROLS */
            Row(verticalAlignment = Alignment.CenterVertically) {
                Row(
                    modifier = Modifier.padding(start = 6.dp, end = 4.dp),
                    verticalAlignment = Alignment.CenterVertically,
                ) {
                    // REORDER
                    IconButton(
                        onClick = { onReorder(index - 1) },
                        enabled = index > 0,
                        modifier = Modifier.width(30.dp).padding(horizontal = 4.dp)
                    ) { Icon(Icons.Filled.KeyboardArrowUp, contentDescription = "Move Up") }

                    IconButton(
                        onClick = { onReorder(index + 1) },
                        enabled = (MAX_PARALLEL_EFFECTS?.let { index < it - 1 } ?: true) && (index != effects.size - 1),
                        modifier = Modifier.width(30.dp).padding(horizontal = 4.dp)
                    ) { Icon(Icons.Filled.KeyboardArrowDown, contentDescription = "Move Down") }

                    Spacer(modifier = Modifier.width(20.dp))

                    // BYPASS
                    DDSwitch(
                        modifier = Modifier.scale(0.8f).padding(end = 8.dp),
                        checked = effect.isBypassed,
                        onCheckedChange = { onBypass(effect.effectId) },
                        imageRes = R.drawable.control_bypass
                    )
                }

                // REMOVE
                IconButton(
                    onClick = onRemove,
                    modifier = Modifier.width(40.dp).scale(0.8f)
                ) { Icon(Icons.Filled.Close, contentDescription = "Remove FX") }
            }
        }

        Spacer(modifier = Modifier.height(10.dp))

        /* PARAMETER LIST */
        ParallelParameterListUI(effect = effect, onSetParam = onSetParam)
    } // Column
} // SlotEditor

@Composable
private fun ParallelParameterListUI(
    effect: Effect,
    onSetParam: (Int, Any) -> Unit
) {
    when (effect.effectType) {
        EffectType.COMPRESSOR -> CompressorParameterList(
            effect = effect,
            assignments = emptyList(),
            selectedModulator = null,
            currentModOffsets = emptyMap(),
            onSetParam = { _, paramId, value -> onSetParam(paramId, value) },
            onAssignMod = { _, _, _ -> },
            onRemoveMod = { _, _, _ -> },
            onModAmountChange = { _, _, _, _ -> },
            onTogglePolarity = { _, _, _ -> }
        )
        EffectType.DISTORTION -> DistortionParameterList(
            effect = effect,
            assignments = emptyList(),
            selectedModulator = null,
            currentModOffsets = emptyMap(),
            onSetParam = { _, paramId, value -> onSetParam(paramId, value) },
            onAssignMod = { _, _, _ -> },
            onRemoveMod = { _, _, _ -> },
            onModAmountChange = { _, _, _, _ -> },
            onTogglePolarity = { _, _, _ -> }
        )
        EffectType.EQUALIZER -> EqualizerParameterList(
            effect = effect,
            assignments = emptyList(),
            selectedModulator = null,
            currentModOffsets = emptyMap(),
            onSetParam = { _, paramId, value -> onSetParam(paramId, value) },
            onAssignMod = { _, _, _ -> },
            onRemoveMod = { _, _, _ -> },
            onModAmountChange = { _, _, _, _ -> },
            onTogglePolarity = { _, _, _ -> }
        )
        EffectType.GATE -> GateParameterList(
            effect = effect,
            assignments = emptyList(),
            selectedModulator = null,
            currentModOffsets = emptyMap(),
            withinParallel = true,
            onSetParam = { _, paramId , value -> onSetParam(paramId, value) },
            onAssignMod = { _, _, _ -> },
            onRemoveMod = { _, _, _ -> },
            onModAmountChange = { _, _, _, _ -> },
            onTogglePolarity = { _, _, _ -> }
        )
        else -> DefaultParameterList(
            effect = effect,
            assignments = emptyList(),
            selectedModulator = null,
            currentModOffsets = emptyMap(),
            withinParallel = true,
            onSetParam = { _, paramId, value -> onSetParam(paramId, value) },
            onAssignMod = { _, _, _ -> },
            onRemoveMod = { _, _, _ -> },
            onModAmountChange = { _, _, _, _ -> },
            onTogglePolarity = { _, _, _ -> }
        )
    } // when
} // ParallelParameterListUI
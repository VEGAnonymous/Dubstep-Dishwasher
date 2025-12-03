package lol.pony.dubstepdishwasher.ui

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.VerticalDivider
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.scale
import androidx.compose.ui.unit.dp
import lol.pony.dubstepdishwasher.model.core.EditorState
import lol.pony.dubstepdishwasher.model.core.Modulator
import lol.pony.dubstepdishwasher.ui.components.EffectChainControls
import lol.pony.dubstepdishwasher.ui.components.EffectList
import lol.pony.dubstepdishwasher.ui.components.ModulatorControls
import lol.pony.dubstepdishwasher.ui.components.ModulatorTabs
import lol.pony.dubstepdishwasher.ui.components.ParameterColumn
import lol.pony.dubstepdishwasher.ui.components.subcomponents.CurveEditor
import lol.pony.dubstepdishwasher.ui.components.subcomponents.parallel.ParallelEditor
import lol.pony.dubstepdishwasher.viewmodel.MainViewModel

enum class LeftColumnMode { FX, MOD }

@Composable
fun MainPanel(mainViewModel: MainViewModel) {
    val resourceError = mainViewModel.resourceError.collectAsState().value

    val effects by mainViewModel.effects.collectAsState()
    val modulators by mainViewModel.modulators.collectAsState()

    val currentModValues by mainViewModel.currentModValues.collectAsState()
    val currentModOffsets by mainViewModel.currentModOffsets.collectAsState()

    val parallelChains by mainViewModel.parallelChains.collectAsState()

    val curveEditorStates by mainViewModel.editorStates.collectAsState()
    var curveEditorOpen by remember { mutableStateOf<Modulator?>(null) }
    val curvePresets by mainViewModel.curvePresets.collectAsState()

    var parallelEditorOpen by remember { mutableStateOf<Int?>(null) } // Parallel effect ID

    Box(modifier = Modifier.fillMaxSize()) {
        Row(modifier = Modifier.fillMaxSize()) {
            // LEFT COLUMN
            var leftColumnMode by remember { mutableStateOf(LeftColumnMode.FX) }
            var selectedModulator by remember { mutableStateOf<Modulator?>(null) }
            LaunchedEffect(modulators) {
                // if nothing selected, pick first
                if (selectedModulator == null && modulators.isNotEmpty()) {
                    selectedModulator = modulators.first()
                } else {
                    selectedModulator?.let { sel ->
                        val updated = modulators.find { it.id == sel.id }
                        if (updated != null && updated !== sel) {
                            selectedModulator = updated
                        }
                    }
                }
            }

            Column(
                Modifier.width(250.dp)
                    .fillMaxHeight()
            ) {
                Row(
                    Modifier.width(250.dp)
                        .padding(vertical = 2.dp)
                        .scale(0.8f),
                    horizontalArrangement = Arrangement.Center
                )
                { // Select which tab
                    ModeToggle(
                        currentMode = leftColumnMode,
                        onModeChange = { leftColumnMode = it }
                    )
                }

                // HorizontalDivider()
                // Spacer(modifier = Modifier.height(2.dp))

                when (leftColumnMode) {
                    LeftColumnMode.FX ->
                        // Chain controls + effect list
                        Column(Modifier.width(250.dp)) {
                            EffectChainControls(
                                onAdd = { mainViewModel.addEffect(it) },
                                onClear = { mainViewModel.clearChain() },
                                resourceError = resourceError,
                                onDismissError = { mainViewModel.clearResourceError() }
                            )
                            HorizontalDivider()
                            EffectList(
                                effects = effects,
                                onToggleBypass = { id -> mainViewModel.toggleBypass(id) },
                                onRemove = { id -> mainViewModel.removeEffect(id) },
                                onReorder = { id, toIndex ->
                                    mainViewModel.reorderEffect(
                                        id,
                                        toIndex
                                    )
                                }
                            )
                        }

                    LeftColumnMode.MOD ->
                        // Modulators
                        Column(Modifier.width(250.dp)) {
                            HorizontalDivider()
                            ModulatorTabs( // Mod tab selection
                                modulators = mainViewModel.modulators.collectAsState().value,
                                assignments = mainViewModel.modAssignments.collectAsState().value,
                                selectedMod = selectedModulator,
                                onSelected = { selectedModulator = it }
                            )
                            HorizontalDivider()
                            selectedModulator?.let { mod ->
                                ModulatorControls( // Display controls for selected mod
                                    mod = mod,
                                    currentModValues = currentModValues,
                                    onSetParam = { modId, paramId, value -> mainViewModel.setModulatorParam(modId, paramId, value) },
                                    onEditCurve = { curveEditorOpen = mod }
                                )
                            }
                        } // Modulators column
                } // Left column mode
            } // Column

            VerticalDivider()

            // RIGHT COLUMN
            // Chain parameter lists
            ParameterColumn(
                effects = effects,
                assignments = mainViewModel.modAssignments.collectAsState().value,
                selectedModulator = selectedModulator,
                currentModOffsets = currentModOffsets,
                onSetParam = { id, param, value -> mainViewModel.setParam(id, param, value) },
                onAssignMod = { modId, effectId, paramId -> mainViewModel.addAssignment(modId, effectId, paramId) },
                onRemoveMod = { modId, effectId, paramId -> mainViewModel.removeAssignment(modId, effectId, paramId) },
                onModAmountChange = { modId, effectId, paramId, amount -> mainViewModel.updateAssignmentAmount(modId, effectId, paramId, amount) },
                onTogglePolarity = { modId, effectId, paramId -> mainViewModel.updateAssignmentPolarity(modId, effectId, paramId) },
                onOpenParallelEditor = { parallelEffectId -> parallelEditorOpen = parallelEffectId }
            )
        } // Row

        // Curve editor overlay
        curveEditorOpen?.let { mod ->
            val modId = mod.id
            val editorState = curveEditorStates[modId] ?: EditorState()

            val currentPos = when (mod) {
                is Modulator.LFO -> mod.phase
                is Modulator.Mapping -> mod.inputValue
            }

            CurveEditor(
                modulator = mod,
                currentPosition = currentPos,
                editorState = editorState,
                onStateChange = { state -> mainViewModel.updateEditorState(modId, state) },
                curvePresets = curvePresets,
                onSavePreset = { name, category, points -> mainViewModel.saveCurvePreset(name, category, points) },
                onDeletePreset = { name -> mainViewModel.deleteCurvePreset(name) },
                onFavoritePreset = { name, favorite -> mainViewModel.favoriteCurvePreset(name, favorite) },
                onDismiss = { curveEditorOpen = null },
                onSave = { curve ->
                    mainViewModel.updateModulatorCurve(modId, curve)
                    curveEditorOpen = null
                }

            )
        }

        // Parallel editor overlay
        parallelEditorOpen?.let { parallelId ->
            ParallelEditor(
                id = parallelId,
                parallelChains = parallelChains,
                resourceError = resourceError,
                onDismissError = { mainViewModel.clearResourceError() },
                onDismiss = { parallelEditorOpen = null },
                onParallelAdd = { id, chain, type -> mainViewModel.parallelAddEffect(id, chain, type) },
                onParallelRemove = { id, chain, effectId -> mainViewModel.parallelRemoveEffect(id, chain, effectId) },
                onParallelReorder = { id, chain, effectId, toIndex -> mainViewModel.parallelReorderEffect(id, chain, effectId, toIndex) },
                onParallelBypass = { id, chain, effectId -> mainViewModel.parallelBypassEffect(id, chain, effectId) },
                onParallelSetParam = { id, chain, effectId, paramId, value -> mainViewModel.parallelSetParam(id, chain, effectId, paramId, value) }
            )
        }
    } // Box
} // MainPanel

@Composable
fun ModeToggle(
    currentMode: LeftColumnMode,
    onModeChange: (LeftColumnMode) -> Unit
) {
    Row(
        Modifier.width(250.dp)
            .padding(vertical = 2.dp)
            .scale(0.8f),
        horizontalArrangement = Arrangement.Center
    ) {
        LeftColumnMode.entries.forEach { mode ->
            val active = (currentMode == mode)
            Surface(
                modifier = Modifier
                    .padding(horizontal = 4.dp)
                    .height(36.dp)
                    .weight(1f)
                    .clickable { onModeChange(mode) },
                color = if (active) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.surfaceVariant,
                shape = RoundedCornerShape(4.dp),
                shadowElevation = if (active) 0.dp else 4.dp
            ) {
                Box(contentAlignment = Alignment.Center) {
                    Text(
                        text = mode.name,
                        style = MaterialTheme.typography.labelLarge
                    )
                }
            }
        }
    }
}
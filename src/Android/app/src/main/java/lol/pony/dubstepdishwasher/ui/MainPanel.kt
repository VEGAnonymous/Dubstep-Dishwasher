package lol.pony.dubstepdishwasher.ui

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.material3.Button
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.VerticalDivider
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.scale
import androidx.compose.ui.unit.dp
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewmodel.compose.viewModel
import lol.pony.dubstepdishwasher.model.BLEManager
import lol.pony.dubstepdishwasher.model.core.*
import lol.pony.dubstepdishwasher.ui.components.*
import lol.pony.dubstepdishwasher.viewmodel.MainViewModel

enum class LeftColumnMode { FX, MOD }

class MainViewModelFactory(private val bleManager: BLEManager) : ViewModelProvider.Factory {
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        if (modelClass.isAssignableFrom(MainViewModel::class.java)) {
            @Suppress("UNCHECKED_CAST")
            return MainViewModel(bleManager) as T
        }
        throw IllegalArgumentException("Unknown ViewModel class")
    }
}

@Composable
fun MainPanel(bleManager: BLEManager) {
    val mainViewModel: MainViewModel = viewModel(factory = MainViewModelFactory(bleManager))
    val effects by mainViewModel.effects.collectAsState()

    val currentModValues by mainViewModel.currentModValues.collectAsState()
    val currentModOffsets by mainViewModel.currentModOffsets.collectAsState()

    val editorStates by mainViewModel.editorStates.collectAsState()
    var editorOpen by remember { mutableStateOf<Modulator?>(null) }
    val curvePresets by mainViewModel.curvePresets.collectAsState()

    Box(modifier = Modifier.fillMaxSize()) {
        Row(modifier = Modifier.fillMaxSize()) {
            // LEFT COLUMN
            var leftColumnMode by remember { mutableStateOf(LeftColumnMode.FX) }
            var selectedModulator by remember { mutableStateOf<Modulator?>(null) }

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
                    Button(
                        onClick = { leftColumnMode = LeftColumnMode.FX },
                        modifier = Modifier.padding(horizontal = 10.dp)
                    ) {
                        Text(text = "FX", style = MaterialTheme.typography.headlineSmall)
                    }
                    Button(
                        onClick = { leftColumnMode = LeftColumnMode.MOD },
                        modifier = Modifier.padding(horizontal = 10.dp)
                    ) {
                        Text("MOD", style = MaterialTheme.typography.headlineSmall)
                    }
                }

                HorizontalDivider()

                when (leftColumnMode) {
                    LeftColumnMode.FX ->
                        // Chain controls + effect list
                        Column(Modifier.width(250.dp)) {
                            EffectChainControls(
                                onAdd = { mainViewModel.addEffect(it) },
                                onClear = { mainViewModel.clearChain() })
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
                                    onEditCurve = { editorOpen = mod }
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
                onTogglePolarity = { modId, effectId, paramId -> mainViewModel.updateAssignmentPolarity(modId, effectId, paramId) }
            )
        } // Row

        // Curve editor overlay
        editorOpen?.let { mod ->
            val modId = mod.id
            val editorState = editorStates[modId] ?: EditorState()

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
                onSavePreset = { name, points -> mainViewModel.saveCurvePreset(name, points) },
                onDismiss = { editorOpen = null },
                onSave = { curve ->
                    mainViewModel.updateModulatorCurve(modId, curve)
                    editorOpen = null
                }
            )
        }

    } // Box
} // MainPanel
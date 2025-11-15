package lol.pony.dubstepdishwasher.ui

import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.IntrinsicSize
import androidx.compose.foundation.layout.Row
// import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.defaultMinSize
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.imePadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Close
// import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.KeyboardArrowDown
import androidx.compose.material.icons.filled.KeyboardArrowUp
import androidx.compose.material3.Button
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
// import androidx.compose.material3.Slider
// import androidx.compose.material3.SliderDefaults
// import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.VerticalDivider
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
// import androidx.compose.runtime.LaunchedEffect
// import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.scale
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewmodel.compose.viewModel
import lol.pony.dubstepdishwasher.model.BLEManager
import lol.pony.dubstepdishwasher.model.core.Effect
import lol.pony.dubstepdishwasher.model.core.EffectParameter
import lol.pony.dubstepdishwasher.model.core.EffectType
import lol.pony.dubstepdishwasher.model.core.UIEnum
import lol.pony.dubstepdishwasher.model.core.mapRange
import lol.pony.dubstepdishwasher.R
import lol.pony.dubstepdishwasher.ui.controls.*
import lol.pony.dubstepdishwasher.viewmodel.EffectChainViewModel
import kotlin.math.pow

class EffectChainViewModelFactory(private val bleManager: BLEManager) : ViewModelProvider.Factory {
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        if (modelClass.isAssignableFrom(EffectChainViewModel::class.java)) {
            @Suppress("UNCHECKED_CAST")
            return EffectChainViewModel(bleManager) as T
        }
        throw IllegalArgumentException("Unknown ViewModel class")
    }
}

@Composable
fun FXPanel(bleManager: BLEManager) {
    val viewModel: EffectChainViewModel = viewModel(factory = EffectChainViewModelFactory(bleManager))
    val effects by viewModel.effects.collectAsState()

    Row(Modifier.fillMaxSize()) {

        // Chain controls + effect list
        Column(
            Modifier.width(250.dp)
                    .fillMaxHeight()
        ) {
            EffectChainControls(
                onAdd = { viewModel.addEffect(it) },
                onClear = { viewModel.clearChain() } )

            HorizontalDivider()

            EffectList(
                effects = effects,
                onToggleBypass = { id -> viewModel.toggleBypass(id) },
                onRemove = { id -> viewModel.removeEffect(id) },
                onReorder = { id, toIndex -> viewModel.reorderEffect(id, toIndex) }
            )
        }

        VerticalDivider()

        // Chain parameter lists
        ParameterColumn(
            effects = effects,
            onSetParam = { id, param, value -> viewModel.setParam(id, param, value) }
        )
    }
}

@Composable
fun EffectChainControls(onAdd: (EffectType) -> Unit, onClear: () -> Unit) {
    Row (modifier = Modifier.padding(horizontal = 16.dp, vertical = 4.dp)) {
        var expanded by remember { mutableStateOf(false) }

        // Add effect button + dropdown
        Box {
            IconButton(onClick = { expanded = !expanded }) {
                Icon(Icons.Filled.Add, contentDescription = "Add FX")
            }
            DropdownMenu(expanded = expanded, onDismissRequest = { expanded = false }) {
                EffectType.entries.forEach { type ->
                    DropdownMenuItem(
                        text = { Text(
                            text = type.uiName,
                            style = MaterialTheme.typography.bodyMedium
                        )},
                        onClick = {
                            onAdd(type)
                            expanded = false
                        }
                    )
                }
            }
        }

        // Clear button
        Button(onClick = onClear, modifier = Modifier.padding(horizontal = 16.dp).scale(0.8f)) {
            Text("Clear", style = MaterialTheme.typography.bodyMedium)
        }
    }
}

@Composable
fun EffectList(
    effects: List<Effect>,
    onToggleBypass: (Int) -> Unit,
    onRemove: (Int) -> Unit,
    onReorder: (Int, Int) -> Unit,
) {
    LazyColumn {
        itemsIndexed(items = effects, key = { _, fx -> fx.effectId }) { index, fx ->
            Row (
                modifier = Modifier
                    .fillMaxWidth()
                    .height(40.dp)
                    .padding(horizontal = 16.dp, vertical = 8.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {

                // Effect name
                Column {
                    Text(fx.effectType.uiName, style = MaterialTheme.typography.headlineSmall, modifier = Modifier.width(130.dp))
                    // Text("Id: ${fx.effectId}")
                    // Text("Index: $index")
                }

                // Reorder effect buttons
                // Doesn't show the down arrow currently but I don't care lol
                // TEMP: Potentially switch to drag and drop
                Column (modifier = Modifier.fillMaxHeight()) {
                    IconButton(
                        onClick = { onReorder(fx.effectId, index - 1) },
                        enabled = index > 0
                    ) { Icon(Icons.Filled.KeyboardArrowUp, contentDescription = "Move Up") }

                    IconButton(
                        onClick = { onReorder(fx.effectId, index + 1) },
                        enabled = index < effects.size - 1
                    ) { Icon(Icons.Filled.KeyboardArrowDown, contentDescription = "Move Down") }
                }

                // Spacer(modifier = Modifier.width(40.dp))

                // Bypass switch
                DDSwitch(
                    modifier = Modifier.scale(1f),
                    checked = fx.isBypassed,
                    onCheckedChange = { onToggleBypass(fx.effectId) },
                    imageRes = R.drawable.control_bypass
                )

                // Remove effect button
                IconButton(onClick = { onRemove(fx.effectId) }) {
                    Icon(Icons.Filled.Close, contentDescription = "Remove") }
            }

            HorizontalDivider()

        }
    }
}

@Composable
fun ParameterColumn(
    effects: List<Effect>,
    onSetParam: (Int, Int, Any) -> Unit
) {
    LazyColumn {
        items(effects, key = { it.effectId }) { fx ->
            Row (
                modifier = Modifier
                    .fillMaxWidth()
                    .height(100.dp)
                    .padding(horizontal = 16.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {

                // TEMP: Generic text label per row; replace with cooler graphic later
                Text(
                    text = fx.effectType.uiName,
                    style = MaterialTheme.typography.headlineMedium,
                    modifier = Modifier.width(100.dp)
                )

                // Rest of param list
                ParameterList(
                    modifier = Modifier.fillMaxWidth(),
                    effect = fx,
                    onSetParam = onSetParam
                )
            }

            HorizontalDivider()
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ParameterList(
    modifier: Modifier = Modifier,
    effect: Effect,
    onSetParam: (Int, Int, Any) -> Unit
) {
    LazyRow(modifier.fillMaxWidth()) {
        items(items = effect.parameters.toList(), key = { it.id }) { param ->
            Column(
                horizontalAlignment = Alignment.CenterHorizontally,
                modifier = Modifier
                    .padding(horizontal = 8.dp)
                    .width(IntrinsicSize.Max)
                    .defaultMinSize(minWidth = 100.dp)
            ) {
                /* Parameter name */
                Text(
                    text = param.name,
                    style = MaterialTheme.typography.bodySmall,
                    modifier = Modifier.fillMaxWidth().padding(top = 4.dp),
                    textAlign = TextAlign.Center,
                    maxLines = 1,
                )

                // Dialog state for Range parameters
                var showDialog by remember { mutableStateOf(false) }
                var textValue by remember { mutableStateOf("") }

                /* Parameter value */
                Text(
                    text = param.formatValue(),
                    style = MaterialTheme.typography.bodySmall,
                    modifier = Modifier.pointerInput(Unit) {
                        detectTapGestures(
                            // Edit value (dialog) on tap
                            onTap = {
                                if (param is EffectParameter.Range) {
                                    textValue = param.value.toString()
                                    showDialog = true
                                }
                            }
                        )
                    }
                )

                /* Parameter input */
                when(param) {
                    is EffectParameter.Range -> {
                        /* Text input dialog */
                        if (showDialog) {
                            androidx.compose.material3.AlertDialog(
                                onDismissRequest = {
                                    showDialog = false
                                    textValue = param.value.toString()
                                },
                                title = { Text(param.name, style = MaterialTheme.typography.bodyMedium) },
                                text = {
                                    androidx.compose.material3.OutlinedTextField(
                                        value = textValue,
                                        onValueChange = { textValue = it },
                                        label = { Text("Value") },
                                        singleLine = true,
                                        modifier = Modifier.fillMaxWidth(),
                                        keyboardOptions = KeyboardOptions(
                                            keyboardType = KeyboardType.Decimal,
                                            imeAction = ImeAction.Done
                                        )
                                    )
                                },
                                confirmButton = {
                                    androidx.compose.material3.TextButton(
                                        onClick = {
                                            textValue.toFloatOrNull()?.let { newValue ->
                                                val clamped = newValue.coerceIn( // Clamp user input
                                                    param.range.first.toFloat(),
                                                    param.range.second.toFloat()
                                                )
                                                param.fromNormalized( // Set internal value from normalized position
                                                    clamped.mapRange(
                                                        inRange = param.range.first.toFloat()..param.range.second.toFloat(),
                                                        outRange = 0f..1f
                                                    ).pow(1 / param.exp)
                                                )
                                                onSetParam(effect.effectId, param.id, param.value)
                                            }
                                            showDialog = false
                                        }
                                    ) { Text("OK") }
                                },
                                dismissButton = null,
                                modifier = Modifier
                                    .fillMaxWidth(0.8f)
                                    .padding(horizontal = 32.dp)
                                    .imePadding()
                            )
                        }

                        // Knob for range parameters
                        DDKnob(
                            modifier = Modifier
                                .width(44.dp)
                                .padding(vertical = 8.dp)
                                .pointerInput(Unit) {
                                    detectTapGestures(
                                        onDoubleTap = {
                                            param.fromNormalized(
                                                param.initialValue
                                                    .mapRange(
                                                        inRange = param.range.first.toFloat()..param.range.second.toFloat(),
                                                        outRange = 0f..1f
                                                    )
                                                    .pow(1f / param.exp)
                                            )
                                            onSetParam(effect.effectId, param.id, param.initialValue)
                                        }
                                    )
                                },
                            value = param.normalized().let { if (it.isNaN() || it.isInfinite()) 0f else it },
                            onValueChange = { normalized ->
                                param.fromNormalized(normalized)
                                onSetParam(effect.effectId, param.id, param.value)
                            },
                            onValueChangeFinished = {},
                            knobImageResId = R.drawable.control_knob,
                            frameCount = 31
                        )
                    }

                    // Dropdown for discrete parameters
                    is EffectParameter.Discrete<*> -> {
                        var expanded by remember { mutableStateOf(false) }
                        Box {
                            Button(
                                onClick = { expanded = true },
                                modifier = Modifier.scale(0.8f).padding(vertical = 8.dp)
                            ) {
                                val paramVal = param.value
                                Text(
                                    text = if (paramVal is UIEnum) paramVal.uiName else paramVal.toString(),
                                    style = MaterialTheme.typography.bodyMedium
                                )
                                Icon(
                                    Icons.Filled.KeyboardArrowDown,
                                    contentDescription = "Select"
                                )
                            }
                            DropdownMenu(
                                expanded = expanded,
                                onDismissRequest = { expanded = false }
                            ) {
                                param.possibleValues.forEach { option ->
                                    DropdownMenuItem(
                                        text = { Text(
                                            text = if (option is UIEnum) option.uiName else option.toString(),
                                            style = MaterialTheme.typography.bodySmall
                                        )},
                                        onClick = {
                                            onSetParam(effect.effectId, param.id, option!!)
                                            expanded = false
                                        })
                                }
                            }
                        }
                    }

                    // Switch for toggle parameters
                    is EffectParameter.Toggle -> {
                        DDSwitch(
                            modifier = Modifier.scale(0.8f),
                            checked = param.value,
                            onCheckedChange = { onSetParam(effect.effectId, param.id, !param.value) },
                            imageRes = R.drawable.control_toggle
                        )
                    }
                }
            }

//            VerticalDivider(
//                thickness = 1.dp,
//                modifier = Modifier
//                    .fillMaxHeight()
//                    .padding(horizontal = 10.dp)
//            )

        }
    }
}
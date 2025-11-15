package lol.pony.dubstepdishwasher.ui

import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.IntrinsicSize
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
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
import androidx.compose.material.icons.filled.Delete
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
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.VerticalDivider
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.scale
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewmodel.compose.viewModel
import com.polidea.rxandroidble3.RxBleDevice
import lol.pony.dubstepdishwasher.model.core.BleManager
import lol.pony.dubstepdishwasher.model.core.Effect
import lol.pony.dubstepdishwasher.model.core.EffectParameter
import lol.pony.dubstepdishwasher.model.core.EffectType
import lol.pony.dubstepdishwasher.model.core.UIEnum
import lol.pony.dubstepdishwasher.model.core.mapRange
import lol.pony.dubstepdishwasher.viewmodel.EffectChainViewModel
import kotlin.math.pow

@Composable
fun FXPanel(
    modifier: Modifier = Modifier, bleManager: BleManager, device: RxBleDevice?,
    onDisconnect: () -> Unit
) {
    val viewModel: EffectChainViewModel = viewModel(factory = EffectChainViewModelFactory(bleManager))
    val effects by viewModel.effects.collectAsState()

    Column(modifier = modifier.fillMaxSize().padding(16.dp)) {
        Row(modifier = Modifier.fillMaxWidth()) {
            EffectChainControls(
                onAdd = { viewModel.addEffect(it) },
                onClear = { viewModel.clearChain() }
            )
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(device?.name ?: "Unknown Device")
                IconButton(onClick = onDisconnect) {
                    Icon(Icons.Filled.Close, contentDescription = "Disconnect")
                }
            }
        }
        Spacer(Modifier.height(8.dp))
        HorizontalDivider()
        EffectList(
            effects = effects,
            onToggleBypass = { id -> viewModel.toggleBypass(id) },
            onRemove = { id -> viewModel.removeEffect(id) },
            onReorder = { id, toIndex -> viewModel.reorderEffect(id, toIndex) },
            onSetParam = { id, param, value -> viewModel.setParam(id, param, value) }
        )
    }
}

@Composable
fun EffectChainControls(onAdd: (EffectType) -> Unit, onClear: () -> Unit) {
    Row {
        var expanded by remember { mutableStateOf(false) }

        // Add effect button + dropdown
        Box {
            IconButton(onClick = { expanded = !expanded }) {
                Icon(Icons.Filled.Add, contentDescription = "Add FX")
            }
            DropdownMenu(expanded = expanded, onDismissRequest = { expanded = false }) {
                EffectType.entries.forEach { type ->
                    DropdownMenuItem(
                        text = { Text(type.uiName) },
                        onClick = {
                            onAdd(type)
                            expanded = false
                        }
                    )
                }
            }
        }

        // Clear button
        IconButton(onClick = onClear) {
            Icon(Icons.Filled.Delete, contentDescription = "Clear")
        }
    }
}

@Composable
fun EffectList(
    effects: List<Effect>,
    onToggleBypass: (Int) -> Unit,
    onRemove: (Int) -> Unit,
    onReorder: (Int, Int) -> Unit,
    onSetParam: (Int, Int, Any) -> Unit
) {
    LazyColumn {
        itemsIndexed(items = effects, key = { _, fx -> fx.effectId }) { index, fx ->
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(120.dp)
                    .padding(vertical = 8.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {

                // Effect name
                Column {
                    Text(fx.effectType.uiName, style = MaterialTheme.typography.titleMedium, modifier = Modifier.width(120.dp))
                    // TEMP: Remove in release
                    // Text("Id: ${fx.effectId}")
                    // Text("Index: $index")
                }

                // Bypass switch
                Switch(
                    modifier = Modifier.scale(0.8f),
                    checked = !fx.isBypassed,
                    onCheckedChange = { onToggleBypass(fx.effectId) })

                // Reorder effect buttons
                // TEMP: Potentially switch to drag and drop
                Column {
                    IconButton(
                        onClick = { onReorder(fx.effectId, index - 1) },
                        enabled = index > 0
                    ) { Icon(Icons.Filled.KeyboardArrowUp, contentDescription = "Move Up") }
                    IconButton(
                        onClick = { onReorder(fx.effectId, index + 1) },
                        enabled = index < effects.size - 1
                    ) { Icon(Icons.Filled.KeyboardArrowDown, contentDescription = "Move Down") }
                }

                // Remove effect button
                IconButton(onClick = { onRemove(fx.effectId) }) {
                    Icon(Icons.Filled.Close, contentDescription = "Remove") }

                // Parameter list
                VerticalDivider(
                    thickness = 2.dp,
                    modifier = Modifier
                        .fillMaxHeight()
                        .padding(horizontal = 2.dp)
                )
                ParamList(effect = fx, onSetParam = onSetParam)
            }
            HorizontalDivider()
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ParamList(
    effect: Effect,
    onSetParam: (Int, Int, Any) -> Unit
) {
    LazyRow(Modifier.fillMaxWidth()) {
        items(items = effect.parameters.toList(), key = { it.id }) { param ->
            Column(
                horizontalAlignment = Alignment.CenterHorizontally,
                modifier = Modifier
                    .padding(horizontal = 10.dp)
                    .width(IntrinsicSize.Max)
                    .defaultMinSize(minWidth = 100.dp)
            ) {
                /* Parameter name */
                Text(
                    text = param.name,
                    modifier = Modifier.fillMaxWidth(),
                    textAlign = TextAlign.Center,
                    maxLines = 1
                )

                // Dialog state for Range parameters
                var showDialog by remember { mutableStateOf(false) }
                var textValue by remember { mutableStateOf("") }

                /* Parameter value */
                Text(
                    text = param.formatValue(),
                    modifier = Modifier.pointerInput(Unit) {
                        detectTapGestures(
                            // Edit value (dialog) on tap
                            onTap = {
                                if (param is EffectParameter.Range) {
                                    textValue = param.value.toString()
                                    showDialog = true
                                }
                            },
                            // Reset to init value on long press
                            onLongPress = {
                                if (param is EffectParameter.Range<*>) {
                                    @Suppress("UNCHECKED_CAST")
                                    (param as EffectParameter.Range<Any>).value = param.initialValue
                                    onSetParam(effect.effectId, param.id, param.initialValue)
                                }
                            }
                        )
                    }
                )

                /* Parameter input */
                when(param) {
                    is EffectParameter.Range -> {
                        var normalizedValue by remember {
                            mutableFloatStateOf(param.normalized().let { if (it.isNaN() || it.isInfinite()) 0f else it })
                        }
                        // Update normalized value when param value changes externally
                        LaunchedEffect(param.value) {
                            normalizedValue = param.normalized().let { if (it.isNaN() || it.isInfinite()) 0f else it }
                        }

                        /* Text input dialog */
                        if (showDialog) {
                            androidx.compose.material3.AlertDialog(
                                onDismissRequest = {
                                    showDialog = false
                                    textValue = param.value.toString()
                                },
                                title = { Text(param.name, style = MaterialTheme.typography.titleSmall) },
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
                                                normalizedValue = param.normalized() // Update slider position
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

                        // Sliders for range parameters
                        // TEMP: Switch to knobs
                        Slider(
                            modifier = Modifier.fillMaxWidth(),
                            track = { sliderState ->
                                SliderDefaults.Track(sliderState = sliderState, thumbTrackGapSize = 0.dp)
                            },
                            colors = SliderDefaults.colors(
                                thumbColor = Color.Black,
                                activeTrackColor = MaterialTheme.colorScheme.primary,
                                inactiveTrackColor = MaterialTheme.colorScheme.surfaceVariant,

                            ),

                            value = normalizedValue,
                            onValueChange = {
                                normalizedValue = it // Sync slider position
                                param.fromNormalized(it) // Set internal value
                                normalizedValue = param.normalized().let { norm -> // Update slider pos to reflect rounded actual
                                    if (norm.isNaN() || norm.isInfinite()) 0f else norm
                                }
                                onSetParam(effect.effectId, param.id, param.value)
                            },
                            onValueChangeFinished = {},
                            valueRange = 0f..1f,
                            steps = 0
                        )
                    }

                    // Dropdown for discrete parameters
                    is EffectParameter.Discrete<*> -> {
                        var expanded by remember { mutableStateOf(false) }
                        Box {
                            Button(onClick = { expanded = true }) {
                                val paramVal = param.value
                                Text(if (paramVal is UIEnum) paramVal.uiName else paramVal.toString())
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
                                        text = { Text(text = if (option is UIEnum) option.uiName else option.toString())},
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
                        Switch(
                            modifier = Modifier.scale(0.8f),
                            checked = param.value,
                            onCheckedChange = { onSetParam(effect.effectId, param.id, !param.value) })
                    }
                }
            }

            VerticalDivider(
                thickness = 1.dp,
                modifier = Modifier
                    .fillMaxHeight()
                    .padding(horizontal = 10.dp)
            )

        }
    }
}

class EffectChainViewModelFactory(private val bleManager: BleManager) : ViewModelProvider.Factory {
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        if (modelClass.isAssignableFrom(EffectChainViewModel::class.java)) {
            @Suppress("UNCHECKED_CAST")
            return EffectChainViewModel(bleManager) as T
        }
        throw IllegalArgumentException("Unknown ViewModel class")
    }
}
package lol.pony.dubstepdishwasher.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.imePadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Edit
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.key
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import lol.pony.dubstepdishwasher.R
import lol.pony.dubstepdishwasher.model.core.LFOMode
import lol.pony.dubstepdishwasher.model.core.Modulator
import lol.pony.dubstepdishwasher.model.core.ModulatorParameter
import lol.pony.dubstepdishwasher.model.core.UIEnum
import lol.pony.dubstepdishwasher.model.core.mapRange
import lol.pony.dubstepdishwasher.ui.components.subcomponents.CurveDisplay
import lol.pony.dubstepdishwasher.ui.components.subcomponents.RandomScope
import lol.pony.dubstepdishwasher.ui.controls.DDKnob
import kotlin.math.pow

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ModulatorControls(
    mod: Modulator,
    currentModValues: Map<String, Float>,
    onSetParam: (String, Int, Any) -> Unit,
    onEditCurve: () -> Unit
) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(8.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        /* PARAMETERS */
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(top = 4.dp, bottom = 14.dp),
            horizontalArrangement = Arrangement.SpaceEvenly,
            verticalAlignment = Alignment.CenterVertically
        ) {
            mod.parameters
                .filter { param ->
                    // Only show Random param in Random mode
                    !(param.id == 2 && mod is Modulator.LFO &&
                            (mod.parameters.find { it.name == "Mode" }?.getValueAny() as? LFOMode) != LFOMode.RANDOM)
                }
                .forEach { param ->
                    key("${mod.id}:${param.id}") {
                        Column(
                            horizontalAlignment = Alignment.CenterHorizontally,
                            modifier = Modifier.weight(1f)
                        ) {

                            /* Parameter name */
                            Text(
                                text = param.name,
                                style = MaterialTheme.typography.labelSmall,
                                maxLines = 1,
                                modifier = Modifier.padding(bottom = 4.dp)
                            )

                            /* Parameter input */
                            when (param) {
                                is ModulatorParameter.Range -> {
                                    // Dialog state
                                    var showDialog by remember { mutableStateOf(false) }
                                    var textValue by remember { mutableStateOf("") }

                                    /* Text input dialog */
                                    if (showDialog) {
                                        AlertDialog(
                                            onDismissRequest = {
                                                showDialog = false
                                                textValue = param.value.toString()
                                            },
                                            title = { Text(text = param.name, style = MaterialTheme.typography.bodyMedium) },
                                            text = {
                                                OutlinedTextField(
                                                    value = textValue,
                                                    onValueChange = { textValue = it },
                                                    label = { Text(text = "Value", style = MaterialTheme.typography.bodySmall) },
                                                    singleLine = true,
                                                    modifier = Modifier.fillMaxWidth(),
                                                    keyboardOptions = KeyboardOptions(
                                                        keyboardType = KeyboardType.Decimal,
                                                        imeAction = ImeAction.Done
                                                    )
                                                )
                                            },
                                            confirmButton = {
                                                TextButton(
                                                    onClick = {
                                                        textValue.toFloatOrNull()?.let { newValue ->
                                                            val clamped = newValue.coerceIn(param.range.first, param.range.second)
                                                            param.fromNormalized(
                                                                clamped.mapRange(
                                                                    inRange = param.range.first..param.range.second,
                                                                    outRange = 0f..1f
                                                                ).pow(1 / param.exp)
                                                            )
                                                            onSetParam(mod.id, param.id, param.value)
                                                        }
                                                        showDialog = false
                                                    }
                                                ) { Text(text = "OK", style = MaterialTheme.typography.bodyMedium) }
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
                                            .size(32.dp)
                                            .pointerInput(mod.id, param.id) {
                                                detectTapGestures(
                                                    onTap = {
                                                        textValue = param.value.toString()
                                                        showDialog = true
                                                    },
                                                    onDoubleTap = {
                                                        param.fromNormalized(
                                                            param.initialValue
                                                                .mapRange(
                                                                    inRange = param.range.first..param.range.second,
                                                                    outRange = 0f..1f
                                                                )
                                                                .pow(1f / param.exp)
                                                        )
                                                        onSetParam(mod.id, param.id, param.initialValue)
                                                    }
                                                )
                                            },
                                        value = param.normalized()
                                            .let { if (it.isNaN() || it.isInfinite()) 0f else it },
                                        onValueChange = { normalized ->
                                            param.fromNormalized(normalized)
                                            onSetParam(mod.id, param.id, param.value)
                                        },
                                        onValueChangeFinished = {},
                                        knobImageResId = R.drawable.control_knob,
                                        frameCount = 31,
                                        effectID = -1,
                                        paramID = param.id,
                                        modAssignments = emptyList(),
                                        selectedModID = null,
                                        isModulatable = false,
                                        currentModOffset = 0f,
                                        onAssignModulator = {},
                                        onRemoveModulator = {},
                                        onModAmountChange = { _, _ -> },
                                        onTogglePolarity = {}
                                    )
                                    Text(
                                        text = param.formatValue(),
                                        style = MaterialTheme.typography.labelSmall
                                    )
                                }

                                // Dropdown for discrete parameters
                                is ModulatorParameter.Discrete<*> -> {
                                    var expanded by remember { mutableStateOf(false) }
                                    Box(modifier = Modifier.padding(horizontal = 8.dp)) {
                                        Button(
                                            onClick = { expanded = true },
                                            modifier = Modifier
                                                .height(32.dp)
                                                .width(80.dp)
                                                .padding(horizontal = 2.dp),
                                            contentPadding = PaddingValues(0.dp)
                                        ) {
                                            val paramVal = param.value
                                            Text(
                                                text = if (paramVal is UIEnum) paramVal.uiName else paramVal.toString(),
                                                style = MaterialTheme.typography.labelSmall,
                                                maxLines = 1
                                            )
                                        }
                                        DropdownMenu(
                                            expanded = expanded,
                                            onDismissRequest = { expanded = false }
                                        ) {
                                            param.possibleValues.forEach { option ->
                                                DropdownMenuItem(
                                                    text = {
                                                        Text(
                                                            text = if (option is UIEnum) option.uiName else option.toString(),
                                                            style = MaterialTheme.typography.bodySmall
                                                        )
                                                    },
                                                    onClick = {
                                                        onSetParam(mod.id, param.id, option!!)
                                                        expanded = false
                                                    }
                                                )
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
        }

        // HorizontalDivider(modifier = Modifier.padding(vertical = 8.dp).height(2.dp))
        // Spacer(modifier = Modifier.height(4.dp))

        /* MODULATION PREVIEW */
        Box(
            modifier = Modifier
                .fillMaxSize()
                .background(Color(0x11000000))
                .padding(4.dp)
        ) {
            // Render appropriate preview
            val lineColor = MaterialTheme.colorScheme.secondary
            when (mod) {
                is Modulator.LFO -> {
                    when (mod.parameters.find { it.name == "Mode" }?.getValueAny()) {
                        LFOMode.NORMAL -> {
                            CurveDisplay(
                                points = mod.curve,
                                modifier = Modifier.fillMaxSize(),
                                currentPosition = mod.phase,
                                showPoints = false,
                                fillGradient = true,
                                lineColor = lineColor,
                                positionColor = Color(0xFFCFFFF5),
                                backgroundColor = MaterialTheme.colorScheme.surfaceVariant,
                                gridColor = Color(0x22FFFFFF),
                                gridX = 2,
                                gridY = 2
                            )
                        }
                        LFOMode.RANDOM -> {
                            // Get current value from view model
                            val currentValue = currentModValues[mod.id] ?: 0.5f
                            RandomScope(
                                modId = mod.id,
                                currentValue = currentValue,
                                modifier = Modifier.fillMaxSize(),
                                lineColor = lineColor
                            )
                        }
                    }
                }
                is Modulator.Mapping -> {
                    CurveDisplay(
                        points = mod.curve,
                        modifier = Modifier.fillMaxSize(),
                        currentPosition = mod.inputValue,
                        showPoints = false,
                        fillGradient = true,
                        lineColor = lineColor,
                        positionColor = Color(0xFFCFFFF5),
                        backgroundColor = MaterialTheme.colorScheme.surfaceVariant,
                        gridColor = Color(0x22FFFFFF),
                        gridX = 2,
                        gridY = 2
                    )
                }
            }

            // Edit button overlay
            if (mod !is Modulator.LFO || mod.parameters.find { it.name == "Mode" }?.getValueAny() != LFOMode.RANDOM) {
                IconButton(
                    onClick = { onEditCurve() },
                    modifier = Modifier
                        .align(Alignment.TopEnd)
                        .size(16.dp)
                        .padding(top = 4.dp, end = 4.dp)
                        .alpha(0.5f)
                ) {
                    Icon(
                        imageVector = Icons.Default.Edit,
                        contentDescription = "Edit",
                        tint = Color(0xFFF3FFFC)
                    )
                }
            }
        }
    }
}
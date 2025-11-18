package lol.pony.dubstepdishwasher.ui.components.subcomponents

import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.IntrinsicSize
import androidx.compose.foundation.layout.defaultMinSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.imePadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.KeyboardArrowDown
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.scale
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.rememberTextMeasurer
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import lol.pony.dubstepdishwasher.R
import lol.pony.dubstepdishwasher.model.core.Effect
import lol.pony.dubstepdishwasher.model.core.EffectParameter
import lol.pony.dubstepdishwasher.model.core.ModAssignment
import lol.pony.dubstepdishwasher.model.core.Modulator
import lol.pony.dubstepdishwasher.model.core.ParamKey
import lol.pony.dubstepdishwasher.model.core.UIEnum
import lol.pony.dubstepdishwasher.model.core.mapRange
import lol.pony.dubstepdishwasher.ui.controls.DDKnob
import lol.pony.dubstepdishwasher.ui.controls.DDSwitch
import lol.pony.dubstepdishwasher.ui.theme.MainFontFamily
import kotlin.math.pow

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ParameterItem (
    effect: Effect,
    param: EffectParameter<*>,
    assignments: List<ModAssignment>,
    selectedModulator: Modulator?,
    currentModOffsets: Map<ParamKey, Float>,
    // Callbacks
    onSetParam: (Int, Int, Any) -> Unit,
    onAssignMod: (String, Int, Int) -> Unit,
    onRemoveMod: (String, Int, Int) -> Unit,
    onModAmountChange: (String, Int, Int, Float) -> Unit,
    onTogglePolarity: (String, Int, Int) -> Unit
) {
    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        modifier = Modifier
        .padding(horizontal = 2.dp)
        .width(IntrinsicSize.Max)
        .defaultMinSize(minWidth = 80.dp)
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

        // Mod assignments for mod arcs
        val paramMods = assignments.filter { it.target.effectId == effect.effectId && it.target.paramId == param.id }

        /* Parameter input */
        when(param) {
            is EffectParameter.Range -> {
                /* Text input dialog */
                if (showDialog) {
                    AlertDialog(
                        onDismissRequest = {
                            showDialog = false
                            textValue = param.value.toString()
                        },
                        title = { Text(param.name, style = MaterialTheme.typography.bodyMedium) },
                        text = {
                            OutlinedTextField(
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
                            TextButton(
                                onClick = {
                                    textValue.toFloatOrNull()?.let { newValue ->
                                        val clamped = newValue.coerceIn(param.range.first.toFloat(), param.range.second.toFloat()) // Clamp user input
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
                            ) { Text(text = "OK", style = MaterialTheme.typography.bodyMedium) }
                        },
                        dismissButton = null,
                        modifier = Modifier
                            .fillMaxWidth(0.8f)
                            .padding(horizontal = 32.dp)
                            .imePadding()
                    )
                }

                // Knob for Range parameters
                val paramKey = ParamKey(effect.effectId, param.id)
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
                    frameCount = 31,

                    effectID = param.effectId,
                    paramID = param.id,
                    modAssignments = paramMods,
                    selectedModID = selectedModulator?.id,
                    isModulatable = param.isModulatable,
                    currentModOffset = currentModOffsets[paramKey] ?: 0f,

                    onAssignModulator = { modId -> onAssignMod(modId, effect.effectId, param.id) },
                    onRemoveModulator = { modId -> onRemoveMod(modId, effect.effectId, param.id) },
                    onModAmountChange = { modId, amount -> onModAmountChange(modId, effect.effectId, param.id, amount) },
                    onTogglePolarity = { modId -> onTogglePolarity(modId, effect.effectId, param.id) }
                )
            }

            // Dropdown for discrete parameters
            is EffectParameter.Discrete<*> -> {
                var expanded by remember { mutableStateOf(false) }

                val textMeasurer = rememberTextMeasurer()
                val density = LocalDensity.current
                val maxTextWidth by remember(param.possibleValues) {
                    mutableStateOf(
                        run {
                            val widestText = param.possibleValues.maxOf { option ->
                                val label = if (option is UIEnum) option.uiName else option.toString()
                                textMeasurer.measure(
                                    text = label,
                                    style = TextStyle(
                                        fontFamily = MainFontFamily,
                                        fontWeight = FontWeight.Normal,
                                        fontSize = 16.sp,
                                        lineHeight = 16.sp
                                    )
                                ).size.width.toFloat()
                            }
                            with(density) { widestText.toDp() + 74.dp }
                        }
                    )
                }

                Box {
                    Button(
                        onClick = { expanded = true },
                        modifier = Modifier
                            .width(maxTextWidth)
                            .scale(0.8f)
                            .padding(vertical = 8.dp)
                    ) {
                        val paramVal = param.value
                        Text(
                            text = if (paramVal is UIEnum) paramVal.uiName else paramVal.toString(),
                            style = MaterialTheme.typography.bodyMedium,
                            overflow = TextOverflow.Ellipsis
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
}
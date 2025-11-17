package lol.pony.dubstepdishwasher.ui.components

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.imePadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.wrapContentWidth
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.AddCircle
import androidx.compose.material.icons.filled.KeyboardArrowDown
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
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
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import lol.pony.dubstepdishwasher.model.core.Preset
import lol.pony.dubstepdishwasher.model.core.PresetContainer

@Composable
fun <T, C : PresetContainer<T>> PresetManager( // T - preset data type, C - preset state container
    presets: List<Preset<T>>,
    currentData: T,
    containerState: C,
    onStateChange: (C) -> Unit,
    onSave: (String) -> Preset<T>,
    onLoad: (Preset<T>) -> Unit,
    copyContainer: (C, Preset<T>) -> C
) {
    var expanded by remember { mutableStateOf(false) }
    var showSaveDialog by remember { mutableStateOf(false) }
    var nameEntry by remember { mutableStateOf("") }

    var currentPreset by remember { mutableStateOf(containerState.currentPreset) }

    Row(verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.Start,
        modifier = Modifier.scale(0.8f)
    ) {
        /* Save button */
        IconButton(onClick = { showSaveDialog = true }, modifier = Modifier.padding(end = 4.dp)) {
            Icon(
                imageVector = Icons.Default.AddCircle,
                contentDescription = "Save Preset"
            )
        }

        /* Preset list */
        // TEMP: Maybe better than a dropdown? "Folder" organization + delete, favorite, etc.
        Box {
            Row (verticalAlignment = Alignment.CenterVertically) {
                Button(
                    onClick = { expanded = true },
                    modifier = Modifier
                        .width(180.dp) // TEMP: Make this a parameter?
                        .scale(0.8f),
                    shape = RoundedCornerShape(4.dp),
                    contentPadding = PaddingValues(horizontal = 8.dp, vertical = 4.dp)
                ) {
                    Text(
                        text = currentPreset.name + (if (currentData != currentPreset.data) "*" else ""), // Asterisk if actual state deviated from preset
                        maxLines = 1,
                        overflow = TextOverflow.Ellipsis,
                        style = MaterialTheme.typography.bodyMedium,
                        modifier = Modifier.weight(1f)
                    )
                    Icon(
                        Icons.Filled.KeyboardArrowDown,
                        contentDescription = "Load Preset"
                    )
                }
            }

            Box (modifier = Modifier.wrapContentWidth()) {
                DropdownMenu(
                    expanded = expanded,
                    onDismissRequest = { expanded = false },
                    modifier = Modifier.width(180.dp)
                ) {
                    presets.forEach { preset ->
                        DropdownMenuItem(
                            text = { Text(text = preset.name, style = MaterialTheme.typography.bodyMedium) },
                            onClick = {
                                expanded = false
                                currentPreset = preset // Update internal state
                                onStateChange(copyContainer(containerState, preset)) // Interface copy hack - try to ensure inheritors are data classes!
                                onLoad(preset) // Load that shit
                            }
                        )
                    }
                } // Dropdown
            } // Box
        } // Box
    } // Row

    /* Save Dialog */
    if (showSaveDialog) {
        AlertDialog(
            onDismissRequest = { showSaveDialog = false },
            title = { Text(text = "Save Preset", style = MaterialTheme.typography.bodyMedium) },
            text = {
                OutlinedTextField(
                    value = nameEntry,
                    onValueChange = { nameEntry = it },
                    label = { Text(text = "Preset Name", style = MaterialTheme.typography.bodySmall) },
                    singleLine = true,
                    modifier = Modifier.fillMaxWidth(),
                    keyboardOptions = KeyboardOptions(
                        keyboardType = KeyboardType.Text,
                        imeAction = ImeAction.Done
                    )
                )
            },
            confirmButton = {
                TextButton(onClick = {
                    val preset = onSave(nameEntry)
                    currentPreset = preset
                    onStateChange(copyContainer(containerState, preset))
                    onLoad(preset)
                    showSaveDialog = false
                }) { Text(text = "Save", style = MaterialTheme.typography.bodyMedium) }
            },
            dismissButton = null,
            modifier = Modifier
                .fillMaxWidth(0.8f)
                .padding(horizontal = 32.dp)
                .imePadding()
        ) // Alert
    } // Save dialog
} // PresetManager
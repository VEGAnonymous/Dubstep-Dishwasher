package lol.pony.dubstepdishwasher.ui.components

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.imePadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.wrapContentWidth
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.AddCircle
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.KeyboardArrowDown
import androidx.compose.material.icons.filled.Refresh
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.HorizontalDivider
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
import lol.pony.dubstepdishwasher.model.core.RandomizablePreset
import lol.pony.dubstepdishwasher.R
import lol.pony.dubstepdishwasher.ui.controls.DDSwitch

@Composable
// T - preset data type, C - preset state container, A - randomizer args if applicable (set to Unit to ignore)
fun <T, C : PresetContainer<T>, A> PresetManager(
    presets: List<Preset<T>>,
    currentData: T?,
    randomArgs: A? = null,
    containerState: C,
    onStateChange: (C) -> Unit,
    onSave: (String, String?) -> Preset<T>,
    onLoad: (Preset<T>) -> Unit,
    onDelete: (String) -> Unit,
    onFavorite: (String, Boolean) -> Unit,
    copyContainer: (C, Preset<T>) -> C
) {
    var expanded by remember { mutableStateOf(false) }

    var showSaveDialog by remember { mutableStateOf(false) }
    var nameEntry by remember { mutableStateOf("") }
    var categoryEntry by remember { mutableStateOf("") }
    var categoryError by remember { mutableStateOf(false) }

    var showDeleteDialog by remember { mutableStateOf(false) }
    var presetToDelete by remember { mutableStateOf<Preset<T>?>(null) }

    var currentPreset by remember { mutableStateOf(containerState.currentPreset) }

    Row(verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy((-16).dp),
        modifier = Modifier.scale(0.8f)
    ) {
        /* Save button */
        IconButton(onClick = { showSaveDialog = true }) {
            Icon(
                imageVector = Icons.Default.AddCircle,
                contentDescription = "Save Preset"
            )
        }

        /* Preset list */
        Box {
            Row (verticalAlignment = Alignment.CenterVertically) {
                Button(
                    onClick = { expanded = true },
                    modifier = Modifier
                        .width(240.dp)
                        .scale(0.8f),
                    shape = RoundedCornerShape(4.dp),
                    contentPadding = PaddingValues(horizontal = 8.dp, vertical = 4.dp)
                ) {
                    Text(
                        text = (currentPreset?.name ?: "Custom") + (if (currentData != currentPreset?.data) "*" else ""),
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

            // Group presets by category / favorites
            val presetsByCategory = remember(presets) { presets.groupBy { it.category } }
            val favoritePresets = presets.filter { it.favorite } // Virtual category

            /* DROPDOWN TREE-MENU */
            // FIXME: Can only un-favorite presets from the favorites category
            Box (modifier = Modifier.wrapContentWidth()) {
                DropdownMenu(
                    expanded = expanded,
                    onDismissRequest = { expanded = false },
                    modifier = Modifier.width(260.dp)
                ) {
                    /* FAVORITES */
                    if (favoritePresets.isNotEmpty()) {
                        CategoryHeader("★ Favorites")
                        favoritePresets.forEach { preset ->
                            PresetRow(
                                preset = preset,
                                onLoad = onLoad,
                                onFavorite = { name, favorite -> onFavorite(name, favorite) },
                                onDelete = null,
                                presetSetter = { preset -> currentPreset = preset }
                            )
                        }
                        HorizontalDivider(modifier = Modifier.padding(vertical = 4.dp))
                    }

                    /* CATEGORIES */
                    presetsByCategory.keys.filterNotNull().sorted().forEach { category ->
                            CategoryHeader(category)
                            presetsByCategory[category]!!.forEach { preset ->
                                PresetRow(
                                    preset = preset,
                                    onLoad = onLoad,
                                    onFavorite = { name, favorite -> onFavorite(name, favorite) },
                                    onDelete = { preset -> presetToDelete = preset; showDeleteDialog = true },
                                    presetSetter = { preset -> currentPreset = preset }
                                )
                            }
                            HorizontalDivider(modifier = Modifier.padding(vertical = 4.dp))
                        }

                    /* UNCATEGORIZED */
                    val noCategory = presetsByCategory[null]
                    if (!noCategory.isNullOrEmpty()) {
                        CategoryHeader("Uncategorized")
                        noCategory.forEach { preset ->
                            PresetRow(
                                preset = preset,
                                onLoad = onLoad,
                                onFavorite = { name, favorite -> onFavorite(name, favorite) },
                                onDelete = { preset -> presetToDelete = preset; showDeleteDialog = true },
                                presetSetter = { preset -> currentPreset = preset }
                            )
                        }
                    }
                } // Dropdown menu
            } // Preset dropdown box
        } // Preset list box

        /* Randomize button */
        @Suppress("UNCHECKED_CAST")
        val randomizablePreset = containerState.currentPreset as? RandomizablePreset<T, A>
        if (randomizablePreset != null) {
            IconButton(onClick = {
                val rp = randomizablePreset.randomized(randomArgs)
                onLoad(rp)
            }) {
                Icon(
                    imageVector = Icons.Default.Refresh,
                    contentDescription = "Randomize"
                )
            }
        }
    } // Row

    /* Save Dialog */
    if (showSaveDialog) {
        AlertDialog(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 32.dp)
                .imePadding(),
            onDismissRequest = { showSaveDialog = false },
            title = { Text(text = "Save Preset", style = MaterialTheme.typography.bodyMedium) },
            text = {
                Column {
                    // Name entry
                    OutlinedTextField(
                        value = nameEntry,
                        onValueChange = { nameEntry = it },
                        label = { Text("Preset Name", style = MaterialTheme.typography.bodySmall) },
                        singleLine = true,
                        modifier = Modifier.fillMaxWidth(),
                        keyboardOptions = KeyboardOptions(
                            keyboardType = KeyboardType.Text,
                            imeAction = ImeAction.Next
                        )
                    )
                    // Category entry
                    OutlinedTextField(
                        value = categoryEntry,
                        onValueChange = {
                            categoryEntry = it
                            categoryError = it.equals("Factory", ignoreCase = true)
                        },
                        isError = categoryError,
                        supportingText = {
                            if (categoryError) {
                                Text(
                                    text = "\"Factory\" is reserved",
                                    color = MaterialTheme.colorScheme.error,
                                    style = MaterialTheme.typography.bodySmall
                                )
                            }
                        },
                        label = { Text("Category (opt)", style = MaterialTheme.typography.bodySmall) },
                        singleLine = true,
                        modifier = Modifier.fillMaxWidth(),
                        keyboardOptions = KeyboardOptions(
                            keyboardType = KeyboardType.Text,
                            imeAction = ImeAction.Done
                        )
                    )
                }
            },
            confirmButton = {
                TextButton(
                    enabled = !categoryError,
                    onClick = {
                        val preset = onSave(nameEntry, categoryEntry.ifBlank { null })
                        currentPreset = preset
                        onStateChange(copyContainer(containerState, preset))
                        onLoad(preset)
                        showSaveDialog = false
                    }
                ) { Text(text = "Save", style = MaterialTheme.typography.bodyMedium) }
            },
            dismissButton = null
        ) // Alert
    } // Save dialog

    /* Deletion dialog */
    if (showDeleteDialog && presetToDelete != null) {
        AlertDialog(
            onDismissRequest = { showDeleteDialog = false },
            title = { Text(text = "Delete '${presetToDelete!!.name}'?", style = MaterialTheme.typography.bodyMedium) },
            confirmButton = {
                TextButton(onClick = {
                    onDelete(presetToDelete!!.name)
                    if (currentPreset?.name == presetToDelete!!.name)
                        currentPreset = null
                    showDeleteDialog = false
                }) { Text(text = "Delete", style = MaterialTheme.typography.bodyMedium) }
            },
            dismissButton = {
                TextButton(onClick = { showDeleteDialog = false }) { Text(text = "Cancel", style = MaterialTheme.typography.bodyMedium) }
            }
        )
    }
} // PresetManager

/* DROPDOWN COMPONENTS */
@Composable
private fun CategoryHeader(name: String) {
    Text(
        text = name,
        style = MaterialTheme.typography.bodyMedium,
        modifier = Modifier.padding(start = 8.dp, top = 6.dp, bottom = 4.dp)
    )
}

@Composable
private fun <T> PresetRow(
    preset: Preset<T>,
    onLoad: (Preset<T>) -> Unit,
    onFavorite: (String, Boolean) -> Unit,
    onDelete: ((Preset<T>) -> Unit)?,
    presetSetter: (Preset<T>) -> Unit
) {
    Row(
        verticalAlignment = Alignment.CenterVertically,
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 8.dp)
    ) {
        // Preset name
        Text(
            text = preset.name,
            modifier = Modifier
                .weight(1f)
                .padding(6.dp)
                .clickable {
                    presetSetter(preset)
                    onLoad(preset)
                },
            style = MaterialTheme.typography.bodySmall
        )

        // Favorite preset toggle
        DDSwitch(
            modifier = Modifier.size(20.dp),
            checked = preset.favorite,
            onCheckedChange = { checked -> onFavorite(preset.name, checked) },
            imageRes = R.drawable.control_star
        )

        // Delete preset
        if (onDelete != null && preset.category != "Factory") {
            IconButton(onClick = { onDelete(preset) }) {
                Icon(
                    Icons.Default.Delete,
                    contentDescription = "Delete Preset"
                )
            }
        }
    }
}
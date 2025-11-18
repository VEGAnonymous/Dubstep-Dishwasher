package lol.pony.dubstepdishwasher.ui

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Close
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.polidea.rxandroidble3.RxBleDevice
import lol.pony.dubstepdishwasher.model.BLEManager
import lol.pony.dubstepdishwasher.model.core.GlobalPresetState
import lol.pony.dubstepdishwasher.model.core.MAX_COMPUTE_USAGE
import lol.pony.dubstepdishwasher.model.core.MAX_MEMORY_USAGE
import lol.pony.dubstepdishwasher.ui.components.PresetManager
import lol.pony.dubstepdishwasher.ui.components.ResourceMeter
import lol.pony.dubstepdishwasher.viewmodel.MainViewModel
import lol.pony.dubstepdishwasher.viewmodel.MainViewModelFactory

@Composable
fun TopBar (
    bleManager: BLEManager,
    device: RxBleDevice?,
    onDisconnect: () -> Unit
) {
    val viewModel: MainViewModel = viewModel(factory = MainViewModelFactory(bleManager))

    val globalState = viewModel.currentGlobalState.collectAsState().value
    val globalPresets = viewModel.globalPresets.collectAsState().value
    var globalPresetContainer by remember { mutableStateOf(GlobalPresetState()) }

    val usage = viewModel.resourceUsage.collectAsState().value
    var showUsageDialog by remember { mutableStateOf(false) }

    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 16.dp, vertical = 4.dp)
            .height(45.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        /* LOGO */
        // TODO: Design a thing for this
        Text(text = "DUBSTEP DISHWASHER", style = MaterialTheme.typography.headlineLarge)

        Spacer(modifier = Modifier.width(15.dp))

        /* PRESET MANAGER */
        PresetManager(
            presets = globalPresets,
            currentData = globalState,
            containerState = globalPresetContainer,
            randomArgs = Unit,
            onStateChange = { globalPresetContainer = it },
            onSave = { name, category -> viewModel.saveGlobalPreset(name, category) },
            onLoad = { preset -> viewModel.loadGlobalPreset(preset.name) },
            onDelete = { name -> viewModel.deleteGlobalPreset(name) },
            onFavorite = { name, favorite -> viewModel.favoriteGlobalPreset(name, favorite) },
            // Interface copy workaround: we know it's literally a data class
            copyContainer = { state, preset -> state.copy(currentPreset = preset) }
        )

        /* RESOURCE MONITOR */
        Row (
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy((-10).dp)
        ) {
            TextButton(
                onClick = { showUsageDialog = true },
                contentPadding = PaddingValues(0.dp),
                modifier = Modifier.size(width = 40.dp, height = 25.dp)
            ) {
                Text(text = "Usage", style = MaterialTheme.typography.labelSmall.copy(color = Color(0xFF000000)))
            }

            ResourceMeter(
                compute = usage.compute,
                memory = usage.memory,
                maxCompute = MAX_COMPUTE_USAGE,
                maxMemory = MAX_MEMORY_USAGE
            )
        }

        Spacer(modifier = Modifier.width(40.dp))

        /* BLE STATUS */
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text(
                text = device?.name ?: "Unknown Device",
                style = MaterialTheme.typography.bodySmall,
                modifier = Modifier.padding(horizontal = 8.dp)
            )
            IconButton(onClick = onDisconnect, modifier = Modifier.size(18.dp).padding(top = 3.dp)) {
                Icon(Icons.Filled.Close, contentDescription = "Disconnect")
            }
        }
    } // Row

    if (showUsageDialog) {
        AlertDialog(
            onDismissRequest = { showUsageDialog = false },
            title = { Text(text = "MCU Resource Usage (Approx.)", style = MaterialTheme.typography.bodyMedium) },
            text = {
                Column {
                    Text(text = "CPU: ${(usage.compute * 100).toInt()}%", style = MaterialTheme.typography.bodyMedium)
                    Text(text = "RAM (DTCM): ${usage.memory} / 512 kB", style = MaterialTheme.typography.bodyMedium)
                }
            },
            confirmButton = {
                TextButton(onClick = { showUsageDialog = false }) {
                    Text(text = "OK", style = MaterialTheme.typography.bodyMedium)
                }
            }
        )
    }
}
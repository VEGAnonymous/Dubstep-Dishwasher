package lol.pony.dubstepdishwasher.ui

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.*
import androidx.compose.material.icons.*
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Clear
import androidx.compose.material.icons.filled.Close
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.*
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewmodel.compose.viewModel
import com.polidea.rxandroidble3.RxBleDevice
import lol.pony.dubstepdishwasher.model.core.*
import lol.pony.dubstepdishwasher.viewmodel.*

@Composable
fun FXPanel(
    modifier: Modifier = Modifier, bleManager: BleManager, device: RxBleDevice,
    onDisconnect: () -> Unit
) {
    val viewModel: EffectChainViewModel = viewModel(factory = EffectChainViewModelFactory(bleManager))
    val effects by viewModel.effects.collectAsState()

    Column(modifier = modifier.fillMaxSize().padding(16.dp)) {
        Row(modifier = Modifier.fillMaxWidth()) {
            AddEffectMenu(onAdd = { viewModel.addEffect(it) })
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(device.name ?: "Unknown Device")
                IconButton(onClick = onDisconnect) {
                    Icon(Icons.Filled.Close, contentDescription = "Disconnect")
                }
            }
        }
        Spacer(Modifier.height(8.dp))
        EffectList(
            effects = effects,
            onToggleBypass = { id -> viewModel.toggleBypass(id) },
            onRemove = { id -> viewModel.removeEffect(id) },
            onReorder = { id, toIndex -> viewModel.reorderEffect(id, toIndex) }
        )
    }
}

@Composable
fun AddEffectMenu(onAdd: (EffectType) -> Unit) {
    var expanded by remember { mutableStateOf(false) }
    Box {
        IconButton(onClick = { expanded = !expanded }) { Icon(Icons.Default.Add, contentDescription = "Add FX") }
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
}

@Composable
fun EffectList(
    effects: List<Effect>,
    onToggleBypass: (Int) -> Unit,
    onRemove: (Int) -> Unit,
    onReorder: (Int, Int) -> Unit
) {
    LazyColumn {
        items(items = effects, key = { it.effectId }) { fx ->
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(vertical = 8.dp, horizontal = 4.dp),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text(fx.effectType.uiName, style = MaterialTheme.typography.titleMedium)
                Switch(modifier = Modifier.scale(0.8f), checked = !fx.isBypassed, onCheckedChange = { onToggleBypass(fx.effectId) })
                IconButton(onClick = { onRemove(fx.effectId) }) { Icon(Icons.Default.Clear, contentDescription = "Remove") }
            }
            HorizontalDivider()
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


//@Preview(showBackground = true)
//@Composable
//fun FXPanelPreview() {
//    DubstepDishwasherTheme {
//        FXPanel()
//    }
//}
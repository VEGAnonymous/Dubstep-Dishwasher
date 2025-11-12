package lol.pony.dubstepdishwasher.ui

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.*
import androidx.compose.material.icons.*
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.scale
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.layout.ModifierLocalBeyondBoundsLayout
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewmodel.compose.viewModel
import com.polidea.rxandroidble3.RxBleDevice
import lol.pony.dubstepdishwasher.model.core.*
import lol.pony.dubstepdishwasher.viewmodel.*
import kotlin.reflect.typeOf

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
            onReorder = { id, toIndex -> viewModel.reorderEffect(id, toIndex) },
            onSetParam = { id, param, value -> viewModel.setParam(id, param, value) }
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
    onReorder: (Int, Int) -> Unit,
    onSetParam: (Int, Int, Float) -> Unit
) {
    LazyColumn {
        itemsIndexed(items = effects, key = { _, fx -> fx.effectId }) { index, fx ->
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(vertical = 8.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(fx.effectType.uiName, style = MaterialTheme.typography.titleMedium, modifier = Modifier.width(120.dp))
                Switch(
                    modifier = Modifier.scale(0.8f),
                    checked = !fx.isBypassed,
                    onCheckedChange = { onToggleBypass(fx.effectId) })
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
                IconButton(onClick = { onRemove(fx.effectId) }) {
                    Icon(Icons.Default.Clear, contentDescription = "Remove") }
                ParamList(effect = fx, onSetParam = onSetParam)
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

@Composable
fun ParamList(
    effect: Effect,
    onSetParam: (Int, Int, Float) -> Unit
) {
    LazyRow(Modifier.fillMaxWidth()) {
        items(items = effect.parameters.toList(), key = { it.id }) { param ->
            var paramNameWidth by remember { mutableStateOf(0) }
            val density = LocalDensity.current

            Column(horizontalAlignment = Alignment.CenterHorizontally, modifier = Modifier.padding(horizontal = 2.dp)) {
                Text(
                    text = param.name,
                    modifier = Modifier.onSizeChanged {
                        paramNameWidth = it.width
                    }
                )
                Text(
                    when (param) {
                        is EffectParameter.Range -> "Range"
                        is EffectParameter.Discrete -> "Discrete"
                        is EffectParameter.Toggle -> "Toggle"
                    }
                )
                var paramValue by remember { mutableStateOf("") }
                TextField(
                    value = paramValue,
                    onValueChange = { paramValue = it },
                    label = { Text(text = "Value", modifier = Modifier.fillMaxWidth(), textAlign = TextAlign.Center) },
                    modifier = Modifier
                        .widthIn(
                            min = 100.dp,
                            max = with(density) { paramNameWidth.toDp() }.coerceAtLeast(100.dp)
                        )
                )
                Button(onClick = {
                    paramValue.toFloatOrNull()?.let {
                        onSetParam(effect.effectId, param.id, it)
                    }
                }) {
                    Text("Set")
                }
            }
        }
    }
}

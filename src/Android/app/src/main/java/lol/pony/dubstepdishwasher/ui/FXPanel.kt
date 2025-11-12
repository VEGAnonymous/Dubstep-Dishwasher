package lol.pony.dubstepdishwasher.ui

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Close
import androidx.compose.material.icons.filled.KeyboardArrowDown
import androidx.compose.material.icons.filled.KeyboardArrowUp
import androidx.compose.material3.Button
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TextField
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.scale
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.platform.LocalDensity
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
import lol.pony.dubstepdishwasher.viewmodel.EffectChainViewModel

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
        IconButton(onClick = { expanded = !expanded }) { Icon(Icons.Filled.Add, contentDescription = "Add FX") }
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
    onSetParam: (Int, Int, Any) -> Unit
) {
    LazyColumn {
        itemsIndexed(items = effects, key = { _, fx -> fx.effectId }) { index, fx ->
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(vertical = 8.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Column {
                    Text(fx.effectType.uiName, style = MaterialTheme.typography.titleMedium, modifier = Modifier.width(120.dp))
                    Text("Id: ${fx.effectId}")
                }

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
                    Icon(Icons.Filled.Close, contentDescription = "Remove") }
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
    onSetParam: (Int, Int, Any) -> Unit
) {
    LazyRow(Modifier.fillMaxWidth()) {
        items(items = effect.parameters.toList(), key = { it.id }) { param ->
            var paramNameWidth by remember { mutableIntStateOf(0) }
            val density = LocalDensity.current

            Column(horizontalAlignment = Alignment.CenterHorizontally, modifier = Modifier.padding(horizontal = 2.dp)) {
                Text(
                    text = param.name,
                    modifier = Modifier.onSizeChanged {
                        paramNameWidth = it.width
                    }
                )
                Text(
                    if (param is EffectParameter.Toggle) {
                        if (param.value) "On" else "Off"
                    } else {
                        param.value.toString()
                    }
                )
                when(param) {
//                    is EffectParameter.Range -> {
//
//                    }
                    is EffectParameter.Discrete<*> -> {
                        var expanded by remember { mutableStateOf(false) }
                        Box {
                            Button(onClick = { expanded = true }) {
                                Text(param.value.toString())
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
                                        text = { Text(option.toString()) },
                                        onClick = {
                                            onSetParam(effect.effectId, param.id, option!!)
                                            expanded = false
                                        })
                                }
                            }
                        }
                    }
                    is EffectParameter.Toggle -> {
                        Switch(
                            modifier = Modifier.scale(0.8f),
                            checked = param.value,
                            onCheckedChange = { onSetParam(effect.effectId, param.id, !param.value) })
                    }
                    else -> {
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
                        Button(onClick = { paramValue.toFloatOrNull()?.let { onSetParam(effect.effectId, param.id, it) }}) {
                            Text("Set")
                        }
                    }
                }
            }
        }
    }
}

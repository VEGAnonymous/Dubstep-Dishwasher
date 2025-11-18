package lol.pony.dubstepdishwasher.ui.components

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material3.Button
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.scale
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import lol.pony.dubstepdishwasher.CPU_LIMIT
import lol.pony.dubstepdishwasher.model.core.EffectType
@Composable
fun EffectChainControls(onAdd: (EffectType) -> Unit, onClear: () -> Unit, currUsage: Float) {
    Row (modifier = Modifier.padding(horizontal = 4.dp, vertical = 4.dp).height(40.dp), horizontalArrangement = Arrangement.Center) {
        var expanded by remember { mutableStateOf(false) }

        // Add effect button + dropdown
        Box {
            IconButton(onClick = { expanded = !expanded }) {
                Icon(Icons.Filled.Add, contentDescription = "Add FX")
            }
            DropdownMenu(expanded = expanded, onDismissRequest = { expanded = false }) {
                EffectType.entries.forEach { type ->
                    val usageDisplay = type.cpuUsage?.let { "$it%" } ?: "N/A"

                    val overLimit = (type.cpuUsage ?: 0f) + currUsage > CPU_LIMIT
                    DropdownMenuItem(
                        text = { Text(
                            text = "${type.uiName} - $usageDisplay",
                            style = MaterialTheme.typography.bodyMedium,
                            color = if (overLimit) Color.Red else Color.Unspecified

                        )},
                        onClick = {
                            onAdd(type)
                            expanded = false
                        }
                    )
                }
            }
        }

        Spacer(modifier = Modifier.width(22.dp))

        // Clear button
        Button(onClick = onClear, modifier = Modifier.padding(horizontal = 4.dp).scale(0.6f)) {
            Text("Clear", style = MaterialTheme.typography.bodyMedium)
        }
    }
}

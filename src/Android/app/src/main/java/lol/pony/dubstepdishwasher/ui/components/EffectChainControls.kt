package lol.pony.dubstepdishwasher.ui.components

import android.media.MediaPlayer
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.scale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import lol.pony.dubstepdishwasher.model.core.EffectType
import lol.pony.dubstepdishwasher.model.core.lerp
import lol.pony.dubstepdishwasher.R

@Composable
fun EffectChainControls(
    onAdd: (EffectType) -> Unit,
    onClear: () -> Unit,
    resourceError: String?,
    onDismissError: () -> Unit
) {
    Row (modifier = Modifier.padding(horizontal = 4.dp, vertical = 4.dp).height(40.dp), horizontalArrangement = Arrangement.Center) {
        var expanded by remember { mutableStateOf(false) }

        // Add effect button + dropdown
        Box {
            IconButton(onClick = { expanded = !expanded }) {
                Icon(Icons.Filled.Add, contentDescription = "Add FX")
            }
            DropdownMenu(expanded = expanded, onDismissRequest = { expanded = false }) {
                EffectType.entries.forEach { type ->
                    val compute = type.resourceUsage.compute; val memory = type.resourceUsage.memory

                    // Compute
                    val computeRatio = (compute / 0.20f).coerceIn(0f, 1f)
                    val computeDotColor = resourceColor(computeRatio)
                    val computeDotSize = lerp(8.dp, 16.dp, computeRatio)
                    // Memory
                    val memoryRatio = (memory.toFloat() / 200f).coerceIn(0f, 1f)
                    val memoryDotColor = resourceColor(memoryRatio)
                    val memoryDotSize = lerp(8.dp, 16.dp, memoryRatio)

                    DropdownMenuItem(
                        text = {
                            Row(verticalAlignment = Alignment.CenterVertically) {
                                Text(
                                    text = type.uiName,
                                    style = MaterialTheme.typography.bodyMedium,
                                    modifier = Modifier.width(130.dp)
                                )
                                Spacer(Modifier.width(30.dp))
                                Box(modifier = Modifier
                                        .size(computeDotSize)
                                        .background(computeDotColor, shape = CircleShape)
                                )
                                Spacer(Modifier.width(8.dp))
                                Box(modifier = Modifier
                                        .size(memoryDotSize)
                                        .background(memoryDotColor, shape = CircleShape)
                                )
                            }
                        },
                        onClick = {
                            onAdd(type)
                            expanded = false
                        }
                    )
                }
            }
        }

        Spacer(modifier = Modifier.width(28.dp))

        // Clear button
        Button(
            onClick = onClear,
            modifier = Modifier.padding(horizontal = 4.dp).scale(0.6f),
            shape = RoundedCornerShape(16.dp),
        ) {
            Text("CLEAR", style = MaterialTheme.typography.labelMedium)
        }
    } // Row

    if (resourceError != null) { // Exceeded resource limit
        val context = LocalContext.current
        LaunchedEffect(Unit) {
            val mediaPlayer = MediaPlayer.create(context, R.raw.ohno) // :pinkiesad:
            mediaPlayer.setOnCompletionListener { it.release() }
            mediaPlayer.start()
        }
        AlertDialog(
            onDismissRequest = { onDismissError() },
            title = { Text(text = "Oh no!", style = MaterialTheme.typography.bodyMedium) }, // :(
            text = { Text(text = resourceError, style = MaterialTheme.typography.bodyMedium) },
            confirmButton = {
                TextButton(onClick = { onDismissError() }) {
                    Text(text = "OK", style = MaterialTheme.typography.bodyMedium)
                }
            }
        )
    }

} // EffectChainControls

package lol.pony.dubstepdishwasher.ui.components

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Close
import androidx.compose.material.icons.filled.KeyboardArrowDown
import androidx.compose.material.icons.filled.KeyboardArrowUp
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.scale
import androidx.compose.ui.unit.dp
import lol.pony.dubstepdishwasher.R
import lol.pony.dubstepdishwasher.model.core.Effect
import lol.pony.dubstepdishwasher.ui.controls.DDSwitch

@Composable
fun EffectList(
    effects: List<Effect>,
    onToggleBypass: (Int) -> Unit,
    onRemove: (Int) -> Unit,
    onReorder: (Int, Int) -> Unit,
) {
    LazyColumn {
        itemsIndexed(items = effects, key = { _, fx -> fx.effectId }) { index, fx ->
            Row (
                modifier = Modifier
                    .fillMaxWidth()
                    .height(40.dp)
                    .padding(horizontal = 16.dp, vertical = 8.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {

                // Effect name
                Column {
                    Text(fx.effectType.uiName, style = MaterialTheme.typography.headlineSmall, modifier = Modifier.width(120.dp))
                    // Text("Id: ${fx.effectId}")
                    // Text("Index: $index")
                }

                // Reorder effect buttons
                // Doesn't show the down arrow currently but I don't care lol
                // TEMP: Potentially switch to drag and drop
                Column (modifier = Modifier.fillMaxHeight()) {
                    IconButton(
                        onClick = { onReorder(fx.effectId, index - 1) },
                        enabled = index > 0
                    ) { Icon(Icons.Filled.KeyboardArrowUp, contentDescription = "Move Up") }

                    IconButton(
                        onClick = { onReorder(fx.effectId, index + 1) },
                        enabled = index < effects.size - 1
                    ) { Icon(Icons.Filled.KeyboardArrowDown, contentDescription = "Move Down") }
                }

                // Spacer(modifier = Modifier.width(40.dp))

                // Bypass switch
                DDSwitch(
                    modifier = Modifier.scale(1f).padding(end = 8.dp),
                    checked = fx.isBypassed,
                    onCheckedChange = { onToggleBypass(fx.effectId) },
                    imageRes = R.drawable.control_bypass
                )

                // Remove effect button
                IconButton(onClick = { onRemove(fx.effectId) }, modifier = Modifier.scale(0.8f)) {
                    Icon(Icons.Filled.Close, contentDescription = "Remove") }
            }

            HorizontalDivider()

        }
    }
}
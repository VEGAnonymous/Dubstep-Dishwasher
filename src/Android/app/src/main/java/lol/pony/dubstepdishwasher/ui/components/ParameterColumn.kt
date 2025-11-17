package lol.pony.dubstepdishwasher.ui.components

import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import lol.pony.dubstepdishwasher.model.core.Effect
import lol.pony.dubstepdishwasher.model.core.ModAssignment
import lol.pony.dubstepdishwasher.model.core.Modulator
import lol.pony.dubstepdishwasher.model.core.ParamKey

@Composable
fun ParameterColumn(
    effects: List<Effect>,
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
    LazyColumn {
        items(effects, key = { it.effectId }) { fx ->
            Row (
                modifier = Modifier
                    .fillMaxWidth()
                    .height(100.dp)
                    .padding(horizontal = 16.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                // TEMP: Generic text label per row; replace with cooler graphic later
                Text(
                    text = fx.effectType.uiName,
                    style = MaterialTheme.typography.headlineMedium,
                    modifier = Modifier.width(100.dp)
                )

                // Parameter list rows
                ParameterList(
                    modifier = Modifier.fillMaxWidth(),
                    effect = fx,
                    assignments = assignments,
                    selectedModulator = selectedModulator,
                    currentModOffsets = currentModOffsets,

                    onSetParam = onSetParam,
                    onAssignMod = onAssignMod,
                    onRemoveMod = onRemoveMod,
                    onModAmountChange = onModAmountChange,
                    onTogglePolarity = onTogglePolarity
                )
            }

            HorizontalDivider()
        }
    }
}
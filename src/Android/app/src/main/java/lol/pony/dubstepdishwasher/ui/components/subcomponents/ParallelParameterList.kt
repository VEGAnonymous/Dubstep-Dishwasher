package lol.pony.dubstepdishwasher.ui.components.subcomponents

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Edit
import androidx.compose.material3.Button
import androidx.compose.material3.Icon
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
fun ParallelParameterList(
    modifier: Modifier = Modifier,
    effect: Effect,
    assignments: List<ModAssignment>,
    selectedModulator: Modulator?,
    currentModOffsets: Map<ParamKey, Float>,
    onSetParam: (Int, Int, Any) -> Unit,
    onAssignMod: (String, Int, Int) -> Unit,
    onRemoveMod: (String, Int, Int) -> Unit,
    onModAmountChange: (String, Int, Int, Float) -> Unit,
    onTogglePolarity: (String, Int, Int) -> Unit,
    onOpenEditor: () -> Unit
) {
    val params = effect.parameters.toList()

    LazyRow(
        modifier.fillMaxWidth(),
        contentPadding = PaddingValues(end = 12.dp)
    ) {
        items(items = params, key = { it.id }) { param ->
            ParameterItem(
                effect = effect,
                param = param,
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

        // Parallel editor
        item {
            Column(
                modifier = Modifier
                    .width(100.dp)
                    .fillMaxHeight()
                    .padding(start = 24.dp),
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.Center
            ) {
                Button(
                    onClick = onOpenEditor,
                    modifier = Modifier.fillMaxWidth(),
                    shape = RoundedCornerShape(16.dp),
                    contentPadding = PaddingValues(start = 0.dp, end = 4.dp, top = 2.dp, bottom = 6.dp),
                ) {
                    Icon(
                        imageVector = Icons.Filled.Edit,
                        contentDescription = "Edit",
                        modifier = Modifier
                            .size(20.dp)
                            .padding(top = 4.dp, end = 4.dp))
                    Text(
                        text = "Edit",
                        style = MaterialTheme.typography.bodySmall,
                        maxLines = 1
                    )
                }
            }
        }
    }
}
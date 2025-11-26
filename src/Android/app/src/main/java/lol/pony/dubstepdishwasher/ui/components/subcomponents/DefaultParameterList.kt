package lol.pony.dubstepdishwasher.ui.components.subcomponents

import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import lol.pony.dubstepdishwasher.model.core.Effect
import lol.pony.dubstepdishwasher.model.core.ModAssignment
import lol.pony.dubstepdishwasher.model.core.Modulator
import lol.pony.dubstepdishwasher.model.core.ParamKey

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun DefaultParameterList(
    modifier: Modifier = Modifier,
    effect: Effect,
    assignments: List<ModAssignment>,
    selectedModulator: Modulator?,
    currentModOffsets: Map<ParamKey, Float>,
    withinParallel: Boolean = false,
    // Callbacks
    onSetParam: (Int, Int, Any) -> Unit,
    onAssignMod: (String, Int, Int) -> Unit,
    onRemoveMod: (String, Int, Int) -> Unit,
    onModAmountChange: (String, Int, Int, Float) -> Unit,
    onTogglePolarity: (String, Int, Int) -> Unit
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
                withinParallel = withinParallel,
                onSetParam = onSetParam,
                onAssignMod = onAssignMod,
                onRemoveMod = onRemoveMod,
                onModAmountChange = onModAmountChange,
                onTogglePolarity = onTogglePolarity
            )
        }
    }
}
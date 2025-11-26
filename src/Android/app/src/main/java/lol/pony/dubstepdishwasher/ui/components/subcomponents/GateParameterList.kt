package lol.pony.dubstepdishwasher.ui.components.subcomponents

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.dp
import lol.pony.dubstepdishwasher.model.core.Effect
import lol.pony.dubstepdishwasher.model.core.EffectParameter
import lol.pony.dubstepdishwasher.model.core.ModAssignment
import lol.pony.dubstepdishwasher.model.core.Modulator
import lol.pony.dubstepdishwasher.model.core.ParamKey
import lol.pony.dubstepdishwasher.model.core.modValue

@Composable
fun GateParameterList(
    modifier: Modifier = Modifier,
    effect: Effect,
    assignments: List<ModAssignment>,
    selectedModulator: Modulator?,
    currentModOffsets: Map<ParamKey, Float>,
    withinParallel: Boolean = false,
    onSetParam: (Int, Int, Any) -> Unit,
    onAssignMod: (String, Int, Int) -> Unit,
    onRemoveMod: (String, Int, Int) -> Unit,
    onModAmountChange: (String, Int, Int, Float) -> Unit,
    onTogglePolarity: (String, Int, Int) -> Unit
) {
    val params = effect.parameters.toList()
    @Suppress("UNCHECKED_CAST")
    val threshold = modValue(effect, params[1] as EffectParameter.Range, currentModOffsets)
    val invert = params[5].getValueAny() as Boolean

    Row(
        modifier = modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.Start
    ) {
        LazyRow(
            modifier = Modifier
                .weight(1f),
            contentPadding = PaddingValues(end = 12.dp)
        ) {
            items(params, key = { it.id }) { param ->
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

        GatePlot(
            threshold = threshold,
            invert = invert,
            modifier = Modifier
                .width(140.dp)
                .height(100.dp)
                .padding(10.dp)
        )
    }
}

/* BACKEND */

private const val MIN_DB_IN = -60f
private const val MAX_DB_IN = 0f
private const val MIN_DB_OUT = -60f
private const val MAX_DB_OUT = 0f

@Composable
fun GatePlot(
    threshold: Float,
    invert: Boolean,
    modifier: Modifier = Modifier
) {
    val samples = remember(threshold, invert) { gateCurve(threshold, invert) }

    val curveColor = Color(0xFF5281D9)
    Canvas(modifier = modifier.background(MaterialTheme.colorScheme.surfaceVariant)) {
        val w = size.width; val h = size.height

        val path = Path()
        samples.forEachIndexed { i, (input, output) ->
            val px = ((input - MIN_DB_IN) / (MAX_DB_IN - MIN_DB_IN)) * w
            val py = h - ((((output - MIN_DB_OUT) / (MAX_DB_OUT - MIN_DB_OUT))) * h)

            if (i == 0) path.moveTo(px, py)
            else path.lineTo(px, py)
        }

        drawPath(
            path = path,
            color = curveColor,
            style = Stroke(width = 2.5f)
        )
    }
}

fun gateCurve(threshold: Float, invert: Boolean): List<Pair<Float, Float>> {
    val n = 256
    val step = (MAX_DB_IN - MIN_DB_IN) / (n - 1)

    return List(n) { i ->
        val x = MIN_DB_IN + i * step
        val y = if (!invert) { if (x < threshold) MIN_DB_OUT else x }
        else if (x < threshold) x else MIN_DB_OUT
        x to y
    }
}


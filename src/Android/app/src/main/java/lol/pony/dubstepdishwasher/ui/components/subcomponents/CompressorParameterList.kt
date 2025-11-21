package lol.pony.dubstepdishwasher.ui.components.subcomponents

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
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
import androidx.compose.ui.geometry.Offset
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

@Suppress("UNCHECKED_CAST")
@Composable
fun CompressorParameterList(
    modifier: Modifier = Modifier,
    effect: Effect,
    assignments: List<ModAssignment>,
    selectedModulator: Modulator?,
    currentModOffsets: Map<ParamKey, Float>,
    onSetParam: (Int, Int, Any) -> Unit,
    onAssignMod: (String, Int, Int) -> Unit,
    onRemoveMod: (String, Int, Int) -> Unit,
    onModAmountChange: (String, Int, Int, Float) -> Unit,
    onTogglePolarity: (String, Int, Int) -> Unit
) {
    val params = effect.parameters.toList()

    val threshold = modValue(effect, params[1] as EffectParameter.Range<Float>, currentModOffsets)
    val ratio = modValue(effect, params[2] as EffectParameter.Range<Float>, currentModOffsets)
    val knee = modValue(effect, params[3] as EffectParameter.Range<Float>, currentModOffsets)
    val makeup = modValue(effect, params[6] as EffectParameter.Range<Float>, currentModOffsets)

    Row(
        modifier = modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.Start
    ) {
        LazyRow(
            modifier = Modifier
                .weight(1f)
                .padding(end = 12.dp)
        ) {
            items(params, key = { it.id }) { param ->
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
        }

        CompressorPlot(
            threshold = threshold,
            ratio = ratio,
            knee = knee,
            makeup = makeup,
            modifier = Modifier
                .width(140.dp)
                .height(100.dp)
                .padding(10.dp)
        )
    }
}

/* BACKEND */

private const val MIN_DB_IN = -60f
private const val MAX_DB_IN = 10f

@Composable
fun CompressorPlot(
    threshold: Float,
    ratio: Float,
    knee: Float,
    makeup: Float,
    modifier: Modifier = Modifier
) {
    val samples = remember(threshold, ratio, knee, makeup) {
        compressorCurve(threshold, ratio, knee, makeup)
    }

    Canvas(modifier = modifier.background(MaterialTheme.colorScheme.surfaceVariant)) {
        val w = size.width; val h = size.height

        val path = Path()
        samples.forEachIndexed { i, (input, output) ->
            val px = ((input - MIN_DB_IN) / (MAX_DB_IN - MIN_DB_IN)) * w
            val py = h - (((output - MIN_DB_IN) / (MAX_DB_IN - MIN_DB_IN)) * h)

            if (i == 0) path.moveTo(px, py)
            else path.lineTo(px, py)
        }

        // Draw threshold axis
        val thresholdX = ((threshold - MIN_DB_IN) / (MAX_DB_IN - MIN_DB_IN)) * w
        drawLine(
            color = Color(0x22FFFFFF),
            start = Offset(thresholdX, 0f),
            end = Offset(thresholdX, h),
            strokeWidth = 1.5.dp.toPx()
        )

        // Draw path
        drawPath(
            path = path,
            color = Color(0xFF00CCAA),
            style = Stroke(width = 2.5f)
        )
    }
}

fun compressorCurve(threshold: Float, ratio: Float, knee: Float, makeup: Float): List<Pair<Float, Float>> {
    val n = 256
    val step = (MAX_DB_IN - MIN_DB_IN) / (n - 1)

    return List(n) { i ->
        val x = MIN_DB_IN + (i * step)
        val y = compressorGain(x, threshold, ratio, knee) + makeup
        x to y
    }
}

@Suppress("LocalVariableName")
fun compressorGain(x: Float, threshold: Float, ratio: Float, knee: Float): Float {
    // https://www.desmos.com/calculator/wkmkrmn9le
    val T = threshold; val R = ratio; val W = knee
    val thresholdLower = T - W / 2f; val thresholdUpper = T + W / 2f

    return when {
        x < thresholdLower -> x // Linear
        x > thresholdUpper -> T + (x - T) / R // Compressed
        else -> { // Knee region
            val dx = x - T + (W / 2f)
            x + (((1f / R) - 1f) * (dx * dx)) / (2f * W)
        }
    }
}
package lol.pony.dubstepdishwasher.ui.components.subcomponents

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.Fill
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.dp
import lol.pony.dubstepdishwasher.model.core.*
import kotlin.math.*

@Composable
fun DistortionParameterList(
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
    scrollable: Boolean = false
) {
    val params = effect.parameters.toList()
    val mode = params[1].value as DistortionMode
    @Suppress("UNCHECKED_CAST")
    val drive = modValue(effect, params[2] as EffectParameter.Range<Float>, currentModOffsets)

    Row(
        modifier = modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.Start
    ) {

        LazyRow(
            modifier = Modifier
                .weight(1f)
                .padding(end = 12.dp),
            userScrollEnabled = scrollable
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

        DistortionPlot(
            mode = mode,
            drive = drive,
            modifier = Modifier
                .width(140.dp)
                .height(100.dp)
                .padding(10.dp)
        )
    }
}

@Composable
fun DistortionPlot(
    mode: DistortionMode,
    drive: Float,
    modifier: Modifier = Modifier
) {
    val samples = remember(mode, drive) { distortionCurve(mode, drive) }

    val curveColor = Color(0xFFFF9C58)
    val gradColor = curveColor.copy(alpha = 0.35f)
    Canvas(modifier = modifier.background(MaterialTheme.colorScheme.surfaceVariant)) {
        val w = size.width; val h = size.height
        val midY = h / 2f; val midX = w / 2f

        val strokePath = Path(); val leftFill = Path(); val rightFill = Path()
        samples.forEachIndexed { i, (x, y) ->
            val px = (x + 1f) / 2f * w
            val py = midY - (y * (h / 2f))

            // Stroke path
            if (i == 0) strokePath.moveTo(px, py) else strokePath.lineTo(px, py)

            // Fill paths (y=0 split)
            if (px <= midX) {
                if (leftFill.isEmpty) leftFill.moveTo(px, midY)
                leftFill.lineTo(px, py)
            } else {
                if (rightFill.isEmpty) rightFill.moveTo(px, midY)
                rightFill.lineTo(px, py)
            }
        }

        // Close fills
        if (!leftFill.isEmpty) {
            leftFill.lineTo(0f, midY)
            leftFill.close()
        }
        if (!rightFill.isEmpty) {
            rightFill.lineTo(w, midY)
            rightFill.close()
        }

        drawPath( // Fill left
            path = leftFill,
            brush = Brush.verticalGradient(colors = listOf(Color.Transparent, gradColor)),
            style = Fill
        )

        drawPath( // Fill right
            path = rightFill,
            brush = Brush.verticalGradient(colors = listOf(gradColor, Color.Transparent)),
            style = Fill
        )

        // Curve outline
        drawPath(
            path = strokePath,
            color = curveColor,
            style = Stroke(width = 2.5f)
        )
    }
}

fun distortionCurve(mode: DistortionMode, drive: Float): List<Pair<Float, Float>> {
    // https://www.desmos.com/calculator/qrqipgp7r4
    fun tube(x: Float): Float {
        val d = 2f + (drive * 11f) // Doesn't match ground truth but for visual contrast
        return atan(x * d) * (2f / PI.toFloat())
    }
    fun softClip(x: Float): Float {
        val d = 1.3f + (drive * 3.7f)
        val thresh = 1f / d
        val v = when {
            x < -thresh -> -2f / 3f
            x >  thresh ->  2f / 3f
            else -> {
                val t = x * d
                t - (t * t * t) / 3f
            }
        }
        return v * 1.5f
    }
    fun hardClip(x: Float): Float {
        val d = 1f + (drive * 4f)
        return (x * d).coerceIn(-1f, 1f)
    }
    fun diode(x: Float): Float {
        val d = 2f + (drive * 8f)
        val v = x * d
        return when {
            v < 0f -> exp(v) - 1f
            v > 0f -> 1f - exp(-v)
            else -> 0f
        }
    }
    fun bitCrush(x: Float): Float {
        val bitDepth = (2 + ((1f - drive) * (1f - drive) * 14f))
        val v = (bitDepth - (drive * drive * drive * 1.5f)).coerceAtLeast(1.0f)
        val levels = (1 shl v.toInt()).toFloat()
        return round(x * levels) / levels
    }
    fun rectify(x: Float): Float {
        val d = 1f + (drive * 4f)
        return min(abs(x * d), 1f)
    }
    fun saturate(x: Float): Float {
        val d = 2f + (drive * 8f)
        return tanh(x * d)
    }

    val function: (Float) -> Float = when (mode) {
        DistortionMode.TUBE -> ::tube
        DistortionMode.SOFT_CLIP -> ::softClip
        DistortionMode.HARD_CLIP -> ::hardClip
        DistortionMode.DIODE -> ::diode
        DistortionMode.BITCRUSH -> ::bitCrush
        DistortionMode.RECTIFY -> ::rectify
        DistortionMode.SATURATE -> ::saturate
    }

    // Sample uniformly across [-1, 1]
    val steps = 200
    return (0..steps).map { i ->
        val x = (i / steps.toFloat()) * 2f - 1f
        x to function(x).coerceIn(-1f, 1f)
    }
}
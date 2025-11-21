package lol.pony.dubstepdishwasher.ui.components.subcomponents

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
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
    onTogglePolarity: (String, Int, Int) -> Unit
) {
    val params = effect.parameters.toList()
    val mode = params[1].value as DistortionMode

    @Suppress("UNCHECKED_CAST")
    val driveParam = effect.parameters[2] as EffectParameter.Range<Float>
    val modOffset = currentModOffsets[ParamKey(effect.effectId, driveParam.id)] ?: 0f
    val drive = (driveParam.value + modOffset).coerceIn(driveParam.range.first, driveParam.range.second)

    LazyRow(modifier = modifier.fillMaxWidth()) {
        params.forEach { param ->
            item(key = param.id) {
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
            // Distortion curve plot
            if (param.id == 2) {
                item(key = "distortion_plot_${effect.effectId}") {
                    DistortionPlot(
                        mode = mode,
                        drive = drive,
                        modifier = Modifier
                            .width(120.dp)
                            .height(80.dp)
                            .padding(start = 16.dp, end = 8.dp, top = 10.dp)
                    )
                }
            }
        }
    }
}

@Composable
fun DistortionPlot(
    mode: DistortionMode,
    drive: Float,
    modifier: Modifier = Modifier
) {
    val samples = remember(mode, drive) { distortionCurve(mode, drive) }

    Canvas(modifier = modifier.background(MaterialTheme.colorScheme.surfaceVariant)) {
        val w = size.width; val h = size.height; val midY = h / 2f

        // Build path
        val path = Path()
        samples.forEachIndexed { i, (x, y) ->
            val px = (x + 1f) / 2f * w
            val py = midY - (y * (h / 2f))

            if (i == 0) path.moveTo(px, py)
            else path.lineTo(px, py)
        }

        // Draw curve
        drawPath(
            path = path,
            color = Color(0xFF00CCAA),
            style = Stroke(width = 2.5f)
        )
    }
}

fun distortionCurve(mode: DistortionMode, drive: Float): List<Pair<Float, Float>> {
    fun tube(x: Float): Float {
        val d = 4f + drive * 11f
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
@file:Suppress("LocalVariableName")

package lol.pony.dubstepdishwasher.ui.components.subcomponents

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.dp
import lol.pony.dubstepdishwasher.model.core.*
import kotlin.math.PI
import kotlin.math.cos
import kotlin.math.exp
import kotlin.math.ln
import kotlin.math.log10
import kotlin.math.max
import kotlin.math.pow
import kotlin.math.sin
import kotlin.math.sqrt

@Suppress("UNCHECKED_CAST")
@Composable
fun EqualizerParameterList(
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

    // Band 1
    val b1Type = params[1].value as BiquadType
    val b1Cutoff = modValue(effect, params[2] as EffectParameter.Range<Float>, currentModOffsets)
    val b1Q = modValue(effect, params[3] as EffectParameter.Range<Float>, currentModOffsets)
    val b1Gain = modValue(effect, params[4] as EffectParameter.Range<Float>, currentModOffsets)

    // Band 2
    val b2Type = params[5].value as BiquadType
    val b2Cutoff = modValue(effect, params[6] as EffectParameter.Range<Float>, currentModOffsets)
    val b2Q = modValue(effect, params[7] as EffectParameter.Range<Float>, currentModOffsets)
    val b2Gain = modValue(effect, params[8] as EffectParameter.Range<Float>, currentModOffsets)

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

        EqualizerPlot(
            b1Type = b1Type,
            b1Cutoff = b1Cutoff,
            b1Q = b1Q,
            b1Gain = b1Gain,
            b2Type = b2Type,
            b2Cutoff = b2Cutoff,
            b2Q = b2Q,
            b2Gain = b2Gain,
            modifier = Modifier
                .width(140.dp)
                .height(100.dp)
                .padding(top = 10.dp, bottom = 10.dp, start = 10.dp, end = 10.dp)
        )
    }
}

/* BACKEND */

private const val MIN_FREQ = 20.0f
private const val MAX_FREQ = 20000.0f
private const val MIN_DB = -24.0f
private const val MAX_DB = 24.0f
private const val DB_RANGE = MAX_DB - MIN_DB

data class EqualizerResponse(val freq: Float, val gainDb: Float)


@Composable
fun EqualizerPlot(
    b1Type: BiquadType,
    b1Cutoff: Float,
    b1Q: Float,
    b1Gain: Float,
    b2Type: BiquadType,
    b2Cutoff: Float,
    b2Q: Float,
    b2Gain: Float,
    modifier: Modifier = Modifier
) {
    val samples = remember(b1Type, b1Cutoff, b1Q, b1Gain, b2Type, b2Cutoff, b2Q, b2Gain) {
        calculateEqualizerResponse(
            b1Type, b1Cutoff, b1Q, b1Gain,
            b2Type, b2Cutoff, b2Q, b2Gain
        )
    }

    Canvas(modifier = modifier.background(MaterialTheme.colorScheme.surfaceVariant)) {
        val w = size.width
        val h = size.height

        val path = Path()
        samples.forEachIndexed { i, sample ->
            val logMin = ln(MIN_FREQ)
            val logMax = ln(MAX_FREQ)
            val logFreq = ln(sample.freq.coerceIn(MIN_FREQ, MAX_FREQ))
            val px = ((logFreq - logMin) / (logMax - logMin)) * w

            val normalizedDb = (sample.gainDb.coerceIn(MIN_DB, MAX_DB) - MIN_DB) / DB_RANGE
            val py = h - (normalizedDb * h)

            if (i == 0) path.moveTo(px, py)
            else path.lineTo(px, py)
        }

        // 0dB line
        val y = h - (((0.0f - MIN_DB) / DB_RANGE) * h)
        drawLine(
            color = Color.DarkGray,
            start = Offset(0f, y),
            end = Offset(w, y),
            strokeWidth = 1f
        )

        // Draw curve
        drawPath(
            path = path,
            color = Color(0xFF00CCAA),
            style = Stroke(width = 2.5f)
        )
    }
}

// Generate log-spaced frequencies
private fun logFrequencies(@Suppress("SameParameterValue") n: Int): List<Double> {
    val startLog = ln(MIN_FREQ)
    val endLog = ln(MAX_FREQ)
    val step = (endLog - startLog) / (n - 1)
    return (0 until n).map { i ->
        exp(startLog + i * step).toDouble()
    }
}

fun calculateEqualizerResponse(
    b1Type: BiquadType, b1Cutoff: Float, b1Q: Float, b1Gain: Float,
    b2Type: BiquadType, b2Cutoff: Float, b2Q: Float, b2Gain: Float
): List<EqualizerResponse> {
    val frequencies = logFrequencies(n = 256)

    // Compute filter coefficients
    val b1Coefficients = biquadCoefficients(b1Type, b1Cutoff, b1Q, b1Gain)
    val b2Coefficients = biquadCoefficients(b2Type, b2Cutoff, b2Q, b2Gain)

    // Cascade frequency responses
    return frequencies.map { f ->
        val mag1 = magnitudeResponse(b1Coefficients, f)
        val mag2 = magnitudeResponse(b2Coefficients, f)

        val totalMag = mag1 * mag2
        val gainDb = 20.0f * log10(totalMag.coerceAtLeast(1e-6f))

        EqualizerResponse(f.toFloat(), gainDb)
    }
}

fun biquadCoefficients(type: BiquadType, cutoff: Float, q: Float, gainDb: Float): DoubleArray {
    // Coefficient calculations
    // Again courtesy of the godsend Audio EQ Cookbook
    val A = 10.0.pow(gainDb / 40.0)
    val omega0 = (2.0 * PI * cutoff.toDouble()) / SAMPLE_RATE
    val cosW0 = cos(omega0); val sinW0 = sin(omega0)
    val alpha = sinW0 / (2.0 * q)

    var b0: Double; var b1: Double; var b2: Double; var a0: Double; var a1: Double; var a2: Double

    val sqrtA = sqrt(A)
    val sqrtA_2a = 2.0f * sqrtA * alpha
    when (type) {
        BiquadType.PEAK -> {
            b0 = 1.0 + alpha * A
            b1 = -2.0 * cosW0
            b2 = 1.0 - alpha * A
            a0 = 1.0 + alpha / A
            a1 = -2.0 * cosW0
            a2 = 1.0 - alpha / A
        }
        BiquadType.LOW_PASS -> {
            b0 = (1.0 - cosW0) / 2.0
            b1 = 1.0 - cosW0
            b2 = (1.0 - cosW0) / 2.0
            a0 = 1.0 + alpha
            a1 = -2.0 * cosW0
            a2 = 1.0 - alpha
        }
        BiquadType.HIGH_PASS -> {
            b0 = (1.0 + cosW0) / 2.0
            b1 = -(1.0 + cosW0)
            b2 = (1.0 + cosW0) / 2.0
            a0 = 1.0 + alpha
            a1 = -2.0 * cosW0
            a2 = 1.0 - alpha
        }
        BiquadType.NOTCH -> {
            b0 = 1.0
            b1 = -2.0 * cosW0
            b2 = 1.0
            a0 = 1.0 + alpha
            a1 = -2.0 * cosW0
            a2 = 1.0 - alpha
        }
        BiquadType.LOW_SHELF -> {
            val Ap1 = A + 1.0
            val Am1 = A - 1.0

            b0 = A * (Ap1 - Am1 * cosW0 + sqrtA_2a)
            b1 = 2.0 * A * (Am1 - Ap1 * cosW0)
            b2 = A * (Ap1 - Am1 * cosW0 - sqrtA_2a)
            a0 = Ap1 + Am1 * cosW0 + sqrtA_2a
            a1 = -2.0 * (Am1 + Ap1 * cosW0)
            a2 = Ap1 + Am1 * cosW0 - sqrtA_2a
        }
        BiquadType.HIGH_SHELF -> {
            val Ap1 = A + 1.0; val Am1 = A - 1.0

            b0 = A * (Ap1 + Am1 * cosW0 + sqrtA_2a)
            b1 = -2.0 * A * (Am1 + Ap1 * cosW0)
            b2 = A * (Ap1 + Am1 * cosW0 - sqrtA_2a)
            a0 = Ap1 - Am1 * cosW0 + sqrtA_2a
            a1 = 2.0 * (Am1 - Ap1 * cosW0)
            a2 = Ap1 - Am1 * cosW0 - sqrtA_2a
        }
    }

    // Normalize
    if (a0 == 0.0) return doubleArrayOf(0.0, 0.0, 0.0, 0.0, 0.0)
    return doubleArrayOf(
        b0 / a0,
        b1 / a0,
        b2 / a0,
        a1 / a0,
        a2 / a0
    )
}

fun magnitudeResponse(coefficients: DoubleArray, f: Double): Float {
    val b0 = coefficients[0]; val b1 = coefficients[1]; val b2 = coefficients[2]
    val a1 = coefficients[3]; val a2 = coefficients[4]

    val w = (2.0 * PI * f) / SAMPLE_RATE
    val cosW = cos(w); val cos2W = cos(2.0 * w)

    val numSq = b0.pow(2) + b1.pow(2) + b2.pow(2) +
            2.0 * (b0 * b1 + b1 * b2) * cosW +
            2.0 * b0 * b2 * cos2W

    val denSq = 1.0 + a1.pow(2) + a2.pow(2) +
            2.0 * (a1 + a1 * a2) * cosW +
            2.0 * a2 * cos2W

    return sqrt(max(0.0, numSq / denSq)).toFloat()
}
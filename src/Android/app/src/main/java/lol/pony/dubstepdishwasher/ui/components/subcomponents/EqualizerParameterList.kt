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
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.Fill
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
    val b1Type = params[1].getValueAny() as BiquadType
    val b1Cutoff = modValue(effect, params[2] as EffectParameter.Range, currentModOffsets)
    val b1Q = modValue(effect, params[3] as EffectParameter.Range, currentModOffsets)
    val b1Gain = modValue(effect, params[4] as EffectParameter.Range, currentModOffsets)

    // Band 2
    val b2Type = params[5].getValueAny() as BiquadType
    val b2Cutoff = modValue(effect, params[6] as EffectParameter.Range, currentModOffsets)
    val b2Q = modValue(effect, params[7] as EffectParameter.Range, currentModOffsets)
    val b2Gain = modValue(effect, params[8] as EffectParameter.Range, currentModOffsets)

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

data class EqualizerResponse(val freq: Float, val gain: Float)

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
        frequencyResponse(b1Type, b1Cutoff, b1Q, b1Gain, b2Type, b2Cutoff, b2Q, b2Gain)
    }

    val curveColor = Color(0xFFFFFFFF)
    val gradColor = curveColor.copy(alpha = 0.35f)
    Canvas(modifier = modifier.background(MaterialTheme.colorScheme.surfaceVariant)) {
        val w = size.width; val h = size.height
        val y_0 = h - ((0f - MIN_DB) / DB_RANGE * h)

        val strokePath = Path(); val fillPath = Path()
        samples.forEachIndexed { i, sample ->
            // Log frequency
            val logMin = ln(MIN_FREQ); val logMax = ln(MAX_FREQ)
            val logFreq = ln(sample.freq.coerceIn(MIN_FREQ, MAX_FREQ))
            val px = ((logFreq - logMin) / (logMax - logMin)) * w

            // Linear dB
            val gain = sample.gain.coerceIn(MIN_DB, MAX_DB)
            val py = h - ((gain - MIN_DB) / DB_RANGE * h)

            if (i == 0) {
                strokePath.moveTo(px, py)
                fillPath.moveTo(px, y_0)
                fillPath.lineTo(px, py)
            } else {
                strokePath.lineTo(px, py)
                fillPath.lineTo(px, py)
            }
        }

        // Close fill
        val last = ln(samples.last().freq.coerceIn(MIN_FREQ, MAX_FREQ))
        val lastPx = ((last - ln(MIN_FREQ)) / (ln(MAX_FREQ) - ln(MIN_FREQ))) * w
        fillPath.lineTo(lastPx, y_0)
        fillPath.close()

        // Fill
        drawPath(
            path = fillPath,
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

// Generate log-spaced frequencies
private fun logFrequencies(@Suppress("SameParameterValue") n: Int): List<Double> {
    val startLog = ln(MIN_FREQ); val endLog = ln(MAX_FREQ)
    val step = (endLog - startLog) / (n - 1)
    return (0 until n).map { i -> exp(startLog + i * step).toDouble() }
}

fun frequencyResponse(
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
        val gain = 20.0f * log10((mag1 * mag2).coerceAtLeast(1e-6f)) // dB

        EqualizerResponse(f.toFloat(), gain)
    }
}

fun biquadCoefficients(type: BiquadType, cutoff: Float, q: Float, gain: Float): DoubleArray {
    // Coefficient calculations
    // Again courtesy of the godsend Audio EQ Cookbook
    val A = 10.0.pow(gain / 40.0)
    val w_0 = (2.0 * PI * cutoff.toDouble()) / SAMPLE_RATE
    val cos_w0 = cos(w_0); val sin_w0 = sin(w_0)
    val a = sin_w0 / (2.0 * q)

    var b0: Double; var b1: Double; var b2: Double; var a0: Double; var a1: Double; var a2: Double

    val sqrtA = sqrt(A)
    val sqrtA_2a = 2.0f * sqrtA * a
    when (type) {
        BiquadType.PEAK -> {
            b0 = 1.0 + (a * A)
            b1 = -2.0 * cos_w0
            b2 = 1.0 - (a * A)
            a0 = 1.0 + (a / A)
            a1 = -2.0 * cos_w0
            a2 = 1.0 - (a / A)
        }
        BiquadType.LOW_PASS -> {
            b0 = (1.0 - cos_w0) / 2.0
            b1 = 1.0 - cos_w0
            b2 = (1.0 - cos_w0) / 2.0
            a0 = 1.0 + a
            a1 = -2.0 * cos_w0
            a2 = 1.0 - a
        }
        BiquadType.HIGH_PASS -> {
            b0 = (1.0 + cos_w0) / 2.0
            b1 = -(1.0 + cos_w0)
            b2 = (1.0 + cos_w0) / 2.0
            a0 = 1.0 + a
            a1 = -2.0 * cos_w0
            a2 = 1.0 - a
        }
        BiquadType.NOTCH -> {
            b0 = 1.0
            b1 = -2.0 * cos_w0
            b2 = 1.0
            a0 = 1.0 + a
            a1 = -2.0 * cos_w0
            a2 = 1.0 - a
        }
        BiquadType.LOW_SHELF -> {
            val Ap1 = A + 1.0
            val Am1 = A - 1.0

            b0 = A * (Ap1 - (Am1 * cos_w0) + sqrtA_2a)
            b1 = 2.0 * A * (Am1 - (Ap1 * cos_w0))
            b2 = A * (Ap1 - (Am1 * cos_w0) - sqrtA_2a)
            a0 = Ap1 + (Am1 * cos_w0) + sqrtA_2a
            a1 = -2.0 * (Am1 + (Ap1 * cos_w0))
            a2 = Ap1 + (Am1 * cos_w0) - sqrtA_2a
        }
        BiquadType.HIGH_SHELF -> {
            val Ap1 = A + 1.0; val Am1 = A - 1.0

            b0 = A * (Ap1 + (Am1 * cos_w0) + sqrtA_2a)
            b1 = -2.0 * A * (Am1 + (Ap1 * cos_w0))
            b2 = A * (Ap1 + (Am1 * cos_w0) - sqrtA_2a)
            a0 = Ap1 - (Am1 * cos_w0) + sqrtA_2a
            a1 = 2.0 * (Am1 - (Ap1 * cos_w0))
            a2 = Ap1 - (Am1 * cos_w0) - sqrtA_2a
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
    val cos_w = cos(w); val cos_2w = cos(2.0 * w)

    val numSq = b0.pow(2) + b1.pow(2) + b2.pow(2) +
            (2.0 * ((b0 * b1) + (b1 * b2)) * cos_w) + (2.0 * b0 * b2 * cos_2w)

    val denSq = 1.0 + a1.pow(2) + a2.pow(2) +
            (2.0 * (a1 + (a1 * a2)) * cos_w) + (2.0 * a2 * cos_2w)

    return sqrt(max(0.0, numSq / denSq)).toFloat()
}
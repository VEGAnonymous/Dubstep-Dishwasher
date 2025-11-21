package lol.pony.dubstepdishwasher.ui.components.subcomponents

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Fill
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.delay

@Composable
fun RandomScope(
    modId: String,
    currentValue: Float,
    modifier: Modifier = Modifier,
    lineColor: Color = Color(0xFF00CCAA),
    gridColor: Color = Color(0x22FFFFFF).copy(alpha = 0.2f),
    backgroundColor: Color = MaterialTheme.colorScheme.surfaceVariant,
    maxHistory: Int = 100
) {
    val valueHistory = remember(modId) { mutableStateListOf<Float>() }
    val currentValueState = rememberUpdatedState(currentValue)

    // Update history with current value
    LaunchedEffect(modId) {
        while (true) {
            valueHistory.add(currentValueState.value)
            if (valueHistory.size > maxHistory) valueHistory.removeAt(0)
            delay(1000L / 50L) // 50 Hz
        }
    }

    Canvas(modifier = modifier.background(backgroundColor)) {
        if (valueHistory.isEmpty()) return@Canvas
        val w = size.width; val h = size.height

        // Draw grid line
        drawLine( // Horizontal
            color = gridColor,
            start = Offset(0f, h / 2f),
            end = Offset(w, h / 2f),
            strokeWidth = 1.5.dp.toPx()
        )

        // Read value history
        val path = Path()
        valueHistory.forEachIndexed { index, value ->
            val x = (index.toFloat() / maxHistory) * w
            val y = h - (value * h)

            if (index == 0) path.moveTo(x, y)
            else path.lineTo(x, y)
        }

        // Fill path
        val fillPath = Path()
        valueHistory.forEachIndexed { index, value ->
            val x = (index.toFloat() / maxHistory) * w
            val y = h - (value * h)
            if (index == 0) {
                fillPath.moveTo(x, h) // Start at bottom
                fillPath.lineTo(x, y) // Up to curve
            } else fillPath.lineTo(x, y)
        }

        // Close fill
        val lastX = ((valueHistory.size - 1).toFloat() / maxHistory) * w
        fillPath.lineTo(lastX, h)
        fillPath.close()

        // Fill
        drawPath(
            path = fillPath,
            brush = Brush.verticalGradient(
                colors = listOf(
                    lineColor.copy(alpha = 0.35f),
                    Color.Transparent
                )
            ),
            style = Fill
        )

        // Draw curve
        drawPath(
            path = path,
            color = lineColor,
            style = Stroke(width = 2.dp.toPx(), cap = StrokeCap.Round)
        )
    }
}
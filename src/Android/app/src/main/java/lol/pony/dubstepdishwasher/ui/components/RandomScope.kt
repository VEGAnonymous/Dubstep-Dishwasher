package lol.pony.dubstepdishwasher.ui.components

import androidx.compose.foundation.Canvas
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.delay

@Composable
fun RandomScope(
    modId: String,
    currentValue: Float,
    modifier: Modifier = Modifier,
    lineColor: Color = Color(0xFF00CCAA),
    maxHistory: Int = 100
) {
    val valueHistory = remember(modId) { mutableStateListOf<Float>() }
    val currentValueState = rememberUpdatedState(currentValue)

    // Update history with current value
    LaunchedEffect(modId) {
        while (true) {
            valueHistory.add(currentValueState.value)
            if (valueHistory.size > maxHistory) valueHistory.removeAt(0)
            delay(20) // 50Hz
        }
    }

    Canvas(modifier = modifier) {
        if (valueHistory.isEmpty()) return@Canvas
        val width = size.width; val height = size.height

        // Draw grid line
        drawLine( // Horizontal
            color = Color(0x22000000).copy(alpha = 0.2f),
            start = Offset(0f, height / 2f),
            end = Offset(width, height / 2f),
            strokeWidth = 1.5.dp.toPx()
        )

        // Draw value history
        val path = Path()
        valueHistory.forEachIndexed { index, value ->
            val x = (index.toFloat() / maxHistory) * width
            val y = height - (value * height)

            if (index == 0) path.moveTo(x, y)
            else path.lineTo(x, y)
        }

        drawPath(
            path = path,
            color = lineColor,
            style = Stroke(width = 2.dp.toPx(), cap = StrokeCap.Round)
        )
    }
}
package lol.pony.dubstepdishwasher.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import lol.pony.dubstepdishwasher.model.core.lerpColor

/* TOP BAR */

@Composable
fun ResourceMeter(
    compute: Float,
    memory: Int,
    maxCompute: Float,
    maxMemory: Int
) {
    Column(
        modifier = Modifier
            .width(100.dp)
            .padding(start = 16.dp),
        verticalArrangement = Arrangement.spacedBy(4.dp)
    ) {
        ResourceBar(value = compute / maxCompute) // CPU
        ResourceBar(value = memory.toFloat() / maxMemory.toFloat()) // RAM
    }
}

@Composable
private fun ResourceBar(value: Float) {
    val clamped = value.coerceIn(0f, 1f)

    val color = when {
        clamped > 0.9f -> Color(0xFFD96D52)
        clamped > 0.7f -> Color(0xFFD0C455)
        else -> MaterialTheme.colorScheme.primary
    }

    Column {
        Box(
            modifier = Modifier
                .height(6.dp)
                .fillMaxWidth()
                .background(MaterialTheme.colorScheme.surfaceVariant)
        ) {
            Box(
                modifier = Modifier
                    .fillMaxHeight()
                    .fillMaxWidth(clamped)
                    .background(color)
            )
        }
    }
}

/* ADD EFFECT DROPDOWN */

@Composable
fun resourceColor(ratio: Float): Color {
    val clamped = ratio.coerceIn(0f, 1f)

    val green = Color(0xFF4CAF50)   // Material green 500
    val yellow = Color(0xFFFFEB3B)  // Material yellow 500
    val red = Color(0xFFF44336)     // Material red 500

    return when {
        clamped < 0.7f -> {
            val t = clamped / 0.7f
            lerpColor(green, yellow, t)
        }
        else -> {
            val t = (clamped - 0.7f) / 0.3f
            lerpColor(yellow, red, t)
        }
    }
}
package lol.pony.dubstepdishwasher.ui.components.subcomponents

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Fill
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.dp
import lol.pony.dubstepdishwasher.model.core.CurvePoint
import lol.pony.dubstepdishwasher.model.core.EditableCurve
import lol.pony.dubstepdishwasher.model.core.applyCurve
import lol.pony.dubstepdishwasher.model.core.lerp

@Composable
fun CurveDisplay(
    points: List<CurvePoint>,
    modifier: Modifier = Modifier,
    currentPosition: Float? = null, // 0-1, null to hide
    showPoints: Boolean = true,
    fillGradient: Boolean = false,
    lineColor: Color = Color(0xFF00CCAA),
    pointColor: Color = Color(0xFFFFFFFF),
    curveHandleColor: Color = Color(0x88666666),
    positionColor: Color = Color(0xFF024F43),
    backgroundColor: Color = MaterialTheme.colorScheme.surfaceVariant,
    gridColor: Color = Color(0x22000000),
    gridX: Int = 8, // -1 to disable
    gridY: Int = 8
) {
    Canvas(modifier = modifier.background(backgroundColor)) {
        if (points.size < 2) return@Canvas

        val w = size.width; val h = size.height
        val sorted = points.sortedBy { it.x }

        // Draw grid
        if (gridX > 0) {
            for (i in 0..gridX) {
                val x = (i.toFloat() / gridX) * w
                val major = (gridX % 4 == 0 && i % (gridX / 4) == 0) ||
                        (gridX % 2 == 0 && i % (gridX / 2) == 0)
                drawLine(
                    color = if (major) gridColor.copy(alpha = 0.2f) else gridColor,
                    start = Offset(x, 0f),
                    end = Offset(x, h),
                    strokeWidth = if (major) 1.5.dp.toPx() else 1.dp.toPx()
                )
            }
        }
        if (gridY > 0) {
            for (i in 0..gridY) {
                val y = (i.toFloat() / gridY) * h
                val major = (gridY % 4 == 0 && i % (gridY / 4) == 0) ||
                        (gridY % 2 == 0 && i % (gridY / 2) == 0)
                drawLine(
                    color = if (major) gridColor.copy(alpha = 0.2f) else gridColor,
                    start = Offset(0f, y),
                    end = Offset(w, y),
                    strokeWidth = if (major) 1.5.dp.toPx() else 1.dp.toPx()
                )
            }
        }

        // Draw curve segments
        val fillPath = Path()
        for (i in 0 until sorted.lastIndex) {
            // Define segment region
            val p0 = sorted[i]; val p1 = sorted[i + 1]

            val path = Path()
            val startX = p0.x * w; val startY = h - (p0.y * h)
            path.moveTo(startX, startY)

            if (i == 0) fillPath.moveTo(startX, startY)

            // Sample points along the curve
            val segments = 50
            for (j in 1..segments) {
                val t = j / segments.toFloat()
                val curved = applyCurve(t, p0)

                val x = (p0.x + (p1.x - p0.x) * t) * w
                val y = h - ((p0.y + curved * (p1.y - p0.y)) * h)
                path.lineTo(x, y)
                fillPath.lineTo(x, y)
            }

            // Draw path
            drawPath(
                path = path,
                color = lineColor,
                style = Stroke(width = 2.dp.toPx(), cap = StrokeCap.Round)
            )
        }

        // Close fill path
        val lastX = sorted.last().x * w
        fillPath.lineTo(lastX, h)
        fillPath.lineTo(sorted.first().x * w, h)
        fillPath.close()

        // Fill
        if (fillGradient) {
            val grad = lineColor.copy(alpha = 0.35f)
            drawPath(
                path = fillPath,
                brush = Brush.verticalGradient(colors = listOf(grad, Color.Transparent)),
                style = Fill
            )
        }

        // Draw points
        if (showPoints) {
            sorted.forEach { point ->
                drawCircle(
                    color = pointColor,
                    radius = 4.dp.toPx(),
                    center = Offset(
                        x = point.x * w,
                        y = h - (point.y * h)
                    )
                )
            }

            // Draw curve handles at segment midpoints
            for (i in 0 until sorted.lastIndex) {
                // Define segment region
                val p0 = sorted[i]; val p1 = sorted[i + 1]

                // Evaluate curve at pos 0.5 to find handle position
                val t = 0.5f
                val curved = applyCurve(t, p0)

                val handleX = lerp(p0.x, p1.x, t) * w
                val handleY = h - (lerp(p0.y, p1.y, curved) * h)

                drawCircle(
                    color = curveHandleColor,
                    radius = 3.dp.toPx(),
                    center = Offset(handleX, handleY)
                )
            }
        }

        // Draw position indicator
        currentPosition?.let { pos ->
            // Evaluate curve at current position
            val curve = EditableCurve(points, loop = true)
            val yVal = curve.evaluate(pos)

            val x = pos * w; val y = h - (yVal * h)
            drawCircle(
                color = positionColor,
                alpha = 0.8f,
                radius = 3.dp.toPx(),
                center = Offset(x, y)
            )
        }
    }
}
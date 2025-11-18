package lol.pony.dubstepdishwasher.ui.components

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
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
    showPoints: Boolean = true,
    lineColor: Color = Color(0xFF00CCAA),
    pointColor: Color = Color(0xFFFFFFFF),
    curveHandleColor: Color = Color(0x88666666),
    backgroundColor: Color = Color.Transparent,
    gridX: Int = 8, // -1 to disable
    gridY: Int = 8,
    gridColor: Color = Color(0x22000000),
    currentPosition: Float? = null, // 0-1, null to hide
    positionColor: Color = Color(0xFF024F43),
) {
    Canvas(modifier = modifier.background(backgroundColor)) {
        if (points.size < 2) return@Canvas

        val width = size.width; val height = size.height
        val sorted = points.sortedBy { it.x }

        // Draw grid
        if (gridX > 0) {
            for (i in 0..gridX) {
                val x = (i.toFloat() / gridX) * width
                val isMajor = (gridX % 4 == 0 && i % (gridX / 4) == 0) ||
                        (gridX % 2 == 0 && i % (gridX / 2) == 0)
                drawLine(
                    color = if (isMajor) gridColor.copy(alpha = 0.2f) else gridColor,
                    start = Offset(x, 0f),
                    end = Offset(x, height),
                    strokeWidth = if (isMajor) 1.5.dp.toPx() else 1.dp.toPx()
                )
            }
        }
        if (gridY > 0) {
            for (i in 0..gridY) {
                val y = (i.toFloat() / gridY) * height
                val isMajor = (gridY % 4 == 0 && i % (gridY / 4) == 0) ||
                        (gridY % 2 == 0 && i % (gridY / 2) == 0)
                drawLine(
                    color = if (isMajor) gridColor.copy(alpha = 0.2f) else gridColor,
                    start = Offset(0f, y),
                    end = Offset(width, y),
                    strokeWidth = if (isMajor) 1.5.dp.toPx() else 1.dp.toPx()
                )
            }
        }

        // Draw curve segments
        for (i in 0 until sorted.lastIndex) {
            // Define segment region
            val p0 = sorted[i]; val p1 = sorted[i + 1]

            val path = Path()
            val startX = p0.x * width
            val startY = height - (p0.y * height)

            path.moveTo(startX, startY)

            // Sample points along the curve
            val segments = 50
            for (j in 1..segments) {
                val t = j / segments.toFloat()
                val curved = applyCurve(t, p0)

                val x = (p0.x + (p1.x - p0.x) * t) * width
                val y = height - ((p0.y + curved * (p1.y - p0.y)) * height)
                path.lineTo(x, y)
            }

            drawPath(
                path = path,
                color = lineColor,
                style = Stroke(width = 2.dp.toPx(), cap = StrokeCap.Round)
            )
        }

        // Draw points
        if (showPoints) {
            sorted.forEach { point ->
                drawCircle(
                    color = pointColor,
                    radius = 4.dp.toPx(),
                    center = Offset(
                        x = point.x * width,
                        y = height - (point.y * height)
                    )
                )
            }

            // Draw curve handles at segment midpoints
            for (i in 0 until sorted.lastIndex) {
                val p0 = sorted[i]
                val p1 = sorted[i + 1]

                // Evaluate curve at pos 0.5 to find handle position
                val t = 0.5f
                val curved = applyCurve(t, p0)

                val handleX = lerp(p0.x, p1.x, t) * width
                val handleY = height - (lerp(p0.y, p1.y, curved) * height)

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
            val yValue = curve.evaluate(pos)

            val x = pos * width
            val y = height - (yValue * height)

            drawCircle(
                color = positionColor,
                alpha = 0.8f,
                radius = 3.dp.toPx(),
                center = Offset(x, y)
            )
        }
    }
}
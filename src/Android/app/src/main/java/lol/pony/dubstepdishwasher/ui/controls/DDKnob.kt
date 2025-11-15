package lol.pony.dubstepdishwasher.ui.controls

import androidx.annotation.DrawableRes
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.res.imageResource
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.IntRect
import androidx.compose.ui.unit.IntSize
import androidx.compose.ui.unit.dp
import kotlin.math.roundToInt

/* Yes this is obviously vibe-coded but don't tell anyone that */

@Composable
fun DDKnob(
    modifier: Modifier = Modifier,
    value: Float,
    onValueChange: (Float) -> Unit,
    onValueChangeFinished: () -> Unit,
    @DrawableRes knobImageResId: Int,
    frameCount: Int,
    dragSensitivity: Float = 200f
) {
    val imageBitmap = ImageBitmap.imageResource(id = knobImageResId)
    val frameHeight = imageBitmap.height / frameCount
    val frameWidth = imageBitmap.width

    val frameIndex = (value * (frameCount - 1))
        .roundToInt()
        .coerceIn(0, frameCount - 1)

    val srcTop = frameIndex * frameHeight
    val srcRect = IntRect(0, srcTop, frameWidth, srcTop + frameHeight)

    val density = LocalDensity.current
    val lastValue = remember { mutableFloatStateOf(value) }
    LaunchedEffect(value) { lastValue.floatValue = value }

    Canvas(
        modifier = modifier
            .aspectRatio(frameWidth.toFloat() / frameHeight)
            .pointerInput(Unit) {
                var dragStartValue = 0f
                var totalDragY = 0f

                detectDragGestures(
                    onDragStart = {
                        dragStartValue = lastValue.floatValue
                        totalDragY = 0f
                    },
                    onDrag = { change, dragAmount ->
                        change.consume()
                        totalDragY += dragAmount.y

                        val dragRangePx = with(density) { dragSensitivity.dp.toPx() }
                        val deltaValue = -totalDragY / dragRangePx

                        val newValue = (dragStartValue + deltaValue).coerceIn(0f, 1f)

                        lastValue.floatValue = newValue
                        onValueChange(newValue)
                    },
                    onDragEnd = { onValueChangeFinished() }
                )
            }
    ) {
        drawImage(
            image = imageBitmap,
            srcOffset = IntOffset(srcRect.left, srcRect.top),
            srcSize = IntSize(frameWidth, frameHeight),
            dstOffset = IntOffset.Zero,
            dstSize = IntSize(size.width.roundToInt(), size.height.roundToInt())
        )
    }
}
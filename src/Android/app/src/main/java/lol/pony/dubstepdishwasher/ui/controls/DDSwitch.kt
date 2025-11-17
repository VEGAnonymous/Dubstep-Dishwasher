package lol.pony.dubstepdishwasher.ui.controls

import androidx.annotation.DrawableRes
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.res.imageResource
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.IntRect
import androidx.compose.ui.unit.IntSize

@Composable
fun DDSwitch(
    checked: Boolean,
    onCheckedChange: (Boolean) -> Unit,
    @DrawableRes imageRes: Int,
    modifier: Modifier = Modifier
) {
    val bitmap = ImageBitmap.imageResource(imageRes)
    val frameCount = 2
    val frameHeight = bitmap.height / frameCount
    val frameWidth = bitmap.width

    val frameIndex = if (checked) 1 else 0
    val srcTop = frameIndex * frameHeight

    val srcRect = IntRect(
        left = 0,
        top = srcTop,
        right = frameWidth,
        bottom = srcTop + frameHeight
    )

    Canvas(
        modifier = modifier
            .aspectRatio(frameWidth.toFloat() / frameHeight)
            .pointerInput(Unit) { detectTapGestures(onTap = { onCheckedChange(!checked) }) }
    ) {
        val dst = IntRect(IntOffset.Zero, IntSize(size.width.toInt(), size.height.toInt()))

        drawImage(
            image = bitmap,
            srcOffset = IntOffset(0, srcRect.top),
            srcSize = IntSize(srcRect.width, srcRect.height),
            dstOffset = IntOffset(0, 0),
            dstSize = IntSize(dst.width, dst.height)
        )
    }
}
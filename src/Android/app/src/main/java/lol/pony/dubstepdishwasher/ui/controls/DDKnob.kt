package lol.pony.dubstepdishwasher.ui.controls

import android.content.ClipDescription
import androidx.annotation.DrawableRes
import androidx.compose.animation.core.FastOutSlowInEasing
import androidx.compose.animation.core.Spring
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.spring
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.draganddrop.dragAndDropTarget
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Close
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draganddrop.DragAndDropEvent
import androidx.compose.ui.draganddrop.DragAndDropTarget
import androidx.compose.ui.draganddrop.mimeTypes
import androidx.compose.ui.draganddrop.toAndroidDragEvent
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.scale
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.res.imageResource
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.IntRect
import androidx.compose.ui.unit.IntSize
import androidx.compose.ui.unit.dp
import lol.pony.dubstepdishwasher.model.core.ModAssignment
import lol.pony.dubstepdishwasher.model.core.ModPolarity
import lol.pony.dubstepdishwasher.R
import kotlin.math.roundToInt

private enum class DragMode { Vertical, Horizontal }

@Composable
fun DDKnob(
    modifier: Modifier = Modifier,
    value: Float,
    onValueChange: (Float) -> Unit,
    onValueChangeFinished: () -> Unit,

    // Image strip
    @DrawableRes knobImageResId: Int,
    frameCount: Int,

    // Model stuff
    effectID : Int,
    paramID : Int,
    modAssignments: List<ModAssignment> = emptyList(),
    selectedModID: String? = null,
    isModulatable: Boolean = false,
    currentModOffset: Float = 0f,

    // Callbacks
    onAssignModulator: (String) -> Unit,
    onRemoveModulator: (String) -> Unit,
    onModAmountChange: (String, Float) -> Unit,
    onTogglePolarity: (String) -> Unit
) {
    // Canvas drawing vars
    val imageBitmap = ImageBitmap.imageResource(id = knobImageResId)
    val frameHeight = imageBitmap.height / frameCount
    val frameWidth = imageBitmap.width
    val frameIndex = (value * (frameCount - 1)).roundToInt().coerceIn(0, frameCount - 1)
    val srcTop = frameIndex * frameHeight
    val srcRect = IntRect(0, srcTop, frameWidth, srcTop + frameHeight)
    val density = LocalDensity.current

    // State vars
    val lastValue = remember { mutableFloatStateOf(value) }
    LaunchedEffect(value) { lastValue.floatValue = value }
    val currentModAssignments by rememberUpdatedState(modAssignments)

    // Drag/drop state vars and animations
    var isDropTarget by remember { mutableStateOf(false) }
    var isHovered by remember { mutableStateOf(false) }
    val scale by animateFloatAsState(
        targetValue = if (isHovered) 1.08f else 1f,
        animationSpec = spring(
            dampingRatio = Spring.DampingRatioMediumBouncy,
            stiffness = Spring.StiffnessLow
        ),
        label = "knob_hover_scale"
    )
    val overlayAlpha by animateFloatAsState(
        targetValue = if (isDropTarget) 1f else 0f,
        animationSpec = tween(
            durationMillis = 200,
            easing = FastOutSlowInEasing
        ),
        label = "overlay_fade"
    )

    /* MAIN COMPOSE */
    Box(modifier = modifier
        .aspectRatio(frameWidth.toFloat() / frameHeight)
        .scale(scale)
    ) {
        Canvas(
            modifier = Modifier.fillMaxSize()

                // Knob dragging controls
                .pointerInput(selectedModID, effectID, paramID) {
                    var dragStartValue = 0f
                    var dragStartMod = 0f
                    var totalDragX = 0f; var totalDragY = 0f

                    // Direction lock
                    var dragMode: DragMode? = null
                    val lockThreshold = with(density) { 8.dp.toPx() }

                    detectDragGestures(
                        onDragStart = {
                            dragStartValue = lastValue.floatValue

                            val activeAssignment = currentModAssignments.firstOrNull {
                                it.modId == selectedModID && it.target.effectId == effectID && it.target.paramId == paramID
                            }
                            dragStartMod = activeAssignment?.amount ?: 0f

                            totalDragX = 0f; totalDragY = 0f
                            dragMode = null
                        },
                        onDrag = { change, dragAmount ->
                            change.consume()

                            totalDragX += dragAmount.x; totalDragY += dragAmount.y
                            val absX = kotlin.math.abs(totalDragX); val absY = kotlin.math.abs(totalDragY)

                            // Determine direction
                            if (dragMode == null) {
                                dragMode = if (absY > lockThreshold && absY > absX) DragMode.Vertical
                                else if (absX > lockThreshold && absX > absY) DragMode.Horizontal
                                else return@detectDragGestures
                            }

                            val verticalRangePx = with(density) { 200.dp.toPx() }
                            val horizontalRangePx = with(density) { 100.dp.toPx() }
                            when (dragMode) {
                                // Vertical drag -> knob value
                                DragMode.Vertical -> {
                                    val deltaValue = -(totalDragY / verticalRangePx)
                                    val newValue = (dragStartValue + deltaValue).coerceIn(0f, 1f)
                                    lastValue.floatValue = newValue
                                    onValueChange(newValue)
                                }
                                // Horizontal drag -> modulation amount
                                DragMode.Horizontal -> {
                                    val deltaAmount = totalDragX / horizontalRangePx
                                    val activeAssignment = currentModAssignments.firstOrNull {
                                        it.modId == selectedModID && it.target.effectId == effectID && it.target.paramId == paramID
                                    }
                                    activeAssignment?.let { assignment ->
                                        val newAmount = (dragStartMod + deltaAmount).coerceIn(-1f, 1f)
                                        onModAmountChange(assignment.modId, newAmount)
                                    }
                                }
                                null -> Unit
                            }
                        },
                        onDragEnd = { onValueChangeFinished() }
                    )
                }

                // Drop modulator to assign it
                .dragAndDropTarget(
                    shouldStartDragAndDrop = { event ->
                        isModulatable && event.mimeTypes().contains(ClipDescription.MIMETYPE_TEXT_PLAIN)
                    },
                    target = remember(modAssignments, selectedModID) {
                        object : DragAndDropTarget {
                            override fun onStarted(event: DragAndDropEvent) {
                                val assigned = modAssignments.any { it.modId == selectedModID && it.target.effectId == effectID && it.target.paramId == paramID }
                                if (isModulatable && !assigned) isDropTarget = true
                            }
                            override fun onEntered(event: DragAndDropEvent) {
                                val assigned = modAssignments.any { it.modId == selectedModID && it.target.effectId == effectID && it.target.paramId == paramID }
                                if (isModulatable && !assigned) { isHovered = true }
                            }
                            override fun onExited(event: DragAndDropEvent) { isHovered = false }
                            override fun onDrop(event: DragAndDropEvent): Boolean { // Modulator dropped on this knob
                                isDropTarget = false; isHovered = false
                                val clipData = event.toAndroidDragEvent().clipData
                                if (clipData.itemCount > 0) {
                                    // Validated, assign modulator
                                    val modulatorId = clipData.getItemAt(0).text.toString()
                                    onAssignModulator(modulatorId)
                                    return true
                                } else return false
                            }
                            override fun onEnded(event: DragAndDropEvent) { isDropTarget = false; isHovered = false }
                        }
                    }
                )
        ) {
            drawImage( // Draw the knob on the canvas
                image = imageBitmap,
                srcOffset = IntOffset(srcRect.left, srcRect.top),
                srcSize = IntSize(frameWidth, frameHeight),
                dstOffset = IntOffset.Zero,
                dstSize = IntSize(size.width.roundToInt(), size.height.roundToInt())
            )

            // Draw modulation arc
            // P.S. You are not expected to know how this works
            if (modAssignments.isNotEmpty()) {
                val sweepStart = 225f; val sweepEnd = -45f
                val sweepRange = sweepStart - sweepEnd

                val insetPx = 6.dp.toPx()
                val arcSize = Size(width = size.width - insetPx * 2, height = size.height - insetPx * 2)

                modAssignments.filter { it.modId == selectedModID }.forEach { assignment ->
                    val amount = assignment.amount
                    if (amount == 0f) return@forEach
                    val absAmount = kotlin.math.abs(amount)

                    // Compute normalized positions
                    val (tStart, tEnd) =
                        when (assignment.polarity) {
                            ModPolarity.Bipolar -> {
                                val half = amount / 2f
                                if (amount > 0) ((value - half).coerceIn(0f, 1f)) to ((value + half).coerceIn(0f, 1f))
                                else ((value - half).coerceIn(0f, 1f)) to ((value + half).coerceIn(0f, 1f))
                            }
                            ModPolarity.Unipolar -> {
                                if (amount > 0) value to (value + absAmount).coerceIn(0f, 1f)
                                else (value - absAmount).coerceIn(0f, 1f) to value
                                }
                            }

                    // Convert to angles inside the sweep
                    val startAngle = sweepStart + (tStart * sweepRange) - 90f
                    val endAngle = sweepStart + (tEnd * sweepRange) - 90f
                    val sweepAngle = endAngle - startAngle

                    drawArc(
                        color = Color(0xFF1B97F6),
                        startAngle = startAngle,
                        sweepAngle = sweepAngle,
                        useCenter = false,
                        topLeft = Offset(insetPx, insetPx),
                        size = arcSize,
                        style = Stroke(
                            width = 4.dp.toPx(),
                            cap = StrokeCap.Round
                        )
                    )
                } // forEach
            } // Modulation arc

            // Draw real-time modulation indicator
            if (currentModOffset != 0f && modAssignments.isNotEmpty()) {
                val sweepStart = 225f; val sweepEnd = -45f
                val sweepRange = sweepStart - sweepEnd

                val insetPx = 6.dp.toPx()
                val arcSize = Size(width = size.width - insetPx * 2, height = size.height - insetPx * 2)

                val indicatorSpan = 10f
                val modulatedValue = (value + currentModOffset).coerceIn(0f, 1f)
                val centerAngle = sweepStart + (modulatedValue * sweepRange) - 90f
                val startAngle = centerAngle - (indicatorSpan / 2f)

                drawArc(
                    color = Color(0xFFB8EBFF),
                    startAngle = startAngle,
                    sweepAngle = indicatorSpan,
                    useCenter = false,
                    topLeft = Offset(insetPx, insetPx),
                    size = arcSize,
                    style = Stroke(
                        width = 4.dp.toPx(),
                        cap = StrokeCap.Round
                    )
                )
            } // RT mod indicator
        } // Canvas

        // Highlight targetable knobs for drag & drop
        if (isModulatable) {
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .alpha(overlayAlpha)
                    .border(3.dp, Color(0xFF38D9FF), CircleShape)
                    .background(Color(0x330074CC), CircleShape)
            )
        }

        modAssignments.filter { it.modId == selectedModID }.forEach { assignment ->
            // Switch for toggling polarity
            DDSwitch(
                modifier = Modifier
                    .size(20.dp)
                    .padding(end = 10.dp, bottom = 12.dp)
                    .align(Alignment.TopStart),
                checked = (assignment.polarity.ordinal == 1),
                onCheckedChange = { onTogglePolarity(assignment.modId) },
                imageRes = R.drawable.control_mod_polarity
            )

            // X button to remove modulation assignment
            IconButton(
                onClick = { onRemoveModulator(assignment.modId) },
                modifier = Modifier
                    .size(20.dp)
                    .padding(start = 10.dp, bottom = 12.dp)
                    .align(Alignment.TopEnd)
            ) { Icon(imageVector = Icons.Filled.Close, contentDescription = "Remove Mod") }
        }

    } // Box
} // DDKnob
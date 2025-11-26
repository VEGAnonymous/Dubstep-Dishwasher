package lol.pony.dubstepdishwasher.ui.components.subcomponents

import android.annotation.SuppressLint
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Done
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import lol.pony.dubstepdishwasher.R
import lol.pony.dubstepdishwasher.model.core.CurvePoint
import lol.pony.dubstepdishwasher.model.core.CurvePreset
import lol.pony.dubstepdishwasher.model.core.CurveRandomArgs
import lol.pony.dubstepdishwasher.model.core.EditorState
import lol.pony.dubstepdishwasher.model.core.Modulator
import lol.pony.dubstepdishwasher.model.core.applyCurve
import lol.pony.dubstepdishwasher.model.core.lerp
import lol.pony.dubstepdishwasher.model.core.snapValue
import lol.pony.dubstepdishwasher.ui.components.PresetManager
import lol.pony.dubstepdishwasher.ui.controls.DDSwitch
import kotlin.math.abs

@SuppressLint("MutableCollectionMutableState")
@Composable
fun CurveEditor(
    modulator: Modulator,
    currentPosition: Float?,
    curvePresets: List<CurvePreset>,
    editorState: EditorState,
    onStateChange: (EditorState) -> Unit,
    onSavePreset: (String, String?, List<CurvePoint>) -> CurvePreset,
    onDeletePreset: (String) -> Unit,
    onFavoritePreset: (String, Boolean) -> Unit,
    onDismiss: () -> Unit,
    onSave: (List<CurvePoint>) -> Unit
) {

    /* State vars */
    // Curve edited points
    var editedPoints by remember { mutableStateOf(modulator.curve) }

    // Match endpoints y
    var lockEndpoints by remember { mutableStateOf(editorState.lockEndpoints) }
    LaunchedEffect(lockEndpoints) {
        if (lockEndpoints) {
            editedPoints = editedPoints.toMutableList().apply {
                this[0] = this.first().copy()
                this[lastIndex] = this.last().copy(y = this[0].y)
            }
        }
    }
    // Grid + snapping
    var snapToGrid by remember { mutableStateOf(editorState.snapToGrid) }
    var gridX by remember { mutableIntStateOf(editorState.gridX) }
    var gridY by remember { mutableIntStateOf(editorState.gridY) }

    /* MAIN COMPOSE */
    Box( // Outside container
        modifier = Modifier
            .fillMaxSize()
            .background(Color(0xAA000000))
            .pointerInput(Unit) { detectTapGestures {
                onStateChange(editorState)
                onDismiss() }
            } // Tap outside window to dismiss
    ) {
        Box( // Window container
            modifier = Modifier
                .align(Alignment.Center)
                .fillMaxWidth(0.8f)
                .fillMaxHeight(0.8f)
                .background(Color(0xFFF5F5F5), shape = RoundedCornerShape(16.dp))
                .pointerInput(Unit) { detectTapGestures { } }
                .padding(24.dp)
        ) {
            // Window content
            Column(modifier = Modifier.fillMaxSize()) {
                Box(
                    modifier = Modifier
                        .weight(1f)
                        .fillMaxWidth()
                        .background(Color.White, shape = RoundedCornerShape(8.dp))
                        .padding(16.dp)
                ) {
                    CurveDisplay(
                        points = editedPoints,
                        modifier = Modifier
                            .fillMaxSize()
                            /* DOUBLE TAP */
                            .pointerInput(Unit) {
                                detectTapGestures(
                                    onDoubleTap = { offset ->
                                        // Tap location
                                        val tapX = (offset.x / size.width).coerceIn(0f, 1f)
                                        val tapY = 1f - (offset.y / size.height).coerceIn(0f, 1f)

                                        // Are we tapping near a point?
                                        val pointThreshold = 0.10f
                                        val nearPointIndex = editedPoints.indexOfFirst { point ->
                                            abs(point.x - tapX) < pointThreshold && abs(point.y - tapY) < pointThreshold
                                        }

                                        // What about a curve handle?
                                        val handleThreshold = 0.10f
                                        for (i in 0 until editedPoints.lastIndex) {
                                            val p0 = editedPoints[i]; val p1 = editedPoints[i + 1]
                                            val t = 0.5f
                                            val curved = applyCurve(t, p0)

                                            val handleX = lerp(p0.x, p1.x, t)
                                            val handleY = lerp(p0.y, p1.y, curved)

                                            if (abs(handleX - tapX) < handleThreshold &&
                                                abs(handleY - tapY) < handleThreshold) {
                                                editedPoints = editedPoints.toMutableList().apply {
                                                    this[i] = p0.copy(curve = 0f) // Reset to linear
                                                }
                                                return@detectTapGestures
                                            }
                                        }

                                        // If tapped a point, remove if not an endpoint
                                        if (nearPointIndex >= 0) {
                                            if (nearPointIndex != 0 && nearPointIndex != editedPoints.lastIndex) {
                                                editedPoints = editedPoints.filterIndexed { idx, _ -> idx != nearPointIndex }
                                            }
                                        } else {
                                            // Otherwise create a new point here
                                            if (tapX > 0.05f && tapX < 0.95f) { // Leave room
                                                val newPoint = CurvePoint(tapX, tapY, curve = 0f)
                                                editedPoints = (editedPoints + newPoint).sortedBy { it.x }.toMutableList()
                                            }
                                        }
                                    }
                                )
                            }
                            .pointerInput(Unit) {
                                /* DRAGGING */
                                var draggedPointIndex: Int? = null
                                var draggedCurveSegment: Int? = null
                                var lastPos: Offset? = null

                                detectDragGestures(
                                    onDragStart = { offset ->
                                        lastPos = offset

                                        // Tap coordinates
                                        val tapX = offset.x / size.width; val tapY = 1f - (offset.y / size.height)
                                        // Tolerance for dragging
                                        val pointThreshold = 0.11f; val handleThreshold = 0.13f

                                        // First check if dragging a point
                                        draggedPointIndex = editedPoints.indexOfFirst { point ->
                                            abs(point.x - tapX) < pointThreshold && abs(point.y - tapY) < pointThreshold
                                        }.takeIf { it >= 0 }

                                        // If not, is it a curve handle?
                                        if (draggedPointIndex == null) {
                                            for (i in 0 until editedPoints.lastIndex) {
                                                // Define segment region
                                                val p0 = editedPoints[i]; val p1 = editedPoints[i + 1]

                                                // Calculate handle position at midpoint
                                                val t = 0.5f
                                                val curved = applyCurve(t, p0)
                                                val handleX = lerp(p0.x, p1.x, t)
                                                val handleY = lerp(p0.y, p1.y, curved)

                                                // If dragging within the handle region, set it as currently being dragged
                                                if (abs(handleX - tapX) < handleThreshold && abs(handleY - tapY) < handleThreshold) {
                                                    draggedCurveSegment = i; break
                                                }
                                            }
                                        }
                                    },
                                    onDrag = { change, dragAmount ->
                                        /* DRAG POINT */
                                        val prev = lastPos ?: change.position
                                        val dx = (change.position.x - prev.x) / size.width; val dy = (change.position.y - prev.y) / size.height
                                        lastPos = change.position

                                        draggedPointIndex?.let { index ->
                                            val point = editedPoints[index]

                                            val newX = if (index == 0 || index == editedPoints.lastIndex) point.x // Lock endpoints x
                                            else (point.x + dx).coerceIn(0.01f, 0.99f)

                                            val newY = if (lockEndpoints && (index == 0 || index == editedPoints.lastIndex)) {// Enforce endpoints same y-value
                                                val endpointY = (point.y - dy).coerceIn(0f, 1f)
                                                editedPoints = editedPoints.toMutableList().apply {
                                                    this[0] = this.first().copy(y = endpointY)
                                                    this[lastIndex] = this.last().copy(y = endpointY)
                                                }
                                                endpointY
                                            } else (point.y - dy).coerceIn(0f, 1f) // Drag as usual

                                            editedPoints = editedPoints.toMutableList().apply {
                                                this[index] = point.copy(x = newX, y = newY)
                                            }
                                        }

                                        /* DRAG CURVE HANDLE */
                                        draggedCurveSegment?.let { segmentIndex ->
                                            // Define segment region
                                            val p0 = editedPoints[segmentIndex]; val p1 = editedPoints[segmentIndex + 1]

                                            // Recalculate handle position with current curve value
                                            val t = 0.5f
                                            val curved = applyCurve(t, p0)
                                            val currentHandleY = lerp(p0.y, p1.y, curved)

                                            // Calculate desired new handle position
                                            val dragY = 1f - (change.position.y / size.height)
                                            val targetHandleY = dragY

                                            // Calculate what curve value would place handle at target position
                                            val delta = targetHandleY - currentHandleY
                                            val curveDelta = delta * (if (p1.y > p0.y) -0.8f else 0.8f)

                                            val newCurve = (p0.curve + curveDelta).coerceIn(-1f, 1f)
                                            editedPoints = editedPoints.toMutableList().apply {
                                                this[segmentIndex] = p0.copy(curve = newCurve)
                                            }
                                        }
                                    },
                                    onDragEnd = {
                                        draggedPointIndex?.let { index ->
                                            /* SNAP TO GRID */
                                            val point = editedPoints[index]
                                            if (lockEndpoints && (index == 0 || index == editedPoints.lastIndex)) { // Snap matched endpoints
                                                val snappedY = snapValue(snapToGrid, value = point.y, divisions = gridY)
                                                editedPoints = editedPoints.toMutableList().apply {
                                                    this[0] = this.first().copy(y = snappedY)
                                                    this[lastIndex] = this.last().copy(y = snappedY)
                                                }
                                            } else { // Snap normally
                                                editedPoints = editedPoints.toMutableList().apply {
                                                    this[index] = point.copy(
                                                        x = snapValue(snapToGrid, value = point.x, divisions = gridX),
                                                        y = snapValue(snapToGrid, value = point.y, divisions = gridY)
                                                    )
                                                }
                                            }
                                        }
                                        draggedPointIndex = null; draggedCurveSegment = null; lastPos = null
                                        editedPoints = editedPoints.sortedBy { it.x }.toMutableList()
                                    },
                                    onDragCancel = {
                                        draggedPointIndex = null; draggedCurveSegment = null; lastPos = null
                                        editedPoints = editedPoints.sortedBy { it.x }.toMutableList()
                                    }
                                )
                            },
                        currentPosition = currentPosition,
                        showPoints = true,
                        fillGradient = false,
                        lineColor = Color(0xFF00AAC7),
                        pointColor = Color(0xFF666666),
                        curveHandleColor = Color(0x88666666),
                        positionColor = Color(0xFF02344F),
                        backgroundColor = Color.White,
                        gridX = gridX,
                        gridY = gridY
                    )
                }

                /* Instructions */
                Text(
                    text = "Double-tap to add/remove point • Drag points or curve handles to adjust",
                    style = MaterialTheme.typography.labelSmall,
                    color = Color(0xFF666666),
                    modifier = Modifier.padding(top = 8.dp)
                )

                /* Controls */
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    /* Left side */
                    Row(verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.Start
                    ) {
                        // Grid controls
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            // Snap to Grid
                            DDSwitch(
                                checked = snapToGrid,
                                onCheckedChange = { snapToGrid = !snapToGrid; onStateChange(editorState.copy(snapToGrid = snapToGrid)) },
                                modifier = Modifier.size(20.dp),
                                imageRes = R.drawable.control_toggle
                            )
                            Text(
                                text = "Snap",
                                style = MaterialTheme.typography.labelSmall,
                                color = Color(0xFF666666),
                                modifier = Modifier.padding(end = 8.dp)
                            )

                            // Grid divisions
                            // TEMP: Kind of goofy but whatever
                            Row(verticalAlignment = Alignment.CenterVertically) {
                                Row(verticalAlignment = Alignment.CenterVertically) {
                                    IconButton(
                                        onClick = {
                                            val x = (gridX - 1).coerceIn(1, 32)
                                            gridX = x; onStateChange(editorState.copy(gridX = x))
                                        },
                                        modifier = Modifier.width(20.dp).padding(horizontal = 2.dp)
                                    ) { Text(text = "–", style = MaterialTheme.typography.labelSmall) }
                                    Text(
                                        text = gridX.toString(),
                                        style = MaterialTheme.typography.headlineSmall,
                                        textAlign = TextAlign.Center
                                    )
                                    IconButton(
                                        onClick = {
                                            val x = (gridX + 1).coerceIn(1, 32)
                                            gridX = x; onStateChange(editorState.copy(gridX = x))
                                        },
                                        modifier = Modifier.width(20.dp).padding(horizontal = 2.dp)
                                    ) { Text(text = "+", style = MaterialTheme.typography.labelSmall) }
                                }
                                Text(text = "×", style = MaterialTheme.typography.labelSmall, modifier = Modifier.padding(horizontal = 2.dp))
                                Row(verticalAlignment = Alignment.CenterVertically) {
                                    IconButton(
                                        onClick = {
                                            val y = (gridY - 1).coerceIn(1, 32)
                                            gridY = y; onStateChange(editorState.copy(gridY = y))
                                        },
                                        modifier = Modifier.width(20.dp).padding(horizontal = 2.dp)
                                    ) { Text(text = "–", style = MaterialTheme.typography.labelSmall) }
                                    Text(
                                        text = gridY.toString(),
                                        style = MaterialTheme.typography.headlineSmall,
                                        textAlign = TextAlign.Center
                                    )
                                    IconButton(
                                        onClick = {
                                            val y = (gridY + 1).coerceIn(1, 32)
                                            gridY = y; onStateChange(editorState.copy(gridX = y))
                                        },
                                        modifier = Modifier.width(20.dp).padding(horizontal = 2.dp)
                                    ) { Text(text = "+", style = MaterialTheme.typography.labelSmall) }
                                }
                            }
                        }

                        Spacer(Modifier.width(16.dp))

                        // Match Endpoints
                        if (modulator is Modulator.LFO) {
                            Row(verticalAlignment = Alignment.CenterVertically) {
                                DDSwitch(
                                    checked = lockEndpoints,
                                    onCheckedChange = {
                                        lockEndpoints = !lockEndpoints
                                        onStateChange(editorState.copy(lockEndpoints = lockEndpoints))
                                    },
                                    modifier = Modifier.size(20.dp),
                                    imageRes = R.drawable.control_toggle
                                )
                                Text(
                                    text = "Match Endpoints",
                                    style = MaterialTheme.typography.labelSmall,
                                    color = Color(0xFF666666)
                                )
                            }
                        }

                        Spacer(Modifier.width(16.dp))

                        // Save/load curve presets
                        Row(verticalAlignment = Alignment.CenterVertically
                        ) {
                            PresetManager(
                                presets = curvePresets,
                                currentData = editedPoints,
                                containerState = editorState,
                                randomArgs = CurveRandomArgs(
                                    snapToGrid = editorState.snapToGrid,
                                    gridX = editorState.gridX,
                                    gridY = editorState.gridY,
                                    lockEndpoints = editorState.lockEndpoints
                                ),
                                onStateChange = { presetContainer -> onStateChange(presetContainer) },
                                onSave = { name, category -> onSavePreset(name, category, editedPoints) },
                                onLoad = { preset -> editedPoints = preset.data.map { it.copy() } },
                                onDelete = { name -> onDeletePreset(name) },
                                onFavorite = { name, favorite -> onFavoritePreset(name, favorite) },
                                // Interface copy workaround: we know it's literally a data class
                                copyContainer = { state, preset -> state.copy(currentPreset = preset as CurvePreset) }
                            )
                        }
                    }
                    /* Right side */
                    // Save
                    Row(verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.End
                    ) {
                        IconButton(onClick = { onSave(editedPoints) }) {
                            Icon(
                                imageVector = Icons.Default.Done,
                                contentDescription = "Save",
                                tint = Color(0xFF000000)
                            )
                        }
                    }
                } // Controls
            } // Window content
        } // Window box
    } // Outside box
} // CurveEditor
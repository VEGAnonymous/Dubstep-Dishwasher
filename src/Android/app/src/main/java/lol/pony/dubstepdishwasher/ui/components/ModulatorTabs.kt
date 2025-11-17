package lol.pony.dubstepdishwasher.ui.components

import android.content.ClipData
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.draganddrop.dragAndDropSource
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draganddrop.DragAndDropTransferData
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import lol.pony.dubstepdishwasher.model.core.ModAssignment
import lol.pony.dubstepdishwasher.model.core.Modulator

@Composable
fun ModulatorTabs(
    modulators: List<Modulator>,
    assignments: List<ModAssignment>,
    selectedMod: Modulator?,
    onSelected: (Modulator) -> Unit
) {
    fun modCount(mod: Modulator): Int = assignments.count { it.modId == mod.id }

    LazyVerticalGrid( // Modulator tabs in a neat grid
        columns = GridCells.Fixed(2),
        modifier = Modifier
            .fillMaxWidth()
            .padding(6.dp)
            .height(85.dp),
        horizontalArrangement = Arrangement.spacedBy(6.dp),
        verticalArrangement = Arrangement.spacedBy(6.dp),
        userScrollEnabled = true
    ) {
        items(modulators) { mod ->
            val isSelected = (mod == selectedMod)

            Row (
                horizontalArrangement = Arrangement.Start,
                verticalAlignment = Alignment.CenterVertically,
                modifier = Modifier
                    .clip(RoundedCornerShape(4.dp))
                    .background(if (isSelected) Color(0x5500CCAA) else Color(0x11000000))
                    .padding(vertical = 4.dp)
                    .clickable { onSelected(mod) }
            ) {
                // Modulator name
                Text(
                    text = mod.id,
                    style = MaterialTheme.typography.bodySmall,
                    textAlign = TextAlign.Center,
                    modifier = Modifier
                        .padding(horizontal = 4.dp)
                        .width(60.dp)
                )

                Spacer(Modifier.width(10.dp))

                // Draggable mod bubble
                Box(contentAlignment = Alignment.Center,
                    modifier = Modifier
                        .size(24.dp)
                        .clip(CircleShape)
                        .background(Color(0xFF444444))
                        .padding(4.dp)
                        .dragAndDropSource (
                            drawDragDecoration = { }
                        ) { transferData ->
                            if (!isSelected) onSelected(mod) // Auto-select dragged modulator
                            DragAndDropTransferData(
                                // Drag plaintext because I couldn't figure out anything better
                                clipData = ClipData.newPlainText("Modulator", mod.id)
                            )
                        }
                ) {
                    Text( // Assignment count for this modulator
                        text = modCount(mod).toString(),
                        color = Color.White,
                        style = MaterialTheme.typography.bodySmall
                    )
                }
            }
        }
    }
}
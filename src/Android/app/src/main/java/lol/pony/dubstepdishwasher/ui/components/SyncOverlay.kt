package lol.pony.dubstepdishwasher.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import lol.pony.dubstepdishwasher.model.core.SyncState

@Composable
fun SyncOverlay(
    syncState: SyncState,
    syncMessage: String,
    onDisconnect: () -> Unit
) {
    if (syncState == SyncState.SYNCED) return

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(Color(0xAA000000))
            .pointerInput(Unit) { detectTapGestures { } }
    ) {
        Box(
            modifier = Modifier
                .align(Alignment.Center)
                .width(240.dp)
                .background(Color(0xFFF5F5F5), shape = RoundedCornerShape(16.dp))
                .pointerInput(Unit) { detectTapGestures { } }
                .padding(24.dp)
        ) {
            Column(
                modifier = Modifier.fillMaxWidth(),
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.spacedBy(16.dp)
            ) {
                CircularProgressIndicator(
                    modifier = Modifier.size(48.dp),
                    color = MaterialTheme.colorScheme.primary
                )

                Text(
                    text = syncMessage,
                    style = MaterialTheme.typography.labelLarge,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis
                )

                if (syncMessage.contains("Awaiting", ignoreCase = true)) { // Allow manual disconnect
                    Button(
                        onClick = onDisconnect,
                        colors = ButtonDefaults.buttonColors(containerColor = MaterialTheme.colorScheme.error)
                    ) { Text("Exit") }
                }
            }
        }
    }
}
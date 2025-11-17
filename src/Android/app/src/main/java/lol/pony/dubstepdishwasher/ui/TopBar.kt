package lol.pony.dubstepdishwasher.ui

import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Close
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.polidea.rxandroidble3.RxBleDevice

@Composable
fun TopBar (
    device: RxBleDevice?,
    onDisconnect: () -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 16.dp, vertical = 4.dp)
            .height(45.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        // Logo stuff
        Text(text = "DUBSTEP DISHWASHER", style = MaterialTheme.typography.headlineLarge)

        Spacer(modifier = Modifier.width(40.dp))

        // TODO: Add resource monitor
        // TODO: Add preset manager
        // TODO: Add mod matrix menu

        // BLE status
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text(
                text = device?.name ?: "Unknown Device",
                style = MaterialTheme.typography.bodyMedium
            )
            IconButton(onClick = onDisconnect) {
                Icon(Icons.Filled.Close, contentDescription = "Disconnect")
            }
        }

    }
}
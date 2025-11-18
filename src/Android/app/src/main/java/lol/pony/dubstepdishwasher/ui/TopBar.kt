package lol.pony.dubstepdishwasher.ui

import android.graphics.ColorSpace
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
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.withStyle
import androidx.compose.ui.unit.dp
import com.polidea.rxandroidble3.RxBleDevice
import lol.pony.dubstepdishwasher.model.core.Effect

@Composable
fun TopBar (
    device: RxBleDevice?,
    onDisconnect: () -> Unit,
    cpuUsage: Float
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

        // Resource Monitor
        val color = Color(0.8f * (cpuUsage / 100), 0.8f * (1 - cpuUsage / 100), 0f)

        Text(
            buildAnnotatedString {
                append("CPU Usage: ")
                withStyle(style = SpanStyle(color = color)) {
                    append("$cpuUsage%")
                }
            }
        )
    }
}
package lol.pony.dubstepdishwasher.ui

import android.os.Build.VERSION.SDK_INT
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import coil.ImageLoader
import coil.compose.rememberAsyncImagePainter
import coil.decode.GifDecoder
import coil.decode.ImageDecoderDecoder
import coil.request.ImageRequest
import coil.size.Size
import com.polidea.rxandroidble3.RxBleDevice
import com.polidea.rxandroidble3.scan.ScanResult
import lol.pony.dubstepdishwasher.model.BLEManager
import lol.pony.dubstepdishwasher.R

@Composable
fun ScanScreen(
    modifier: Modifier = Modifier,
    bleManager: BLEManager,
    requestPermissions: () -> Unit,
) {
    val isScanning by bleManager.isScanning

    Box(
        modifier = modifier.fillMaxSize(),
        contentAlignment = Alignment.Center
    ) {
        // Main content (GIF and button)
        Column(
            modifier = Modifier.fillMaxSize(),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            // GIF
            val context = LocalContext.current
            val imageLoader = ImageLoader.Builder(context)
                .components {
                    if (SDK_INT >= 28) add(ImageDecoderDecoder.Factory())
                    else add(GifDecoder.Factory())
                }.build()
            Image(
                painter = rememberAsyncImagePainter(
                    ImageRequest.Builder(context).data(data = R.drawable.dubstep_dishwasher)
                        .apply(block = { size(Size.ORIGINAL) })
                        .build(), imageLoader = imageLoader),
                contentDescription = null,
                modifier = Modifier.heightIn(max = 350.dp).padding(vertical = 20.dp)
            )

            Button(onClick = {
                if (!bleManager.hasPermissions()) requestPermissions()
                else { bleManager.startBleScan() }
            }) { Text("Scan for Devices", style = MaterialTheme.typography.bodyMedium) }
        }

        if (isScanning) { Box(modifier = Modifier.fillMaxSize().background(Color.White.copy(alpha = 0.5f))) } // White fade overlay
        // Overlay popup for scanning results
        AnimatedVisibility(
            visible = isScanning,
            modifier = Modifier.align(Alignment.Center)
        ) {
            Column(
                modifier = Modifier
                    .width(250.dp)
                    .clip(RoundedCornerShape(16.dp))
                    .background(Color(0xFFF4F4F4))
                    .padding(16.dp)
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.Center,
                    verticalAlignment = Alignment.CenterVertically
                ) { Button(onClick = { bleManager.stopBleScan() }) { Text("Stop Scanning") } }

                Spacer(modifier = Modifier.height(12.dp))

                ScannedDevicesList(
                    devices = bleManager.scannedDevices.value,
                    onConnect = { bleManager.connectToDevice(it) }
                )
            }
        }
    }
}

@Composable
fun ScannedDevicesList(
    devices: List<ScanResult>,
    onConnect: (RxBleDevice) -> Unit
) {
    LazyColumn(modifier = Modifier
        .padding(top = 16.dp)
        .fillMaxWidth(),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        items(devices) { deviceResult ->
            Column(modifier = Modifier
                .padding(vertical = 12.dp)
                .fillMaxWidth(),
                horizontalAlignment = Alignment.CenterHorizontally
            ) {
                Text(
                    text = "Name: ${deviceResult.bleDevice.name ?: "Unknown"}\nAddress: ${deviceResult.bleDevice.macAddress}",
                    style = MaterialTheme.typography.bodySmall)
                Button(
                    onClick = { onConnect(deviceResult.bleDevice) },
                    modifier = Modifier.padding(top = 10.dp, bottom = 4.dp),
                    contentPadding = PaddingValues(horizontal = 20.dp, vertical = 4.dp)
                ) { Text(text = "Connect", style = MaterialTheme.typography.bodySmall) }
            }
            HorizontalDivider(modifier = Modifier.width(200.dp).padding(vertical = 4.dp))
        }
    }
}
package lol.pony.dubstepdishwasher.ui

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.Button
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.polidea.rxandroidble3.RxBleDevice
import com.polidea.rxandroidble3.scan.ScanResult
import lol.pony.dubstepdishwasher.model.BLEManager
import lol.pony.dubstepdishwasher.viewmodel.MainViewModel
import lol.pony.dubstepdishwasher.viewmodel.MainViewModelFactory

@Composable
fun ScanScreen(
    modifier: Modifier = Modifier,
    bleManager: BLEManager,
    requestPermissions: () -> Unit
) {
    val mainViewModel: MainViewModel = viewModel(factory = MainViewModelFactory(bleManager))
    Column(modifier = modifier.padding(16.dp)) {
        Row {
            // Scan buttons
            Button(onClick = { if (bleManager.hasPermissions()) bleManager.startBleScan() else requestPermissions() }) {
                Text(text = "Start Scan", style = MaterialTheme.typography.bodyMedium)
            }
            Spacer(modifier = Modifier.width(8.dp))
            Button(onClick = { bleManager.stopBleScan() }) {
                Text(text = "Stop Scan", style = MaterialTheme.typography.bodyMedium)
            }
        }
        // Displays scanned devices
        ScannedDevicesList(
            devices = bleManager.scannedDevices.value,
            onConnect = {
                bleManager.connectToDevice(it)
                mainViewModel.clearChain()
            })
    }
}

@Composable
fun ScannedDevicesList(
    devices: List<ScanResult>,
    onConnect: (RxBleDevice) -> Unit
) {
    LazyColumn(modifier = Modifier.padding(top = 16.dp)) {
        items(devices) { deviceResult ->
            Column(modifier = Modifier.padding(vertical = 8.dp)) {
                Text(
                    text = "Name: ${deviceResult.bleDevice.name ?: "Unknown"}\nAddress: ${deviceResult.bleDevice.macAddress}",
                    style = MaterialTheme.typography.bodySmall)
                Button(
                    onClick = { onConnect(deviceResult.bleDevice) },
                    modifier = Modifier.padding(top = 10.dp, bottom = 2.dp),
                    contentPadding = PaddingValues(horizontal = 20.dp, vertical = 2.dp)
                ) { Text(text = "Connect", style = MaterialTheme.typography.bodySmall) }
            }
            HorizontalDivider(modifier = Modifier.width(200.dp))
        }
    }
}
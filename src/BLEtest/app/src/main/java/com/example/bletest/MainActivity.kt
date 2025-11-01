package com.example.bletest

import android.bluetooth.BluetoothGattCharacteristic
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.Button
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextField
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.example.bletest.ui.theme.BLETestTheme
import com.polidea.rxandroidble3.RxBleConnection
import com.polidea.rxandroidble3.RxBleDevice
import com.polidea.rxandroidble3.scan.ScanResult
import java.util.UUID

class MainActivity : ComponentActivity() {

    private lateinit var bleManager: BleManager

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // creates instance of BleManager
        bleManager = BleManager(this)

        // request permissions and scans if valid
        val requestPermissionLauncher =
            registerForActivityResult(
                ActivityResultContracts.RequestMultiplePermissions()
            ) { permissions ->
                if (permissions.entries.all { it.value }) {
                    bleManager.startBleScan()
                }
            }

        setContent {
            BLETestTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    BleScannerApp(
                        modifier = Modifier.padding(innerPadding),
                        bleManager = bleManager,
                        requestPermissions = { requestPermissionLauncher.launch(bleManager.requiredPermissions) }
                    )
                }
            }
        }
    }

    override fun onPause() {
        super.onPause()
        // disconnects devices when leaving app
//        bleManager.stopBleScan()
    }
}

/**
 * Determines screen to display based on connection state
 * @param modifier Modifier to apply to the layout
 * @param bleManager instance of BleManager
 * @param requestPermissions function to request permissions
 */
@Composable
fun BleScannerApp(modifier: Modifier = Modifier, bleManager: BleManager, requestPermissions: () -> Unit) {
    // gets connection state and connection device
    val connectedDevice = bleManager.connectedDevice.value
    val connectionState = bleManager.connectionState.value

    // Scan for devices if no device is connected
    if (connectedDevice != null && connectionState == RxBleConnection.RxBleConnectionState.CONNECTED) {
        ConnectedDeviceScreen(
            bleManager = bleManager,
            device = connectedDevice,
            onDisconnect = { bleManager.disconnect() })
    } else {
        ScanScreen(
            modifier = modifier,
            bleManager = bleManager,
            requestPermissions = requestPermissions)
    }
}

/**
 * Displays list of scanned devices.
 * Shows buttons to start/stop scanning.
 * @param modifier Modifier to apply to the layout
 * @param bleManager instance of BleManager
 * @param requestPermissions function to request permissions
 */
@Composable
fun ScanScreen(modifier: Modifier = Modifier, bleManager: BleManager, requestPermissions: () -> Unit) {
    Column(modifier = modifier.padding(16.dp)) {
        Row {
            // Scan buttons
            Button(onClick = {
                if (bleManager.hasPermissions()) {
                    bleManager.startBleScan()
                } else {
                    requestPermissions()
                }
            }) {
                Text("Start Scan")
            }
            Spacer(modifier = Modifier.width(8.dp))
            Button(onClick = { bleManager.stopBleScan() }) {
                Text("Stop Scan")
            }
        }
        // displays scanned devices
        ScannedDevicesList(devices = bleManager.scannedDevices.value, onConnect = { bleManager.connectToDevice(it) })
    }
}

/**
 * Displays list of scanned devices with names.
 * Shows button to connect to a device.
 * @param devices list of scanned devices
 * @param onConnect button to connect to a device
 */
@Composable
fun ScannedDevicesList(devices: List<ScanResult>, onConnect: (RxBleDevice) -> Unit) {
    LazyColumn(modifier = Modifier.padding(top = 16.dp)) {
        items(devices) { deviceResult ->
            Column(modifier = Modifier.padding(vertical = 8.dp)) {
                Text(text = "Name: ${deviceResult.bleDevice.name ?: "Unknown"}\nAddress: ${deviceResult.bleDevice.macAddress}")
                Button(onClick = { onConnect(deviceResult.bleDevice) }) {
                    Text("Connect")
                }
            }
            HorizontalDivider()
        }
    }
}

/**
 * Shows currently connected devices and it's writable characteristics.
 * Shows button to disconnect from a device.
 * @param bleManager instance of BleManager
 * @param device connected device
 * @param onDisconnect button to disconnect from a device
 */
@Composable
fun ConnectedDeviceScreen(bleManager: BleManager, device: RxBleDevice, onDisconnect: () -> Unit) {
    val characteristics = bleManager.characteristics.value
    val characteristicsData = bleManager.characteristicsData.value

    Column(modifier = Modifier.padding(16.dp)) {
        Text("Connected to: ${device.name ?: device.macAddress}")
        Button(onClick = onDisconnect) {
            Text("Disconnect")
        }

        // Displays list of characteristics
        LazyColumn(modifier = Modifier.padding(top = 16.dp)) {
            items(characteristics) { characteristic ->
                CharacteristicItem(
                    characteristic = characteristic,
                    value = characteristicsData[characteristic.uuid],
                    onWrite = { uuid, value -> bleManager.writeCharacteristic(value, uuid) }
                )
                HorizontalDivider()
            }
        }
    }
}

/**
 * Display for each characteristic
 *
 * Shows current value or last sent value.
 * Has text box and a button to write to the characteristic.
 * @param characteristic characteristic to display
 * @param value characteristic's current value or last sent value
 * @param onWrite button to write to the characteristic
 */
@Composable
fun CharacteristicItem(
    characteristic: BluetoothGattCharacteristic,
    value: String?,
    onWrite: (UUID, String) -> Unit,
//    onRead: (UUID) -> Unit
) {
    // value to write to characteristic
    var writeValue by remember { mutableStateOf("") }
    // checks if characteristic is readable
    val isReadable = (characteristic.properties and BluetoothGattCharacteristic.PROPERTY_READ) != 0
    val label = if (isReadable) "Value:" else "Last Sent:"

    Column(modifier = Modifier.padding(vertical = 8.dp)) {
        Text("UUID: ${characteristic.uuid}")
        Text("$label ${value ?: "N/A"}")

        Row(verticalAlignment = Alignment.CenterVertically, modifier = Modifier.padding(top = 8.dp)) {
            TextField(
                value = writeValue,
                onValueChange = { writeValue = it },
                label = { Text("Hex Value") },
                modifier = Modifier.weight(1f)
            )
            Spacer(modifier = Modifier.width(8.dp))
            Button(onClick = {
                onWrite(characteristic.uuid, writeValue)
                writeValue = ""
            }) {
                Text("Write")
            }
        }
    }
}

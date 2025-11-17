package lol.pony.dubstepdishwasher

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.Button
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.polidea.rxandroidble3.RxBleConnection
import com.polidea.rxandroidble3.RxBleDevice
import com.polidea.rxandroidble3.scan.ScanResult
import lol.pony.dubstepdishwasher.model.BLEManager

import lol.pony.dubstepdishwasher.ui.*
import lol.pony.dubstepdishwasher.ui.theme.DubstepDishwasherTheme

/* SET THIS FLAG TO SKIP BLE - FOR DEVELOPMENT ONLY */
const val SKIP_BLE = true

class MainActivity : ComponentActivity() {

    private lateinit var bleManager: BLEManager

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        bleManager = BLEManager(this)

        // Request permissions and scans if valid
        val requestPermissionLauncher =
            registerForActivityResult(
                ActivityResultContracts.RequestMultiplePermissions()
            ) { permissions ->
                if (permissions.entries.all { it.value }) {
                    bleManager.startBleScan()
                }
            }

        enableEdgeToEdge()
        setContent {
            DubstepDishwasherTheme {
                Row(Modifier.fillMaxSize()) {
                    BleScannerApp(
                        modifier = Modifier
                            .fillMaxHeight()
                            .fillMaxWidth()
                            .padding(16.dp),
                        bleManager = bleManager,
                        requestPermissions = { requestPermissionLauncher.launch(bleManager.requiredPermissions) }
                    )
                }
            }
        }
    }
}

/**
 * Determines screen to display based on connection state
 * @param modifier Modifier to apply to the layout
 * @param bleManager instance of BleManager
 * @param requestPermissions function to request permissions
 */
@Composable
fun BleScannerApp(modifier: Modifier = Modifier, bleManager: BLEManager, requestPermissions: () -> Unit) {
    // Gets connection state and connection device
    val connectedDevice = bleManager.connectedDevice.value
    val connectionState = bleManager.connectionState.value

    // Scan for devices if no device is connected
    if ((connectedDevice != null && connectionState == RxBleConnection.RxBleConnectionState.CONNECTED) || SKIP_BLE /* SET FLAG ABOVE TO SKIP BLE */) {
        /* MAIN GUI */
        Column(modifier = Modifier.fillMaxHeight()) {
            TopBar(
                device = if (SKIP_BLE) null else connectedDevice,
                onDisconnect = { bleManager.disconnect() }
            )

            HorizontalDivider()

            MainPanel(bleManager = bleManager)
        }
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
fun ScanScreen(modifier: Modifier = Modifier, bleManager: BLEManager, requestPermissions: () -> Unit) {
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
        // Displays scanned devices
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
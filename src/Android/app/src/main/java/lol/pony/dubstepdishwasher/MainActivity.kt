package lol.pony.dubstepdishwasher

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.*
import androidx.compose.material3.HorizontalDivider
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.polidea.rxandroidble3.RxBleConnection
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
            registerForActivityResult(ActivityResultContracts.RequestMultiplePermissions())
            { permissions -> if (permissions.entries.all { it.value }) bleManager.startBleScan() }

        enableEdgeToEdge()
        setContent {
            DubstepDishwasherTheme {
                Row(Modifier.fillMaxSize()) {
                    App(
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

@Composable
fun App(
    modifier: Modifier = Modifier,
    bleManager: BLEManager,
    requestPermissions: () -> Unit
) {
    // Get connection state and connection device
    val connectedDevice = bleManager.connectedDevice.value
    val connectionState = bleManager.connectionState.value

    // Decide which screen to display
    // Scan for devices if no device is connected
    if ((connectedDevice != null && connectionState == RxBleConnection.RxBleConnectionState.CONNECTED) || SKIP_BLE /* SET FLAG ABOVE TO SKIP BLE */) {
        /* MAIN GUI */
        Column(modifier = Modifier.fillMaxHeight()) {
            TopBar(
                bleManager = bleManager,
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
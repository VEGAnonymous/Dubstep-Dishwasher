package lol.pony.dubstepdishwasher

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.HorizontalDivider
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.polidea.rxandroidble3.RxBleConnection
import lol.pony.dubstepdishwasher.model.BLEManager
import lol.pony.dubstepdishwasher.model.core.UserPresets
import lol.pony.dubstepdishwasher.ui.MainPanel
import lol.pony.dubstepdishwasher.ui.ScanScreen
import lol.pony.dubstepdishwasher.ui.TopBar
import lol.pony.dubstepdishwasher.ui.theme.DubstepDishwasherTheme
import lol.pony.dubstepdishwasher.viewmodel.MainViewModel
import lol.pony.dubstepdishwasher.viewmodel.MainViewModelFactory

/* SET THIS FLAG TO SKIP BLE - FOR DEVELOPMENT ONLY */
const val SKIP_BLE = false

class MainActivity : ComponentActivity() {

    private lateinit var bleManager: BLEManager
    private lateinit var userPresets: UserPresets


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        bleManager = BLEManager(this)
        userPresets = UserPresets(this)


        // Request permissions and scans if valid
        val requestPermissionLauncher =
            registerForActivityResult(ActivityResultContracts.RequestMultiplePermissions())
            { permissions -> if (permissions.entries.all { it.value }) bleManager.startBleScan() }

        enableEdgeToEdge()
        setContent {
            val mainViewModel: MainViewModel = viewModel(
                factory = MainViewModelFactory(bleManager, userPresets)
            )
            DubstepDishwasherTheme {
                Row(Modifier.fillMaxSize()) {
                    App(
                        modifier = Modifier
                            .fillMaxHeight()
                            .fillMaxWidth()
                            .padding(16.dp),
                        bleManager = bleManager,
                        requestPermissions = { requestPermissionLauncher.launch(bleManager.requiredPermissions) },
                        mainViewModel = mainViewModel
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
    requestPermissions: () -> Unit,
    mainViewModel: MainViewModel
) {
    // Get connection state and connection device
    val connectedDevice = bleManager.connectedDevice.value
    val connectionState = bleManager.connectionState.value

    // Decide which screen to display
    // Scan for devices if no device is connected
    @Suppress("SimplifyBooleanWithConstants", "KotlinConstantConditions", "RedundantSuppression")
    if ((connectedDevice != null && connectionState == RxBleConnection.RxBleConnectionState.CONNECTED) || SKIP_BLE /* SET FLAG ABOVE TO SKIP BLE */) {
        /* MAIN GUI */
        Column(modifier = Modifier.fillMaxHeight()) {
            TopBar(
                viewModel = mainViewModel,
                device = if (SKIP_BLE) null else connectedDevice,
                onDisconnect = { bleManager.disconnect() }
            )
            HorizontalDivider()
            MainPanel(mainViewModel)
        }
    } else {
        ScanScreen(
            modifier = modifier,
            bleManager = bleManager,
            requestPermissions = requestPermissions)
    }
}
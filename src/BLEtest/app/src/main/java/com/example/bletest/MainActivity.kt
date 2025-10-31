package com.example.bletest

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import com.example.bletest.ui.theme.BLETestTheme
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothManager
import android.content.Intent

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        // bluetooth adapter setup/checking
        val bluetoothManager: BluetoothManager = getSystemService(BluetoothManager::class.java)
        val bluetoothAdapter: BluetoothAdapter? = bluetoothManager.getAdapter()

        // display
        setContent {
            BLETestTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    Greeting(
                        bluetoothAdapter = bluetoothAdapter,
                        modifier = Modifier.padding(innerPadding)
                    )
                }
            }
        }
    }
}

@Composable
fun Greeting(bluetoothAdapter: BluetoothAdapter?, modifier: Modifier = Modifier) {
    if (bluetoothAdapter == null) {
        Text(
            text = "BLE does not work",
            modifier = modifier
        )
    } else {
        Text(
            text = "BLE does work",
            modifier = modifier
        )
    }
    if (bluetoothAdapter?.isEnabled == false) {
        val enableBtIntent = Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)
        dispatchResult(enableBtIntent, REQUEST_ENABLE_BT)
    }
}

//@Preview(showBackground = true)
//@Composable
//fun GreetingPreview() {
//    BLETestTheme {
//        Greeting("Android")
//    }
//}
package lol.pony.dubstepdishwasher.model

import android.Manifest
import android.bluetooth.BluetoothGattCharacteristic
import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import androidx.compose.runtime.mutableStateOf
import androidx.core.app.ActivityCompat
import com.polidea.rxandroidble3.RxBleClient
import com.polidea.rxandroidble3.RxBleConnection
import com.polidea.rxandroidble3.RxBleDevice
import com.polidea.rxandroidble3.scan.ScanResult
import com.polidea.rxandroidble3.scan.ScanSettings
import io.reactivex.rxjava3.core.Observable
import io.reactivex.rxjava3.disposables.CompositeDisposable

class BLEManager(private val context: Context) {
    private val rxBleClient: RxBleClient = RxBleClient.create(context)
    // composite disposable for managing multiple disposables
    private val compositeDisposable = CompositeDisposable()
    private var connection: RxBleConnection? = null

    val scannedDevices = mutableStateOf(listOf<ScanResult>())
    val connectedDevice = mutableStateOf<RxBleDevice?>(null)
    val connectionState = mutableStateOf<RxBleConnection.RxBleConnectionState?>(null)
    // val characteristics = mutableStateOf<List<BluetoothGattCharacteristic>>(emptyList())
    // val characteristicsData = mutableStateOf<Map<UUID, String>>(emptyMap())
    val characteristic = mutableStateOf<BluetoothGattCharacteristic?>(null)
    val characteristicData = mutableStateOf<String?>(null)


    val requiredPermissions = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
        arrayOf(
            Manifest.permission.BLUETOOTH_SCAN,
            Manifest.permission.BLUETOOTH_CONNECT,
            Manifest.permission.ACCESS_FINE_LOCATION
        )
    } else {
        arrayOf(
            Manifest.permission.ACCESS_FINE_LOCATION
        )
    }

    /**
     * Returns true if all required BLE and location permissions are granted.
     * The list of required permissions depends on the Android version.
     */
    fun hasPermissions(): Boolean {
        return requiredPermissions.all {
            ActivityCompat.checkSelfPermission(context, it) == PackageManager.PERMISSION_GRANTED
        }
    }

    private fun scan(): Observable<ScanResult> = rxBleClient.scanBleDevices(
        ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()
    )

    /**
     * Scans for BLE devices and adds them to [scannedDevices].
     * Clears previous results and disposable before starting a new scan.
     */
    fun startBleScan() {
        // clearing previous results/disposables
        compositeDisposable.clear()
        scannedDevices.value = emptyList()
        scan()
            // filter out nameless devices
            .filter { it.bleDevice.name != null }
            .subscribe({ scanResult ->
                // add new devices to list
                val newDevices = scannedDevices.value.toMutableList()
                if (newDevices.find { it.bleDevice.macAddress == scanResult.bleDevice.macAddress } == null) {
                    newDevices.add(scanResult)
                    scannedDevices.value = newDevices
                }
            }, { throwable ->
                throwable.printStackTrace()
            }).let { compositeDisposable.add(it) }
    }

    /**
     * Stops scanning for BLE devices.
     */
    fun stopBleScan() {
        compositeDisposable.clear()
    }

    /**
     * Connects to specified [RxBleDevice]
     * and updates connection status.
     * Also scans for characteristics.
     * @param device The BLE device to connect to.
     */
    fun connectToDevice(device: RxBleDevice) {
        connectedDevice.value = device
        device.establishConnection(false)
            .subscribe({ rxBleConnection ->
                connection = rxBleConnection
                connectionState.value = RxBleConnection.RxBleConnectionState.CONNECTED
                // scan for characteristics
                discoverCharacteristics()
            }, {
                connectionState.value = RxBleConnection.RxBleConnectionState.DISCONNECTED
            }).let { compositeDisposable.add(it) }
    }

    /**
     * Searches for the first writable characteristic
     * of the [connectedDevice].
     */
    fun discoverCharacteristics() {
        connection?.discoverServices()
            ?.subscribe({ services ->
                // gets the first writable characteristic
                val writableCharacteristic = services.bluetoothGattServices
                    .flatMap { it.characteristics }
                    .firstOrNull { (it.properties and (BluetoothGattCharacteristic.PROPERTY_WRITE or BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE)) != 0 }
                characteristic.value = writableCharacteristic
                // reads initial value
                writableCharacteristic?.let {
                    if ((it.properties and BluetoothGattCharacteristic.PROPERTY_READ) != 0) {
                        readCharacteristic(it)
                    }
                }
            }, { throwable ->
                throwable.printStackTrace()
            })
            ?.let { compositeDisposable.add(it) }
    }

    /**
     * Reads the value of a characteristic.
     * Updates the [characteristicData] with the new value.
     *
     * If [expectedValue] is provided, the value is verified against it.
     *
     * @param characteristic The characteristic to read.
     * @param expectedValue The expected value of the characteristic.
     */
    fun readCharacteristic(characteristic: BluetoothGattCharacteristic, expectedValue: ByteArray? = null) {
        connection?.readCharacteristic(characteristic.uuid)
            ?.subscribe({ value ->
                val hexString = bytesToHexString(value)
                // verifying stored value
                if (expectedValue != null) {
                    if (value.contentEquals(expectedValue)) {
                        characteristicData.value = hexString
                    } else {
                        characteristicData.value = "$hexString (Invalid)"
                    }
                } else {
                    // stores value
                    characteristicData.value = hexString
                }
            }, { throwable ->
                // update with read error message
                characteristicData.value = "Error: ${throwable.message}"
            })
            ?.let { compositeDisposable.add(it) }
    }

    /**
     * Writes a value to the characteristic.
     *
     * @param value The values to write converted to a byte array.
     */
    fun writeCharacteristic(value: String) {
        characteristic.value?.let { char ->
            try {
                // converts string to identical byte array
                val bytesToWrite = hexStringToByteArray(value)
                connection?.writeCharacteristic(char.uuid, bytesToWrite)
                    ?.subscribe({
                        // reads value and verifies it if readable characteristic
                        val isReadable = char.properties.and(BluetoothGattCharacteristic.PROPERTY_READ) != 0
                        if (isReadable) {
                            readCharacteristic(char, bytesToWrite)
                        } else {
                            // updates value manually if unreadable
                            characteristicData.value = bytesToHexString(bytesToWrite)
                        }
                    }, { throwable ->
                        // updates with write error message
                        characteristicData.value = "Write Error: ${throwable.message}"
                    })
                    ?.let { compositeDisposable.add(it) }
            } catch (e: IllegalArgumentException) {
                // gives error message if string is invalid
                characteristicData.value = "Invalid Hex String"
            }
        }
    }


    /**
     * Disconnects from the current device and clears all disposables.
     */
    fun disconnect() {
        compositeDisposable.clear()
        connectedDevice.value = null
        connectionState.value = null
        // characteristics.value = emptyList()
        // characteristicsData.value = emptyMap()
        characteristic.value = null
        characteristicData.value = null
    }

    /**
     * Converts string to byte array with direct mapping
     * to the hex values in the string.
     * Also checks for valid hex bytes
     * @param hex The string to convert to byte array.
     * @return Byte array representation of [hex].
     *
     * Example:
     * ```kotlin
     * val result = hexStringToByteArray("1234 567 8")
     * result -> [0x12, 0x34, 0x56, 0x78]
     * ```
     */
    private fun hexStringToByteArray(hex: String): ByteArray {
        val cleanHex = hex.replace(" ", "")
        require(cleanHex.length % 2 == 0) { "Hex string must have an even length" }
        return cleanHex.chunked(2)
            // only maps 0x0-0xF
            .map { it.toInt(16).toByte() }
            .toByteArray()
    }

    /**
     * Converts byte array to string with space separator.
     * @param bytes The byte array to convert to string.
     * @return String representation of [bytes].
     */
    private fun bytesToHexString(bytes: ByteArray): String {
        return bytes.joinToString(" ") { "%02X".format(it) }
    }
}
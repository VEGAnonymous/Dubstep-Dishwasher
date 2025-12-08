package lol.pony.dubstepdishwasher.model

import android.Manifest
import android.bluetooth.BluetoothGattCharacteristic
import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import android.util.Log
import androidx.compose.runtime.mutableStateOf
import androidx.core.app.ActivityCompat
import com.polidea.rxandroidble3.RxBleClient
import com.polidea.rxandroidble3.RxBleConnection
import com.polidea.rxandroidble3.RxBleDevice
import com.polidea.rxandroidble3.scan.ScanResult
import com.polidea.rxandroidble3.scan.ScanSettings
import io.reactivex.rxjava3.core.Observable
import io.reactivex.rxjava3.disposables.CompositeDisposable
import lol.pony.dubstepdishwasher.model.core.Status
import java.util.UUID

class BLEManager(private val context: Context) {
    private val rxBleClient: RxBleClient = RxBleClient.create(context)
    // Composite disposable for managing multiple disposables
    private val compositeDisposable = CompositeDisposable()
    private var connection: RxBleConnection? = null

    // Scanning
    val scannedDevices = mutableStateOf(listOf<ScanResult>())
    val connectedDevice = mutableStateOf<RxBleDevice?>(null)
    val connectionState = mutableStateOf<RxBleConnection.RxBleConnectionState?>(null)
    val characteristic = mutableStateOf<BluetoothGattCharacteristic?>(null)
    val characteristicData = mutableStateOf<String?>(null)
    private var isReadable = false
    val isScanning = mutableStateOf(false)

    // Status
    var onStatusReceived: ((Status) -> Unit)? = null
    private val notifyCharacteristicUUID = UUID.fromString("beb5483f-36e1-4688-b7f5-ea07361b26a8")

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
        // Clear previous results/disposables
        compositeDisposable.clear()
        scannedDevices.value = emptyList()

        isScanning.value = true
        scan()
            // Filter out nameless devices
            .filter { it.bleDevice.name != null }
            .subscribe({ scanResult ->
                // Add new devices to list
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
        isScanning.value = false
    }

    fun setupNotifications() {
        connection?.setupNotification(notifyCharacteristicUUID)
            ?.flatMap { it }
            ?.subscribe({ bytes ->
                // Parse status packet
                val status = Status.fromBytes(bytes)
                if (status != null) onStatusReceived?.invoke(status)
            }, { throwable ->
                Log.e("BLE", "Notification error: ${throwable.message}")
            })
            ?.let { compositeDisposable.add(it) }
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
            .flatMapSingle { rxBleConnection ->
                rxBleConnection.requestMtu(512).map { rxBleConnection }
            }
            .subscribe({ rxBleConnection ->
                connection = rxBleConnection
                connectionState.value = RxBleConnection.RxBleConnectionState.CONNECTED
                // Discover characteristics
                discoverCharacteristics()
                // Setup notifications for status updates
                setupNotifications()
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
                // Gets the first writable characteristic
                val writableCharacteristic = services.bluetoothGattServices
                    .flatMap { it.characteristics }
                    .firstOrNull { (it.properties and (BluetoothGattCharacteristic.PROPERTY_WRITE or BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE)) != 0 }
                characteristic.value = writableCharacteristic
                // Reads initial value
                characteristic.value?.let {
                    if ((it.properties and BluetoothGattCharacteristic.PROPERTY_READ) != 0) {
                        isReadable = true
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
                // Verifying stored value
                if (expectedValue != null) {
                    if (value.contentEquals(expectedValue)) {
                        characteristicData.value = hexString
                    } else {
                        characteristicData.value = "$hexString (Invalid)"
                    }
                } else {
                    // Store value
                    characteristicData.value = hexString
                }
            }, { throwable ->
                // Update with read error message
                characteristicData.value = "Error: ${throwable.message}"
            })
            ?.let { compositeDisposable.add(it) }
    }

    /**
     * Writes a value to the characteristic.
     */
    fun writeCharacteristic(bytesToWrite: ByteArray) {
        characteristic.value?.let { char ->
            try {
                connection?.writeCharacteristic(char.uuid, bytesToWrite)
                    ?.subscribe({
                        // Read value and verifies it if readable characteristic
                        if (isReadable) {
                            readCharacteristic(char, bytesToWrite)
                        } else {
                            // Update value manually if unreadable
                            characteristicData.value = bytesToHexString(bytesToWrite)
                        }
                    }, { throwable ->
                        // Update with write error message
                        characteristicData.value = "Write Error: ${throwable.message}"
                    })
                    ?.let { compositeDisposable.add(it) }
            } catch (e: IllegalArgumentException) {
                // Throw if string is invalid
                characteristicData.value = "$e: Invalid Hex String:"
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
     * Converts byte array to string with space separator.
     * @param bytes The byte array to convert to string.
     * @return String representation of [bytes].
     */
    private fun bytesToHexString(bytes: ByteArray): String {
        return bytes.joinToString(" ") { "%02X".format(it) }
    }
}
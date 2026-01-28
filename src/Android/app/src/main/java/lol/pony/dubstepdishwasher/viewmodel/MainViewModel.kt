package lol.pony.dubstepdishwasher.viewmodel

import android.util.Log
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch
import lol.pony.dubstepdishwasher.model.*
import lol.pony.dubstepdishwasher.model.core.*
import java.util.concurrent.ConcurrentHashMap

internal const val LOG_DEBUG = true

@Suppress("PropertyName")
class MainViewModel(
    internal val bleManager: BLEManager,
    internal val userPresets: UserPresets
    ) : ViewModel() {

    init {
        viewModelScope.launch {
            userPresets.userPresetsFlow.collect { saved ->
                _globalPresets.value = saved.globalPresets
                _curvePresets.value = saved.curvePresets
            }
        }

        bleManager.onStatusReceived = { status -> handleStatus(status) }
    }

    /* DATA STRUCTURES */

    // System
    internal val _resourceUsage = MutableStateFlow(ResourceUsage(0f, 0))
    val resourceUsage = _resourceUsage

    internal val _resourceError = MutableStateFlow<String?>(null)
    val resourceError: StateFlow<String?> = _resourceError
    fun clearResourceError() { _resourceError.value = null }

    internal fun calculateTotalUsage(): ResourceUsage {
        val mainUsage = chain.totalUsage() // Main chain
        // Sum all parallel chain usages
        val parallelUsage = _parallelChains.value.values.fold(ResourceUsage(0f, 0)) { acc, state ->
            ResourceUsage(
                compute = acc.compute + state.chainAUsage.compute + state.chainBUsage.compute,
                memory = acc.memory + state.chainAUsage.memory + state.chainBUsage.memory
            )
        }

        return ResourceUsage( // TOTAL
            compute = mainUsage.compute + parallelUsage.compute,
            memory = mainUsage.memory + parallelUsage.memory
        )
    }

    // Effects
    internal val chain = EffectChain()
    internal val _effects = MutableStateFlow<List<Effect>>(emptyList())
    val effects: StateFlow<List<Effect>> = _effects

    // Modulation
    internal val _modulators = MutableStateFlow(
        listOf(
            // In-house LFO / random
            Modulator.LFO(id = "LFO1"),
            Modulator.LFO(id = "LFO2"),
            Modulator.LFO(id = "LFO3"),
            Modulator.LFO(id = "LFO4"),
            Modulator.LFO(id = "LFO5"),
            Modulator.LFO(id = "LFO6"),
            // ML quality regression
            Modulator.Mapping(id = "Bright"),
            Modulator.Mapping(id = "Warmth"),
            Modulator.Mapping(id = "Intensity"),
            Modulator.Mapping(id = "Perc"),
            Modulator.Mapping(id = "Speed"),
            // Expression pedal
            Modulator.Mapping(id = "Expr"),
        )
    )
    val modulators: StateFlow<List<Modulator>> = _modulators

    internal val _modAssignments = MutableStateFlow<List<ModAssignment>>(emptyList())
    val modAssignments: StateFlow<List<ModAssignment>> = _modAssignments

    internal val _currentModValues = MutableStateFlow<Map<String, Float>>(emptyMap())
    val currentModValues: StateFlow<Map<String, Float>> = _currentModValues

    internal val _currentModOffsets = MutableStateFlow<Map<ParamKey, Float>>(emptyMap())
    val currentModOffsets: StateFlow<Map<ParamKey, Float>> = _currentModOffsets

    internal val _editorStates = MutableStateFlow<Map<String, EditorState>>(emptyMap())
    val editorStates: StateFlow<Map<String, EditorState>> = _editorStates

    // Parallel
    internal val _parallelChains = MutableStateFlow<Map<Int, ParallelChainState>>(emptyMap())
    val parallelChains: StateFlow<Map<Int, ParallelChainState>> = _parallelChains

    // Presets
    internal val _globalPresets = MutableStateFlow(defaultGlobalPresets())
    val globalPresets: StateFlow<List<GlobalPreset>> = _globalPresets

    internal val _curvePresets = MutableStateFlow(defaultCurvePresets())
    val curvePresets: StateFlow<List<CurvePreset>> = _curvePresets

    /* GLOBAL STATE / PRESETS */

    internal val _syncState = MutableStateFlow(SyncState.SYNCED)
    val syncState: StateFlow<SyncState> = _syncState

    /* CONTROL */

    // Clock all updates to control rate
    internal val controlQueue = ControlQueue(
        scope = viewModelScope,
        rate = CONTROL_RATE,
        onFlush = { commands -> sendCommands(commands) },
        onUpdate = { },
        onRetry = { command -> handleRetry(command) }
    )

    @Suppress("unused")
    internal val lfoControl = ControlQueue(
        scope = viewModelScope,
        rate = LFO_UPDATE_RATE,
        onFlush = { },
        onUpdate = { controlUpdate() },
        onRetry = { }
    )

    internal fun controlUpdate() {
        /* Apply modulation */
        val dt = 1f / LFO_UPDATE_RATE
        val offsets = ModRouter.computeModulations( // Get all mod offsets
            modulators = _modulators.value,
            assignments = _modAssignments.value,
            dt = dt
        )

        _currentModOffsets.value = offsets // Expose offsets
        // And track individual modulator output
        val modValues = _modulators.value.associate { mod ->
            mod.id to when (mod) {
                is Modulator.LFO -> ModEngine.lfoValue(mod, dt)
                is Modulator.Mapping -> ModEngine.mappingValue(mod)
            }
        }
        _currentModValues.value = modValues
    }

    /* BLE */

    internal var heartbeatTime = 0L
    internal var commandSequence: Byte = 0
    internal val pendingCommands = ConcurrentHashMap<Byte, PendingCommand>()

    // Monitor connection
    init {
        viewModelScope.launch {
            while (bleManager.connectedDevice.value != null) {
                delay(HEARTBEAT_TIMEOUT)
                if (System.currentTimeMillis() - heartbeatTime > HEARTBEAT_TIMEOUT) {
                    if (_syncState.value == SyncState.SYNCED) resync()
                }
            }
        }
    }

    fun disconnect() { // Manual disconnect
        Log.d("SYNC", "Manual disconnect")
        bleManager.disconnect()
        _syncState.value = SyncState.SYNCED
        pendingCommands.clear()
    }
} // MainViewModel
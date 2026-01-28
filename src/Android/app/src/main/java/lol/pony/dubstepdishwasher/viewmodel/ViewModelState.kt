@file:Suppress("UnusedReceiverParameter")

package lol.pony.dubstepdishwasher.viewmodel

import android.util.Log
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import lol.pony.dubstepdishwasher.model.core.*
import lol.pony.dubstepdishwasher.model.core.CommandType.*
import kotlin.collections.component1
import kotlin.collections.component2
import kotlin.collections.forEach
import kotlin.collections.set

@Suppress("ObjectPropertyName")
private val _syncMessage = MutableStateFlow("Syncing...")
private var loadJob: Job? = null

val MainViewModel.syncMessage: StateFlow<String> get() = _syncMessage
private var MainViewModel.loadJobInternal: Job?
    get() = loadJob
    set(value) { loadJob = value }

val MainViewModel.currentGlobalState: StateFlow<GlobalPresetData>
    get() = combine(
        _effects,
        _modulators,
        _modAssignments,
        _editorStates,
        _parallelChains
    ) { effects, modulators, assignments, editorStates, parallelChains ->
        GlobalPresetData(
            effects = effects.map {
                EffectSnapshot(
                    effectType = it.effectType,
                    parameters = it.parameters,
                    isBypassed = it.isBypassed
                )
            },
            modulators = modulators.map {
                ModulatorSnapshot(
                    id = it.id,
                    isLFO = it is Modulator.LFO,
                    parameters = it.parameters,
                    curve = it.curve.map { c -> c.copy() }
                )
            },
            assignments = assignments.map { it.copy() },
            editorStates = editorStates.mapValues { it.value.copy() },
            parallelChains = snapshotParallelChains(parallelChains)
        )
    }.stateIn(
        viewModelScope,
        SharingStarted.Eagerly,
        GlobalPresetData( // Init
            effects = emptyList(),
            modulators = emptyList(),
            assignments = emptyList(),
            editorStates = emptyMap(),
            parallelChains = emptyMap()
        )
    )

fun MainViewModel.saveGlobalPreset(name: String, category: String?) : GlobalPreset {
    val data = GlobalPresetData(
        effects = snapshotEffects(),
        modulators = snapshotModulators(),
        assignments = _modAssignments.value.map { it.copy() },
        editorStates = _editorStates.value.mapValues { it.value.copy() },
        parallelChains = snapshotParallelChains(_parallelChains.value)
    )
    val preset = GlobalPreset(name, data, category, false)
    _globalPresets.update { presets -> presets.filterNot { it.name == name } + preset }

    updateDataStore()
    return preset
}

suspend fun MainViewModel.loadGlobalPreset(data: GlobalPresetData) {
    clearChain()
    controlQueue.flushNow()
    awaitPending()

    rebuildEffects(data.effects)
    rebuildParallelChains(data.parallelChains)
    rebuildModulators(data.modulators)
    _modAssignments.value = data.assignments.map { it.copy() }
    _editorStates.value = data.editorStates.mapValues { it.value.copy() }

    syncModulation()
    controlQueue.flushNow()
    awaitPending()

    if (LOG_DEBUG) Log.d("SYNC", "Preset load complete")
}

fun MainViewModel.loadGlobalPreset(name: String) {
    if (_syncState.value == SyncState.AWAIT) return
    loadJobInternal?.cancel()

    loadJobInternal = viewModelScope.launch {
        _syncMessage.value = "Loading: $name"
        _syncState.value = SyncState.RESYNC // Trigger overlay

        val preset = _globalPresets.value.find { it.name == name } ?: return@launch
        try {
            loadGlobalPreset(preset.data)
            _syncState.value = SyncState.SYNCED
        } catch (e: CancellationException) {
            pendingCommands.clear()
            throw e // Rethrow to cancel coroutine
        } catch (e: ConnectionLostException) {
            // Connection lost during load - trigger resync
            Log.e("SYNC", "Connection lost during preset load, attempting resync")
            pendingCommands.clear()
            resync()
        }
    }
}

fun MainViewModel.deleteGlobalPreset(name: String) {
    _globalPresets.update { it.filterNot { preset -> preset.name == name } }
    updateDataStore()
}

fun MainViewModel.favoriteGlobalPreset(name: String, favorite: Boolean) {
    _globalPresets.update { list -> list.map { preset -> if (preset.name == name) preset.copy(favorite = favorite) else preset } }
    updateDataStore()
}

fun MainViewModel.updateDataStore() {
    viewModelScope.launch {
        userPresets.savePresets(_globalPresets.value, _curvePresets.value)
    }
}

internal fun MainViewModel.snapshotEffects(): List<EffectSnapshot> {
    return _effects.value.map { effect ->
        EffectSnapshot(
            effectType = effect.effectType,
            parameters = effect.parameters,
            isBypassed = effect.isBypassed
        )
    }
}

internal fun MainViewModel.snapshotModulators(): List<ModulatorSnapshot> {
    return _modulators.value.map { mod ->
        ModulatorSnapshot(
            id = mod.id,
            isLFO = mod is Modulator.LFO,
            parameters = mod.parameters,
            curve = mod.curve.map { it.copy() }
        )
    }
}

internal fun MainViewModel.snapshotParallelChains(chains: Map<Int, ParallelChainState>): Map<Int, ParallelChainSnapshot> {
    return chains.mapValues { (_, state) ->
        ParallelChainSnapshot(
            chainA = state.chainAEffects.map { effect ->
                EffectSnapshot(
                    effectType = effect.effectType,
                    parameters = effect.parameters,
                    isBypassed = effect.isBypassed
                )
            },
            chainB = state.chainBEffects.map { effect ->
                EffectSnapshot(
                    effectType = effect.effectType,
                    parameters = effect.parameters,
                    isBypassed = effect.isBypassed
                )
            }
        )
    }
}

internal suspend fun MainViewModel.rebuildEffects(list: List<EffectSnapshot>) {
    list.forEach { snap ->
        addEffect(snap.effectType)
        controlQueue.flushNow()
        awaitPending()

        val newEffect = _effects.value.lastOrNull() ?: return@forEach
        snap.parameters.forEachIndexed { paramId, param ->
            setParam(newEffect.effectId, paramId, param.getValueAny())
            controlQueue.flushNow()
        }
        if (snap.isBypassed) {
            toggleBypass(newEffect.effectId)
            controlQueue.flushNow()
            awaitPending()
        }
    }
    if (LOG_DEBUG) Log.d("SYNC", "REBUILD EFFECTS")
}

internal fun MainViewModel.rebuildModulators(list: List<ModulatorSnapshot>) {
    _modulators.value = list.map { snap ->
        val mod = if (snap.isLFO) Modulator.LFO(snap.id, curve = snap.curve.map { it.copy() })
        else Modulator.Mapping(snap.id, curve = snap.curve.map { it.copy() })

        snap.parameters.forEachIndexed { paramId, param ->
            mod.setParam(paramId, param.getValueAny())
            controlQueue.flushNow()
        }
        mod
    }
    if (LOG_DEBUG) Log.d("SYNC", "REBUILD MODULATORS")
}

internal fun MainViewModel.syncModulation() { // Sync modulation with downstream
    // Send modulator parameters and curves
    _modulators.value.forEachIndexed { index, mod ->
        // Parameters
        mod.parameters.forEachIndexed { paramId, param ->
            when (val value = param.getValueAny()) {
                is Float -> controlQueue.enqueue(MOD_SET_PARAMETER, index, paramId, value)
                is LFOMode -> controlQueue.enqueue(MOD_SET_PARAMETER, index, paramId, value.ordinal.toFloat())
                is RandomMode -> controlQueue.enqueue(MOD_SET_PARAMETER, index, paramId, value.ordinal.toFloat())
            }
        }
        // Curves
        controlQueue.enqueue(MOD_CLEAR_CURVE, index, 0, 0f)
        mod.curve.forEachIndexed { pointIndex, point ->
            controlQueue.enqueue(MOD_SET_CURVE_POINT, index, pointIndex, point.x, point.y, point.curve)
        }
    }

    // Send assignments
    _modAssignments.value.forEach { assignment ->
        val modIndex = modulatorIdToIndex(assignment.modId)
        controlQueue.enqueue(MOD_ASSIGNMENT_ADD,
            modIndex,
            assignment.target.effectId,
            assignment.target.paramId.toFloat(),
            assignment.amount,
            assignment.polarity.ordinal.toFloat()
        )
    }
    if (LOG_DEBUG) Log.d("SYNC", "SYNC MODULATION")
}

internal suspend fun MainViewModel.rebuildParallelChains(snapshots: Map<Int, ParallelChainSnapshot>) {
    snapshots.forEach { (parallelId, snapshot) ->
        // Rebuild chain A
        snapshot.chainA.forEach { effectSnap ->
            parallelAddEffect(parallelId, ParallelChain.A, effectSnap.effectType)
            controlQueue.flushNow()
            awaitPending()

            val effects = _parallelChains.value[parallelId]?.chainAEffects ?: return@forEach
            val newEffect = effects.lastOrNull() ?: return@forEach

            effectSnap.parameters.forEachIndexed { paramId, param ->
                parallelSetParam(parallelId, ParallelChain.A, newEffect.effectId, paramId, param.getValueAny())
                controlQueue.flushNow()
            }

            if (effectSnap.isBypassed) {
                parallelBypassEffect(parallelId, ParallelChain.A, newEffect.effectId)
                controlQueue.flushNow()
                awaitPending()
            }
        }

        // Rebuild chain B
        snapshot.chainB.forEach { effectSnap ->
            parallelAddEffect(parallelId, ParallelChain.B, effectSnap.effectType)
            controlQueue.flushNow()
            awaitPending()

            val effects = _parallelChains.value[parallelId]?.chainBEffects ?: return@forEach
            val newEffect = effects.lastOrNull() ?: return@forEach

            effectSnap.parameters.forEachIndexed { paramId, param ->
                parallelSetParam(parallelId, ParallelChain.B, newEffect.effectId, paramId, param.getValueAny())
                controlQueue.flushNow()
            }

            if (effectSnap.isBypassed) {
                parallelBypassEffect(parallelId, ParallelChain.B, newEffect.effectId)
                controlQueue.flushNow()
                awaitPending()
            }
        }
    }
    if (LOG_DEBUG) Log.d("SYNC", "REBUILD PARALLEL")
}

internal suspend fun MainViewModel.awaitPending() { // Wait for pending commands to be ACKed
    var retryCount = 0
    while (pendingCommands.isNotEmpty() && retryCount < MAX_AWAIT_RETRIES) {
        delay(AWAIT_TIMEOUT)
        retryCount++
        if (LOG_DEBUG && retryCount % 4 == 0) Log.d("SYNC", "Awaiting ${pendingCommands.size} pending commands")
    }
    if (pendingCommands.isNotEmpty()) {
        if (LOG_DEBUG) Log.e("SYNC", "Timeout on pending commands")
        throw ConnectionLostException("Timeout on pending commands")
    }
}

internal suspend fun MainViewModel.awaitConnection() {
    _syncState.value = SyncState.AWAIT
    Log.d("SYNC", "Awaiting connection")
    var retryCount = 0
    while (System.currentTimeMillis() - heartbeatTime > HEARTBEAT_TIMEOUT) { // Only if heartbeat timed out
        if (retryCount >= MAX_AWAIT_RETRIES) {
            Log.e("SYNC", "Heartbeat timed out")
            _syncMessage.value = "Awaiting connection..."
            delay(5000)
            retryCount = 0
            continue
        }
        _syncMessage.value = "Reconnecting... (${retryCount + 1}/$MAX_AWAIT_RETRIES)"
        delay(500)
        retryCount++
    }
}

internal fun MainViewModel.resync() {
    viewModelScope.launch {
        // Snapshot current app state
        val snapshot = GlobalPresetData(
            effects = snapshotEffects(),
            modulators = snapshotModulators(),
            assignments = _modAssignments.value.map { it.copy() },
            editorStates = _editorStates.value.mapValues { it.value.copy() },
            parallelChains = snapshotParallelChains(_parallelChains.value)
        )

        if (LOG_DEBUG) Log.d("SYNC", "Starting sync")
        awaitConnection()

        _syncState.value = SyncState.RESYNC
        while (true) {
            try {
                _syncMessage.value = "Syncing..."
                loadGlobalPreset(snapshot) // Attempt to load snapshot (internal call to prevent recursion)

                if (LOG_DEBUG) Log.d("SYNC", "Resync complete")
                _syncState.value = SyncState.SYNCED
                return@launch
            } catch (e: ConnectionLostException) {
                Log.e("SYNC", "Resync failed; retrying")
                delay(1000)
            }
        }
    }
}

/* OTHER PRESETS */

fun MainViewModel.saveCurvePreset(name: String, category: String?, points: List<CurvePoint>) : CurvePreset {
    val preset = CurvePreset(name, data = points, category, false)
    _curvePresets.update { presets -> presets.filterNot { it.name == name } + preset } // Add new preset or overwrite if same name
    updateDataStore()
    return preset
}

fun MainViewModel.deleteCurvePreset(name: String) {
    _curvePresets.update { it.filterNot { p -> p.name == name } }
    updateDataStore()
}

fun MainViewModel.favoriteCurvePreset(name: String, favorite: Boolean) {
    _curvePresets.update { list -> list.map { preset -> if (preset.name == name) preset.copy(favorite = favorite) else preset } }
    updateDataStore()
}

/* UI */

fun MainViewModel.updateEditorState(modId: String, state: EditorState) {
    _editorStates.value = _editorStates.value.toMutableMap().apply { this[modId] = state }
}
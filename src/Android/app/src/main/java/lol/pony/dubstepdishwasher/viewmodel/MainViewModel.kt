package lol.pony.dubstepdishwasher.viewmodel

// import android.util.Log
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import lol.pony.dubstepdishwasher.model.BLEManager
import lol.pony.dubstepdishwasher.model.ControlQueue
import lol.pony.dubstepdishwasher.model.EffectChain
import lol.pony.dubstepdishwasher.model.core.BiquadType
import lol.pony.dubstepdishwasher.model.core.CONTROL_RATE
import lol.pony.dubstepdishwasher.model.core.Command
import lol.pony.dubstepdishwasher.model.core.CommandType.EFFECT_ADD
import lol.pony.dubstepdishwasher.model.core.CommandType.EFFECT_BYPASS
import lol.pony.dubstepdishwasher.model.core.CommandType.EFFECT_CLEAR
import lol.pony.dubstepdishwasher.model.core.CommandType.EFFECT_REMOVE
import lol.pony.dubstepdishwasher.model.core.CommandType.EFFECT_REORDER
import lol.pony.dubstepdishwasher.model.core.CommandType.EFFECT_SET_PARAMETER
import lol.pony.dubstepdishwasher.model.core.CommandType.MOD_ASSIGNMENT_ADD
import lol.pony.dubstepdishwasher.model.core.CommandType.MOD_ASSIGNMENT_REMOVE
import lol.pony.dubstepdishwasher.model.core.CommandType.MOD_ASSIGNMENT_SET
import lol.pony.dubstepdishwasher.model.core.CommandType.MOD_CLEAR_CURVE
import lol.pony.dubstepdishwasher.model.core.CommandType.MOD_SET_CURVE_POINT
import lol.pony.dubstepdishwasher.model.core.CommandType.MOD_SET_PARAMETER
import lol.pony.dubstepdishwasher.model.core.CommandType.PARALLEL_CHAIN_COMMAND
import lol.pony.dubstepdishwasher.model.core.CurvePoint
import lol.pony.dubstepdishwasher.model.core.CurvePreset
import lol.pony.dubstepdishwasher.model.core.DistortionMode
import lol.pony.dubstepdishwasher.model.core.EditorState
import lol.pony.dubstepdishwasher.model.core.Effect
import lol.pony.dubstepdishwasher.model.core.EffectSnapshot
import lol.pony.dubstepdishwasher.model.core.EffectType
import lol.pony.dubstepdishwasher.model.core.EnvelopeType
import lol.pony.dubstepdishwasher.model.core.FFTSize
import lol.pony.dubstepdishwasher.model.core.GlobalPreset
import lol.pony.dubstepdishwasher.model.core.GlobalPresetData
import lol.pony.dubstepdishwasher.model.core.LFOMode
import lol.pony.dubstepdishwasher.model.core.LFO_UPDATE_RATE
import lol.pony.dubstepdishwasher.model.core.MAX_COMPUTE_USAGE
import lol.pony.dubstepdishwasher.model.core.MAX_MEMORY_USAGE
import lol.pony.dubstepdishwasher.model.core.ModAssignment
import lol.pony.dubstepdishwasher.model.core.ModEngine
import lol.pony.dubstepdishwasher.model.core.ModPolarity
import lol.pony.dubstepdishwasher.model.core.ModRouter
import lol.pony.dubstepdishwasher.model.core.ModulationEffectMode
import lol.pony.dubstepdishwasher.model.core.Modulator
import lol.pony.dubstepdishwasher.model.core.ModulatorSnapshot
import lol.pony.dubstepdishwasher.model.core.ParallelChain
import lol.pony.dubstepdishwasher.model.core.ParallelChainSnapshot
import lol.pony.dubstepdishwasher.model.core.ParallelChainState
import lol.pony.dubstepdishwasher.model.core.ParallelMode
import lol.pony.dubstepdishwasher.model.core.ParamKey
import lol.pony.dubstepdishwasher.model.core.ParamUnit
import lol.pony.dubstepdishwasher.model.core.RandomMode
import lol.pony.dubstepdishwasher.model.core.ResourceUsage
import lol.pony.dubstepdishwasher.model.core.UserPresets
import lol.pony.dubstepdishwasher.model.core.WavetableType
import lol.pony.dubstepdishwasher.model.core.defaultCurvePresets
import lol.pony.dubstepdishwasher.model.core.defaultGlobalPresets
import java.nio.ByteBuffer
import java.nio.ByteOrder

// import kotlin.experimental.xor

class MainViewModel(
    private val bleManager: BLEManager,
    private val userPresets: UserPresets
    ) : ViewModel() {

    /* DATA STRUCTURES */

    // System
    private val _resourceUsage = MutableStateFlow(ResourceUsage(0f, 0))
    val resourceUsage = _resourceUsage

    private val _resourceError = MutableStateFlow<String?>(null)
    val resourceError: StateFlow<String?> = _resourceError
    fun clearResourceError() { _resourceError.value = null }

    private fun calculateTotalUsage(): ResourceUsage {
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
    private val chain = EffectChain()
    private val _effects = MutableStateFlow<List<Effect>>(emptyList())
    val effects: StateFlow<List<Effect>> = _effects

    // Modulation
    private val _modulators = MutableStateFlow(
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

    private val _modAssignments = MutableStateFlow<List<ModAssignment>>(emptyList())
    val modAssignments: StateFlow<List<ModAssignment>> = _modAssignments

    private val _currentModValues = MutableStateFlow<Map<String, Float>>(emptyMap())
    val currentModValues: StateFlow<Map<String, Float>> = _currentModValues

    private val _currentModOffsets = MutableStateFlow<Map<ParamKey, Float>>(emptyMap())
    val currentModOffsets: StateFlow<Map<ParamKey, Float>> = _currentModOffsets

    private val _editorStates = MutableStateFlow<Map<String, EditorState>>(emptyMap())
    val editorStates: StateFlow<Map<String, EditorState>> = _editorStates

    // Parallel

    private val _parallelChains = MutableStateFlow<Map<Int, ParallelChainState>>(emptyMap())
    val parallelChains: StateFlow<Map<Int, ParallelChainState>> = _parallelChains

    // Presets
    private val _globalPresets = MutableStateFlow(defaultGlobalPresets())
    val globalPresets: StateFlow<List<GlobalPreset>> = _globalPresets

    private val _curvePresets = MutableStateFlow(defaultCurvePresets())
    val curvePresets: StateFlow<List<CurvePreset>> = _curvePresets

    /* GLOBAL PRESETS */

    val currentGlobalState: StateFlow<GlobalPresetData> =
        combine(
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
            // Initial value
            GlobalPresetData(
                effects = emptyList(),
                modulators = emptyList(),
                assignments = emptyList(),
                editorStates = emptyMap(),
                parallelChains = emptyMap()
            )
        )

    init {
        viewModelScope.launch {
            userPresets.presetsFlow.collect { saved ->
                _globalPresets.value = saved
            }
        }
    }

    fun saveGlobalPreset(name: String, category: String?) : GlobalPreset {
        val data = GlobalPresetData(
            effects = snapshotEffects(),
            modulators = snapshotModulators(),
            assignments = _modAssignments.value.map { it.copy() },
            editorStates = _editorStates.value.mapValues { it.value.copy() },
            parallelChains = snapshotParallelChains(_parallelChains.value) // NEW
        )
        val preset = GlobalPreset(name, data, category, false)
        _globalPresets.update { presets -> presets.filterNot { it.name == name } + preset }

        updateDataStore()
        return preset
    }

    fun loadGlobalPreset(name: String) {
        val preset = _globalPresets.value.find { it.name == name } ?: return
        val data = preset.data

        clearChain()
        rebuildEffects(data.effects)
        rebuildParallelChains(data.parallelChains)
        rebuildModulators(data.modulators)
        _modAssignments.value = data.assignments.map { it.copy() }
        _editorStates.value = data.editorStates.mapValues { it.value.copy() }

        syncModulation()
    }

    fun deleteGlobalPreset(name: String) {
        _globalPresets.update { it.filterNot { preset -> preset.name == name } }

        updateDataStore()
    }

    fun favoriteGlobalPreset(name: String, favorite: Boolean) {
        _globalPresets.update { list -> list.map { preset -> if (preset.name == name) preset.copy(favorite = favorite) else preset } }

        updateDataStore()
    }

    fun updateDataStore() {
        viewModelScope.launch {
            userPresets.savePresets(_globalPresets.value)
        }
    }

    private fun snapshotEffects(): List<EffectSnapshot> {
        return _effects.value.map { effect ->
            EffectSnapshot(
                effectType = effect.effectType,
                parameters = effect.parameters,
                isBypassed = effect.isBypassed
            )
        }
    }

    private fun snapshotModulators(): List<ModulatorSnapshot> {
        return _modulators.value.map { mod ->
            ModulatorSnapshot(
                id = mod.id,
                isLFO = mod is Modulator.LFO,
                parameters = mod.parameters,
                curve = mod.curve.map { it.copy() }
            )
        }
    }

    private fun snapshotParallelChains(chains: Map<Int, ParallelChainState>): Map<Int, ParallelChainSnapshot> {
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

    private fun rebuildEffects(list: List<EffectSnapshot>) {
        list.forEach { snap ->
            addEffect(snap.effectType)
            val newEffect = _effects.value.last()
            snap.parameters.forEachIndexed { paramId, param ->
                val value = param.getValueAny()
                setParam(newEffect.effectId, paramId, value)
            }
            if (snap.isBypassed) toggleBypass(newEffect.effectId)
        }
    }

    private fun rebuildModulators(list: List<ModulatorSnapshot>) {
        _modulators.value = list.map { snap ->
            val mod = if (snap.isLFO) Modulator.LFO(snap.id, curve = snap.curve.map { it.copy() })
            else Modulator.Mapping(snap.id, curve = snap.curve.map { it.copy() })

            snap.parameters.forEachIndexed { paramId, param ->
                val value = param.getValueAny()
                mod.setParam(paramId, value) }
            mod
        }
    }

    private fun rebuildParallelChains(snapshots: Map<Int, ParallelChainSnapshot>) {
        snapshots.forEach { (parallelId, snapshot) ->
            // Rebuild chain A
            snapshot.chainA.forEach { effectSnap ->
                parallelAddEffect(parallelId, ParallelChain.A, effectSnap.effectType)
                val effects = _parallelChains.value[parallelId]?.chainAEffects ?: return@forEach
                val newEffect = effects.lastOrNull() ?: return@forEach

                effectSnap.parameters.forEachIndexed { paramId, value ->
                    parallelSetParam(parallelId, ParallelChain.A, newEffect.effectId, paramId, value)
                }

                if (effectSnap.isBypassed) {
                    parallelBypassEffect(parallelId, ParallelChain.A, newEffect.effectId)
                }
            }

            // Rebuild chain B
            snapshot.chainB.forEach { effectSnap ->
                parallelAddEffect(parallelId, ParallelChain.B, effectSnap.effectType)
                val effects = _parallelChains.value[parallelId]?.chainBEffects ?: return@forEach
                val newEffect = effects.lastOrNull() ?: return@forEach

                effectSnap.parameters.forEachIndexed { paramId, value ->
                    parallelSetParam(parallelId, ParallelChain.B, newEffect.effectId, paramId, value)
                }

                if (effectSnap.isBypassed) {
                    parallelBypassEffect(parallelId, ParallelChain.B, newEffect.effectId)
                }
            }
        }
    }

    /* OTHER PRESETS */

    fun saveCurvePreset(name: String, category: String?, points: List<CurvePoint>) : CurvePreset {
        val preset = CurvePreset(name, data = points, category, false)
        _curvePresets.update { presets -> presets.filterNot { it.name == name } + preset } // Add new preset or overwrite if same name
        return preset
    }

    fun deleteCurvePreset(name: String) {
        _curvePresets.update { it.filterNot { p -> p.name == name } }
    }

    fun favoriteCurvePreset(name: String, favorite: Boolean) {
        _curvePresets.update { list -> list.map { preset -> if (preset.name == name) preset.copy(favorite = favorite) else preset } }
    }

    /* UI */

    fun updateEditorState(modId: String, state: EditorState) {
        _editorStates.value = _editorStates.value.toMutableMap().apply { this[modId] = state }
    }

    /* CONTROL */

    // Clock all updates to control rate
    private val controlQueue = ControlQueue(
        scope = viewModelScope,
        rate = CONTROL_RATE,
        onFlush = { commands -> sendCommands(commands) },
        onUpdate = { }
    )

    @Suppress("unused")
    private val lfoControl = ControlQueue(
        scope = viewModelScope,
        rate = LFO_UPDATE_RATE,
        onFlush = { },
        onUpdate = { update() }
    )

    private fun update() {
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

    /* COMMANDS */

    // EFFECTS

    fun addEffect(type: EffectType) {
        val projected = chain.projectedUsage(type)
        val total = calculateTotalUsage()
        val projectedTotal = ResourceUsage(
            compute = total.compute + projected.compute - chain.totalUsage().compute,
            memory = total.memory + projected.memory - chain.totalUsage().memory
        )
        if (projectedTotal.compute > MAX_COMPUTE_USAGE || projectedTotal.memory > MAX_MEMORY_USAGE) {
            _resourceError.value = "Could not add ${type.uiName}: resource limit exceeded"
            return
        }

        chain.addEffect(type)
        _effects.value = chain.getAll()
        _resourceUsage.value = calculateTotalUsage()
        controlQueue.enqueue(EFFECT_ADD, type.ordinal, 0, 0.0f)

        // Init parallel state if applicable
        if (type == EffectType.PARALLEL) { _parallelChains.update { chains -> chains + (_effects.value.last().effectId to ParallelChainState()) } }
        // Log.d("cmd", "EFFECT_ADD: effectType=${type.name}")
    }

    fun removeEffect(effectId: Int) {
        val effect = chain.get(effectId)
        chain.removeEffect(effectId)
        _modAssignments.value = _modAssignments.value.filterNot { it.target.effectId == effectId } // Also remove mod assignments
        if (effect?.effectType == EffectType.PARALLEL) { _parallelChains.update { chains -> chains - effectId } } // Clean up any parallel state

        _effects.value = chain.getAll()
        _resourceUsage.value = calculateTotalUsage()

        controlQueue.enqueue(EFFECT_REMOVE, effectId, 0, 0.0f)
        _modAssignments.value.filter { it.target.effectId == effectId }.forEach { assignment -> // And downstream assignments
            val modIndex = modulatorIdToIndex(assignment.modId)
            controlQueue.enqueue(
                MOD_ASSIGNMENT_REMOVE, modIndex, assignment.target.effectId, assignment.target.paramId.toFloat()
            )
        }
        // Log.d("cmd", "EFFECT_REMOVE: effectId=$effectId")
    }

    fun reorderEffect(effectId: Int, toIndex: Int) {
        chain.reorderEffect(effectId, toIndex)
        _effects.value = chain.getAll()
        controlQueue.enqueue(EFFECT_REORDER, effectId, toIndex, 0.0f)
        // Log.d("cmd", "EFFECT_REORDER: effectId=$effectId, toIndex=$toIndex")
    }

    fun setParam(effectId: Int, paramId: Int, value: Any) {
        chain.setParam(effectId, paramId, value)
        _effects.value = chain.getAll()

        // Casting to float for value
        val sendValue = when (value) {
            is Float -> value
            is Int -> value.toFloat()
            is Boolean -> if (value) 1.0f else 0.0f

            is FFTSize -> value.value
            is EnvelopeType, is ModulationEffectMode, is DistortionMode, is BiquadType, is ParallelMode, is WavetableType, is ParamUnit
                -> value.ordinal.toFloat()

            else -> throw IllegalArgumentException("Unsupported value type")
        }

        controlQueue.enqueue(EFFECT_SET_PARAMETER, effectId, paramId, sendValue)
        // Log.d("cmd", "EFFECT_SET_PARAMETER: effectId=$effectId, paramId=$paramId, value=$value")
    }

    fun toggleBypass(effectId: Int) {
        val value = if (chain.get(effectId)!!.isBypassed) 0.0f else 1.0f
        chain.setBypass(effectId, !chain.get(effectId)!!.isBypassed)
        _effects.value = chain.getAll()
        controlQueue.enqueue(EFFECT_BYPASS, effectId, 0, value)
        // Log.d("cmd", "EFFECT_BYPASS: effectId=$effectId, bypass=$value")
    }

    fun clearChain() {
        chain.clear()
        _modAssignments.value = emptyList()
        _effects.value = chain.getAll()
        _parallelChains.value = emptyMap()
        _resourceUsage.value = ResourceUsage(0f, 0)
        controlQueue.enqueue(EFFECT_CLEAR, 0, 0, 0.0f)
        // Log.d("cmd", "EFFECT_CLEAR")
    }

    // MODULATION

    private fun modulatorIdToIndex(modId: String): Int { return _modulators.value.indexOfFirst { it.id == modId }.coerceIn(0, 11) }

    fun setModulatorParam(modId: String, paramId: Int, value: Any) {
        val mod = _modulators.value.find { it.id == modId } ?: return
        mod.setParam(paramId, value)

        // Casting to float for value
        val modIndex = modulatorIdToIndex(modId)
        val sendValue = when (value) {
            is Float -> value
            is Int -> value.toFloat()
            is LFOMode -> value.ordinal.toFloat()
            is RandomMode -> value.ordinal.toFloat()
            else -> throw IllegalArgumentException("Unsupported value type")
        }

        controlQueue.enqueue(MOD_SET_PARAMETER,
            modIndex, paramId, sendValue)
        // Log.d("cmd", "MOD_SET_PARAMETER: modId=$modIndex, paramId=$paramId, value=$sendValue")
    }

    fun updateModulatorCurve(modId: String, curve: List<CurvePoint>) {
        _modulators.value = _modulators.value.map { mod ->
            if (mod.id == modId) {
                mod.updateCurve(curve)
                when (mod) {
                    is Modulator.LFO -> mod.copy()
                    is Modulator.Mapping -> mod.copy()
                }
            } else mod
        }

        val modIndex = modulatorIdToIndex(modId)

        // Clear existing curve
        controlQueue.enqueue(MOD_CLEAR_CURVE,
            modIndex, 0, 0f)

        // Send each point
        curve.forEachIndexed { index, point ->
            controlQueue.enqueue(MOD_SET_CURVE_POINT,
                modIndex, index, point.x, point.y, point.curve)
        }

        // Reset LFO phase to sync with downstream
        val mod = _modulators.value.find { it.id == modId }
        if (mod is Modulator.LFO) mod.phase = 0f
        // Log.d("cmd", "MOD_UPDATE_CURVE")
    }

    fun addAssignment(modId: String, effectId: Int, paramId: Int) {
        val key = ParamKey(effectId, paramId)
        val assignList = _modAssignments.value.toMutableList()
        val exists = assignList.any { it.modId == modId && it.target == key } // Prevent duplicate assignments

        if (!exists) {
            val assignment = ModAssignment(
                modId = modId,
                target = key,
                amount = 0.5f,
                polarity = ModPolarity.Bipolar
            )
            assignList += assignment
            _modAssignments.value = assignList

            val modIndex = modulatorIdToIndex(modId)
            controlQueue.enqueue(MOD_ASSIGNMENT_ADD,
                modIndex, effectId, paramId.toFloat(), 0.5f, ModPolarity.Bipolar.ordinal.toFloat())
            // Log.d("cmd", "MOD_ASSIGNMENT_ADD: effectId=$effectId, paramId=$paramId, amount=0.5, polarity=${ModPolarity.Bipolar.ordinal}")
        }
    }

    fun removeAssignment(modId: String, effectId: Int, paramId: Int) {
        _modAssignments.value = _modAssignments.value.filterNot {
            it.modId == modId && it.target.effectId == effectId && it.target.paramId == paramId
        }

        val modIndex = modulatorIdToIndex(modId)
        controlQueue.enqueue(MOD_ASSIGNMENT_REMOVE,
            modIndex, effectId, paramId.toFloat())
        // Log.d("cmd", "MOD_ASSIGNMENT_REMOVE: effectId=$effectId, paramId=$paramId")
    }

    fun updateAssignmentAmount(modId: String, effectId: Int, paramId: Int, amount: Float) {
        _modAssignments.value = _modAssignments.value.map {
            if (it.modId == modId && it.target.effectId == effectId && it.target.paramId == paramId)
                it.copy(amount = amount)
            else it
        }

        val modIndex = modulatorIdToIndex(modId)
        val assignment = _modAssignments.value.find {
            it.modId == modId && it.target.effectId == effectId && it.target.paramId == paramId
        } ?: return

        controlQueue.enqueue(MOD_ASSIGNMENT_SET,
            modIndex, effectId, paramId.toFloat(), assignment.amount, assignment.polarity.ordinal.toFloat())
        // Log.d("cmd", "MOD_ASSIGNMENT_SET: effectId=$effectId, paramId=$paramId, amount=${assignment.amount}, polarity=${assignment.polarity.ordinal}")
    }

    fun updateAssignmentPolarity(modId: String, effectId: Int, paramId: Int) {
        _modAssignments.value = _modAssignments.value.map {
            if (it.modId == modId && it.target.effectId == effectId && it.target.paramId == paramId) {
                val newPolarity = when (it.polarity) {
                    ModPolarity.Bipolar -> ModPolarity.Unipolar
                    ModPolarity.Unipolar -> ModPolarity.Bipolar
                }
                it.copy(polarity = newPolarity)
            } else it
        }

        val modIndex = modulatorIdToIndex(modId)
        val assignment = _modAssignments.value.find {
            it.modId == modId && it.target.effectId == effectId && it.target.paramId == paramId
        } ?: return

        controlQueue.enqueue(MOD_ASSIGNMENT_SET,
            modIndex, effectId, paramId.toFloat(), assignment.amount, assignment.polarity.ordinal.toFloat())
        // Log.d("cmd", "MOD_ASSIGNMENT_SET: effectId=$effectId, paramId=$paramId, amount=${assignment.amount}, polarity=${assignment.polarity.ordinal}")
    }

    /* // Probably unused here - ESP32 should manually send
    fun setMappingInput(modId: String, normalizedInput: Float) {
        val mod = _modulators.value.find { it.id == modId }
        if (mod is Modulator.Mapping) {
            mod.inputValue = normalizedInput

            val modIndex = modulatorIdToIndex(modId)
            controlQueue.enqueue(CommandType.MOD_MAPPING_SET_INPUT, modIndex, 0, normalizedInput)
        }
    }
    */

    private fun syncModulation() { // Sync modulation with downstream
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
        // Log.d("cmd", "SYNC MODULATION")
    }

    // PARALLEL

    private fun encodeChainParam(chain: ParallelChain, paramId: Int): Int {
        val chainBit = if (chain == ParallelChain.B) 1 else 0
        return (chainBit shl 4) or (paramId and 0x0F)
    }

    private fun getParallelEffectChain(parallelId: Int, chain: ParallelChain): EffectChain? {
        val state = _parallelChains.value[parallelId] ?: return null
        return when (chain) {
            ParallelChain.A -> state.chainA
            ParallelChain.B -> state.chainB
        }
    }

    private fun updateParallelChainState(parallelId: Int) {
        _parallelChains.update { chains ->
            val state = chains[parallelId] ?: return@update chains
            chains + (parallelId to state.copy(
                chainAEffects = state.chainA.getAll(),
                chainBEffects = state.chainB.getAll(),
                chainAUsage = state.chainA.totalUsage(),
                chainBUsage = state.chainB.totalUsage()
            ))
        }
    }

    fun parallelAddEffect(parallelId: Int, chain: ParallelChain, effectType: EffectType) {
        val effectChain = getParallelEffectChain(parallelId, chain) ?: return

        val projected = effectChain.projectedUsage(effectType)
        val total = calculateTotalUsage()
        val projectedTotal = ResourceUsage(
            compute = total.compute + projected.compute - effectChain.totalUsage().compute,
            memory = total.memory + projected.memory - effectChain.totalUsage().memory
        )
        if (projectedTotal.compute > MAX_COMPUTE_USAGE || projectedTotal.memory > MAX_MEMORY_USAGE) {
            _resourceError.value = "Could not add ${effectType.uiName}: resource limit exceeded"
            return
        }

        effectChain.addEffect(effectType)
        updateParallelChainState(parallelId)
        _resourceUsage.value = calculateTotalUsage()

        controlQueue.enqueue(PARALLEL_CHAIN_COMMAND,
            parallelId, encodeChainParam(chain, 0), 0f, 0f, effectType.ordinal.toFloat())
        // Log.d("cmd", "PARALLEL id=$parallelId - EFFECT_ADD: effectType=$effectType")
    }

    fun parallelRemoveEffect(parallelId: Int, chain: ParallelChain, effectId: Int) {
        val effectChain = getParallelEffectChain(parallelId, chain) ?: return
        effectChain.removeEffect(effectId)
        updateParallelChainState(parallelId)
        _resourceUsage.value = calculateTotalUsage()

        controlQueue.enqueue(PARALLEL_CHAIN_COMMAND,
            parallelId, encodeChainParam(chain, 0), 1f, effectId.toFloat(), 0f)
        // Log.d("cmd", "PARALLEL id=$parallelId - EFFECT_REMOVE: effectId=$effectId")
    }

    fun parallelReorderEffect(parallelId: Int, chain: ParallelChain, effectId: Int, toIndex: Int) {
        val effectChain = getParallelEffectChain(parallelId, chain) ?: return
        effectChain.reorderEffect(effectId, toIndex)
        updateParallelChainState(parallelId)

        controlQueue.enqueue(PARALLEL_CHAIN_COMMAND,
            parallelId, encodeChainParam(chain, 0), 2f, effectId.toFloat(), toIndex.toFloat())
        // Log.d("cmd", "PARALLEL id=$parallelId - EFFECT_REORDER: effectId=$effectId, toIndex=$toIndex")
    }

    fun parallelSetParam(parallelId: Int, chain: ParallelChain, effectId: Int, paramId: Int, value: Any) {
        val effectChain = getParallelEffectChain(parallelId, chain) ?: return
        effectChain.setParam(effectId, paramId, value)
        updateParallelChainState(parallelId)

        // Casting to float for value
        val sendValue = when (value) {
            is Float -> value
            is Int -> value.toFloat()
            is Boolean -> if (value) 1.0f else 0.0f

            is EnvelopeType, is ModulationEffectMode, is DistortionMode, is BiquadType, is ParallelMode, is WavetableType, is ParamUnit
                -> value.ordinal.toFloat()

            else -> throw IllegalArgumentException("Unsupported value type")
        }

        controlQueue.enqueue(PARALLEL_CHAIN_COMMAND,
            parallelId, encodeChainParam(chain, paramId), 3f, effectId.toFloat(), sendValue)
        // Log.d("cmd", "PARALLEL id=$parallelId - EFFECT_SET_PARAMETER: effectId=$effectId, paramId=$paramId, value=$value")
    }

    fun parallelBypassEffect(parallelId: Int, chain: ParallelChain, effectId: Int) {
        val effectChain = getParallelEffectChain(parallelId, chain) ?: return
        val value = if (effectChain.get(effectId)!!.isBypassed) 0.0f else 1.0f
        effectChain.setBypass(effectId, !effectChain.get(effectId)!!.isBypassed)
        updateParallelChainState(parallelId)

        controlQueue.enqueue(PARALLEL_CHAIN_COMMAND,
            parallelId, encodeChainParam(chain, 0), 4f, effectId.toFloat(), value)

        // Log.d("cmd", "PARALLEL id=$parallelId - EFFECT_BYPASS: effectId=$effectId, bypass=$value")
    }

    /* BLE */

    /**
     * Sends a list of commands to the BLE device in a single buffer.
     * @param commands The list of commands to send
     */
    private fun sendCommands(commands: List<Command>) {
        if (commands.isEmpty()) return

        // 16 bytes per Command
        val buffer = ByteBuffer.allocate(commands.size * 16).order(ByteOrder.LITTLE_ENDIAN)

        for (command in commands) {
//            Log.e("cmd", "Sending command: $command")
            buffer.put(command.type.value)
            buffer.put(command.id1.toByte())
            buffer.put(command.id2.toByte())
            // Compute checksum
            /*
            val checksum = command.type.value xor
                    command.id1.toByte() xor
                    command.id2.toByte() xor
                    command.value1.toBits().toByte() xor
                    ((command.value1.toBits() shr 8) and 0xFF).toByte() xor
                    ((command.value1.toBits() shr 16) and 0xFF).toByte() xor
                    ((command.value1.toBits() shr 24) and 0xFF).toByte() xor
                    command.value2.toBits().toByte() xor
                    ((command.value2.toBits() shr 8) and 0xFF).toByte() xor
                    ((command.value2.toBits() shr 16) and 0xFF).toByte() xor
                    ((command.value2.toBits() shr 24) and 0xFF).toByte() xor
                    command.value3.toBits().toByte() xor
                    ((command.value3.toBits() shr 8) and 0xFF).toByte() xor
                    ((command.value3.toBits() shr 16) and 0xFF).toByte() xor
                    ((command.value3.toBits() shr 24) and 0xFF).toByte()
            */
            val checksum = 0.toByte() // TEMP
            buffer.put(checksum)
            buffer.putFloat(command.value1)
            buffer.putFloat(command.value2)
            buffer.putFloat(command.value3)
        }
        bleManager.writeCharacteristic(buffer.array())
    }
} // MainViewModel
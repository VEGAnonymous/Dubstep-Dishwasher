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
import lol.pony.dubstepdishwasher.model.ControlQueue
import lol.pony.dubstepdishwasher.model.EffectChain
import lol.pony.dubstepdishwasher.model.BLEManager
import lol.pony.dubstepdishwasher.model.core.*
import lol.pony.dubstepdishwasher.model.core.CommandType.*
import java.nio.ByteBuffer
import java.nio.ByteOrder
// import kotlin.experimental.xor

class MainViewModel(private val bleManager: BLEManager) : ViewModel() {

    /* DATA STRUCTURES */

    // System
    private val _resourceUsage = MutableStateFlow(ResourceUsage(0f, 0))
    val resourceUsage = _resourceUsage

    private val _resourceError = MutableStateFlow<String?>(null)
    val resourceError: StateFlow<String?> = _resourceError
    fun clearResourceError() { _resourceError.value = null }

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
            _editorStates
        ) { effects, modulators, assignments, editorStates ->
            GlobalPresetData(
                effects = effects.map {
                    EffectSnapshot(
                        effectType = it.effectType,
                        parameters = it.parameters.map { p -> p.value },
                        isBypassed = it.isBypassed
                    )
                },
                modulators = modulators.map {
                    ModulatorSnapshot(
                        id = it.id,
                        isLFO = it is Modulator.LFO,
                        parameters = it.parameters.map { p -> p.value },
                        curve = it.curve.map { c -> c.copy() }
                    )
                },
                assignments = assignments.map { it.copy() },
                editorStates = editorStates.mapValues { it.value.copy() }
            )
        }.stateIn(
            viewModelScope,
            SharingStarted.Eagerly,
            // Initial value
            GlobalPresetData(
                effects = emptyList(),
                modulators = emptyList(),
                assignments = emptyList(),
                editorStates = emptyMap()
            )
        )

    fun saveGlobalPreset(name: String, category: String?) : GlobalPreset {
        val data = GlobalPresetData(
            effects = snapshotEffects(),
            modulators = snapshotModulators(),
            assignments = _modAssignments.value.map { it.copy() },
            editorStates = _editorStates.value.mapValues { it.value.copy() }
        )
        val preset = GlobalPreset(name, data, category, false)
        _globalPresets.update { presets -> presets.filterNot { it.name == name } + preset }
        return preset
    }

    fun loadGlobalPreset(name: String) {
        val preset = _globalPresets.value.find { it.name == name } ?: return
        val data = preset.data

        clearChain()
        rebuildEffects(data.effects)
        rebuildModulators(data.modulators)
        _modAssignments.value = data.assignments.map { it.copy() }
        _editorStates.value = data.editorStates.mapValues { it.value.copy() }

        syncModulation()
    }

    fun deleteGlobalPreset(name: String) {
        _globalPresets.update { it.filterNot { preset -> preset.name == name } }
    }

    fun favoriteGlobalPreset(name: String, favorite: Boolean) {
        _globalPresets.update { list -> list.map { preset -> if (preset.name == name) preset.copy(favorite = favorite) else preset } }
    }

    private fun snapshotEffects(): List<EffectSnapshot> {
        return _effects.value.map { effect ->
            EffectSnapshot(
                effectType = effect.effectType,
                parameters = effect.parameters.map { it.value },
                isBypassed = effect.isBypassed
            )
        }
    }

    private fun snapshotModulators(): List<ModulatorSnapshot> {
        return _modulators.value.map { mod ->
            ModulatorSnapshot(
                id = mod.id,
                isLFO = mod is Modulator.LFO,
                parameters = mod.parameters.map { p -> p.value },
                curve = mod.curve.map { it.copy() }
            )
        }
    }

    private fun rebuildEffects(list: List<EffectSnapshot>) {
        list.forEach { snap ->
            addEffect(snap.effectType)
            val newEffect = _effects.value.last()
            snap.parameters.forEachIndexed { paramId, value ->
                if (value != null) setParam(newEffect.effectId, paramId, value)
            }
            if (snap.isBypassed) toggleBypass(newEffect.effectId)
        }
    }

    private fun rebuildModulators(list: List<ModulatorSnapshot>) {
        _modulators.value = list.map { snap ->
            val mod = if (snap.isLFO) Modulator.LFO(snap.id, curve = snap.curve.map { it.copy() })
            else Modulator.Mapping(snap.id, curve = snap.curve.map { it.copy() })

            snap.parameters.forEachIndexed { paramId, value -> if (value != null) mod.setParam(paramId, value) }
            mod
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
        if (projected.compute > MAX_COMPUTE_USAGE || projected.memory > MAX_MEMORY_USAGE) {
            _resourceError.value = "Could not add ${type.uiName}: resource limit exceeded"
            return
        }

        chain.addEffect(type)
        _effects.value = chain.getAll()
        _resourceUsage.value = chain.totalUsage()
        controlQueue.enqueue(EFFECT_ADD, type.ordinal, 0, 0.0f)
        // Log.d("cmd", "EFFECT_ADD: effectType=${type.name}")
    }

    fun removeEffect(effectId: Int) {
        chain.removeEffect(effectId)
        _modAssignments.value = _modAssignments.value.filterNot { it.target.effectId == effectId } // Also remove mod assignments

        _effects.value = chain.getAll()
        _resourceUsage.value = chain.totalUsage()

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
        // Log.d("cmd", "EFFECT_BYPASS: effectId=$effectId")
    }

    fun clearChain() {
        chain.clear()
        _modAssignments.value = emptyList()
        _effects.value = chain.getAll()
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
                when (val value = param.value) {
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
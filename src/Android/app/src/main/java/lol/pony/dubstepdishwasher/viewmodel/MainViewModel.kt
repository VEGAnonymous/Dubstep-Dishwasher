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
        update()
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

    /* MODULATION */

    fun addAssignment(modId: String, effectId: Int, paramId: Int) {
        val key = ParamKey(effectId, paramId)
        val assignList = _modAssignments.value.toMutableList()
        val exists = assignList.any { it.modId == modId && it.target == key } // Prevent duplicate assignments

        if (!exists) {
            assignList += ModAssignment(
                modId = modId,
                target = key,
                amount = 0.5f,
                polarity = ModPolarity.Bipolar
            )
            _modAssignments.value = assignList
        }
    }

    fun removeAssignment(modId: String, effectId: Int, paramId: Int) {
        _modAssignments.value = _modAssignments.value.filterNot {
            it.modId == modId && it.target.effectId == effectId && it.target.paramId == paramId
        }
    }

    fun updateAssignmentAmount(modId: String, effectId: Int, paramId: Int, amount: Float) {
        _modAssignments.value = _modAssignments.value.map {
            if (it.modId == modId && it.target.effectId == effectId && it.target.paramId == paramId)
                it.copy(amount = amount)
            else it
        }
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
    }

    fun setModulatorParam(modId: String, paramId: Int, value: Any) {
        val mod = _modulators.value.find { it.id == modId } ?: return
        mod.setParam(paramId, value)
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
    }

    /* UI */

    fun updateEditorState(modId: String, state: EditorState) {
        _editorStates.value = _editorStates.value.toMutableMap().apply { this[modId] = state }
    }

    /* CONTROL */

    // Clock all updates to control rate
    private val controlQueue = ControlQueue(
        scope = viewModelScope,
        rate = 50,
        onFlush = { cmd -> sendCommand(cmd.type, cmd.id1, cmd.id2, cmd.value) },
        onUpdate = { update() }
    )

    private fun update() {
        /* Apply modulation */
        val dt = 1f / 50f // 50Hz
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

        offsets.forEach { (paramKey, offset) -> // Apply offsets to all targets
            val effect = chain.get(paramKey.effectId) ?: return@forEach
            val param = effect.getParam(paramKey.paramId) ?: return@forEach
            if (param !is EffectParameter.Range<*>) return@forEach

            val baseValue = param.value.toFloat()

            // Compute final modulated value
            val span = param.range.second.toFloat() - param.range.first.toFloat()
            val modulatedValue = (baseValue + offset * span).coerceIn(param.range.first.toFloat(), param.range.second.toFloat())
            // println(modulatedValue)

            // Send command
            /* TODO: Make this optional for internal-only parameters (like modulating LFO rates
               Although that's (impossible currently because you can't drag to another modulator tab...yet) */
            controlQueue.enqueue(SET_PARAM, paramKey.effectId, paramKey.paramId, modulatedValue)
        }
    }

    /* COMMANDS */

    fun addEffect(type: EffectType) {
        val projected = chain.projectedUsage(type)
        if (projected.compute > MAX_COMPUTE_USAGE || projected.memory > MAX_MEMORY_USAGE) {
            _resourceError.value = "Could not add ${type.uiName}: resource limit exceeded"
            return
        }

        chain.addEffect(type)
        _effects.value = chain.getAll()
        _resourceUsage.value = chain.totalUsage()
        controlQueue.enqueue(ADD, type.ordinal, 0, 0.0f)
        // Log.d("cmd", "ADD: effectType=${type.name}")
    }

    fun removeEffect(effectId: Int) {
        chain.removeEffect(effectId)
        _modAssignments.value = _modAssignments.value.filter { it.target.effectId != effectId } // Also remove mod assignments

        _effects.value = chain.getAll()
        _resourceUsage.value = chain.totalUsage()
        controlQueue.enqueue(REMOVE, effectId, 0, 0.0f)
        // Log.d("cmd", "REMOVE: effectId=$effectId")
    }

    fun reorderEffect(effectId: Int, toIndex: Int) {
        chain.reorderEffect(effectId, toIndex)
        _effects.value = chain.getAll()
        controlQueue.enqueue(REORDER, effectId, toIndex, 0.0f)
        // Log.d("cmd", "REORDER: effectId=$effectId, toIndex=$toIndex")
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

        controlQueue.enqueue(SET_PARAM, effectId, paramId, sendValue)
        // Log.d("cmd", "SET_PARAM: effectId=$effectId, paramId=$paramId, value=$value")
    }

    fun toggleBypass(effectId: Int) {
        chain.setBypass(effectId, !chain.get(effectId)!!.isBypassed)
        _effects.value = chain.getAll()
        val value = if (chain.get(effectId)!!.isBypassed) 0.0f else 1.0f
        controlQueue.enqueue(BYPASS, effectId, 0, value)
        // Log.d("cmd", "BYPASS: effectId=$effectId")
    }

    fun clearChain() {
        chain.clear()
        _modAssignments.value = emptyList()
        _effects.value = chain.getAll()
        _resourceUsage.value = ResourceUsage(0f, 0)
        controlQueue.enqueue(CLEAR, 0, 0, 0.0f)
        // Log.d("cmd", "CLEAR")
    }

    /* BLE */

    // Convert byte array to string with space separator
    private fun bytesToHexString(bytes: ByteArray): String { return bytes.joinToString(" ") { "%02X".format(it) } }

    /**
     * Sends a command to the BLE device via a
     * byte array produced by a buffer.
     * @param cmd Command of type [CommandType] to send
     * @param id1 EffectID for command
     * @param id2 ParamID for setParam or 2nd EffectID for reorderEffect
     * @param value Float value for setParam/toggleBypass
     */
    private fun sendCommand(cmd: CommandType, id1: Int, id2: Int, value: Float) {
        // Log.d("sendCommand", "SENT: CommandType=$cmd, id1=$id1, id2=$id2, value=$value")

        // Initialize buffer
        val buffer = ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN)
        // Write buffer values
        buffer.put(cmd.value)       // cmd
        buffer.put(id1.toByte())    // id1
        buffer.put(id2.toByte())    // id2
        buffer.put(0.toByte())      // checksum (always 0)
        buffer.putFloat(value)          // value

        // Converts buffer to array and then hex string for writeCommand
        // TEMP: Might just remove the hexString part later
        val commandBytes = buffer.array()
        val hexString = bytesToHexString(commandBytes)

        bleManager.writeCharacteristic(hexString)
    }
} // MainViewModel
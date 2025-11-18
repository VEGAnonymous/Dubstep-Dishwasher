package lol.pony.dubstepdishwasher.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.flow.*
import kotlinx.coroutines.launch
import lol.pony.dubstepdishwasher.CPU_LIMIT
import lol.pony.dubstepdishwasher.model.ControlQueue
import lol.pony.dubstepdishwasher.model.EffectChain
import lol.pony.dubstepdishwasher.model.BLEManager
import lol.pony.dubstepdishwasher.model.core.*
import lol.pony.dubstepdishwasher.model.core.CommandType.*
import java.nio.ByteBuffer
import java.nio.ByteOrder

class MainViewModel(private val bleManager: BLEManager) : ViewModel() {

    /* DATA STRUCTURES */

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
            // ML quality regression
            Modulator.Mapping(id = "Bright"),
            Modulator.Mapping(id = "Warmth"),
            Modulator.Mapping(id = "Intense"),
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
    private val _curvePresets = MutableStateFlow(defaultCurvePresets())
    val curvePresets: StateFlow<List<CurvePreset>> = _curvePresets

    // UI Events
    private val _toastMessages = MutableSharedFlow<String>()
    val toastMessages = _toastMessages.asSharedFlow()

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

    /* PRESETS */

    fun saveCurvePreset(name: String, points: List<CurvePoint>) : CurvePreset {
        val preset = CurvePreset(name, points)
        _curvePresets.update { presets -> presets.filterNot { it.name == name } + preset } // Add new preset or overwrite if same name
        return preset
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
        val totalUsage = _effects.value.sumOf { it.cpuUsage?.toDouble() ?: 0.0 }.toFloat()
        if (totalUsage + (type.cpuUsage ?: 0.0f) > CPU_LIMIT) {
            viewModelScope.launch {
                _toastMessages.emit("${type.uiName} not added: Over CPU limit")
            }
            return
        }

        viewModelScope.launch {
            _toastMessages.emit("${type.uiName} added")
        }

        chain.addEffect(type)
        _effects.value = chain.getAll()
        controlQueue.enqueue(ADD, type.ordinal, 0, 0.0f)
    }

    fun removeEffect(effectId: Int) {
        chain.removeEffect(effectId)
        _modAssignments.value = _modAssignments.value.filter { it.target.effectId != effectId } // Also remove mod assignments
        _effects.value = chain.getAll()
        controlQueue.enqueue(REMOVE, effectId, 0, 0.0f)
    }

    fun reorderEffect(effectId: Int, toIndex: Int) {
        chain.reorderEffect(effectId, toIndex)
        _effects.value = chain.getAll()
        controlQueue.enqueue(REORDER, effectId, toIndex, 0.0f)
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
    }

    fun toggleBypass(effectId: Int) {
        chain.setBypass(effectId, !chain.get(effectId)!!.isBypassed)
        _effects.value = chain.getAll()
        val value = if (chain.get(effectId)!!.isBypassed) 0.0f else 1.0f
        controlQueue.enqueue(BYPASS, effectId, 0, value)
    }

    fun clearChain() {
        chain.clear()
        _modAssignments.value = emptyList()
        _effects.value = chain.getAll()
        controlQueue.enqueue(CLEAR, 0, 0, 0.0f)
    }

    /**
     * Converts byte array to string with space separator.
     * @param bytes The byte array to convert to string.
     * @return String representation of [bytes].
     */
    private fun bytesToHexString(bytes: ByteArray): String {
        return bytes.joinToString(" ") { "%02X".format(it) }
    }

    /**
     * Sends a command to the BLE device via a
     * byte array produced by a buffer.
     * @param cmd Command of type [CommandType] to send.
     * @param id1 EffectID for command.
     * @param id2 ParamID for setParam or 2nd EffectID for reorderEffect.
     * @param value Float value for setParam/toggleBypass.
     */
    private fun sendCommand(cmd: CommandType, id1: Int, id2: Int, value: Float) {
        // initialize buffer
        val buffer = ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN)
        // write buffer values
        buffer.put(cmd.value)       // cmd
        buffer.put(id1.toByte())    // id1
        buffer.put(id2.toByte())    // id2
        buffer.put(0.toByte())      // checksum (always 0)
        buffer.putFloat(value)          // value

        // converts buffer to array and then hex string for writeCommand
        // might just remove the hexString part later
        val commandBytes = buffer.array()
        val hexString = bytesToHexString(commandBytes)

        bleManager.writeCharacteristic(hexString)
    }
}

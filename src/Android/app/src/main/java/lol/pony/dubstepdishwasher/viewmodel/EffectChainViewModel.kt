package lol.pony.dubstepdishwasher.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import lol.pony.dubstepdishwasher.model.ControlQueue
import lol.pony.dubstepdishwasher.model.EffectChain
import lol.pony.dubstepdishwasher.model.core.BiquadType
import lol.pony.dubstepdishwasher.model.BLEManager
import lol.pony.dubstepdishwasher.model.core.CommandType
import lol.pony.dubstepdishwasher.model.core.CommandType.*
import lol.pony.dubstepdishwasher.model.core.DistortionMode
import lol.pony.dubstepdishwasher.model.core.Effect
import lol.pony.dubstepdishwasher.model.core.EffectType
import lol.pony.dubstepdishwasher.model.core.EnvelopeType
import lol.pony.dubstepdishwasher.model.core.ModulationEffectMode
import lol.pony.dubstepdishwasher.model.core.ParallelMode
import lol.pony.dubstepdishwasher.model.core.ParamUnit
import lol.pony.dubstepdishwasher.model.core.WavetableType
import java.nio.ByteBuffer
import java.nio.ByteOrder

class EffectChainViewModel(private val bleManager: BLEManager) : ViewModel() {
    private val chain = EffectChain()
    private val _effects = MutableStateFlow<List<Effect>>(emptyList())
    val effects: StateFlow<List<Effect>> = _effects

    // Clock all updates to control rate
    private val controlQueue = ControlQueue(
        scope = viewModelScope,
        rate = 50,
        onFlush = { cmd -> sendCommand(cmd.type, cmd.id1, cmd.id2, cmd.value) }
    )

    fun addEffect(type: EffectType) {
        chain.addEffect(type)
        _effects.value = chain.getAll()
        controlQueue.enqueue(ADD, type.ordinal, 0, 0.0f)
    }

    fun removeEffect(effectId: Int) {
        chain.removeEffect(effectId)
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
            is EnvelopeType -> value.ordinal.toFloat()
            is ModulationEffectMode -> value.ordinal.toFloat()
            is DistortionMode -> value.ordinal.toFloat()
            is BiquadType -> value.ordinal.toFloat()
            is ParallelMode -> value.ordinal.toFloat()
            is WavetableType -> value.ordinal.toFloat()
            is ParamUnit -> value.ordinal.toFloat()
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
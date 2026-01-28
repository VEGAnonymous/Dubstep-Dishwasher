package lol.pony.dubstepdishwasher.viewmodel

import android.util.Log
import lol.pony.dubstepdishwasher.model.core.*
import java.nio.ByteBuffer
import java.nio.ByteOrder

internal fun MainViewModel.sendCommands(commands: List<Command>) {
    if (commands.isEmpty()) return

    val packetSize = 19
    val buffer = ByteBuffer.allocate(commands.size * packetSize).order(ByteOrder.LITTLE_ENDIAN)

    for (command in commands) {
        // Sequence number for structural commands
        val seq = if (getCommandPriority(command.type) == CommandPriority.PRIORITY_STRUCTURE) {
            commandSequence++
            pendingCommands[commandSequence] = PendingCommand(command)
            commandSequence
        } else 0.toByte()

        // Build packet
        val pkt = ByteBuffer.allocate(packetSize).order(ByteOrder.LITTLE_ENDIAN)
        pkt.putShort(0xAA55.toShort())
        pkt.put(command.type.value)
        pkt.put(command.id1.toByte())
        pkt.put(command.id2.toByte())
        pkt.put(0) // Checksum placeholder
        pkt.putFloat(command.value1)
        pkt.putFloat(command.value2)
        pkt.putFloat(command.value3)
        pkt.put(seq)

        // Compute checksum
        val arr = pkt.array()
        var checksum: Byte = 0
        for (i in arr.indices) {
            if (i != 5) checksum = (checksum.toInt() xor arr[i].toInt()).toByte()
        }; arr[5] = checksum
        buffer.put(arr)
    }
    bleManager.writeCharacteristic(buffer.array())
}

internal fun MainViewModel.handleStatus(status: Status) {
    when (status.type) {
        StatusType.HEARTBEAT -> { heartbeatTime = System.currentTimeMillis() }
        StatusType.ACK -> { pendingCommands.remove(status.id) }
        StatusType.INFERENCE -> {
            // Update mapping modulators
            val brightness = status.value1
            val warmth = status.value2
            val intensity = status.value3
            val percussive = status.getPercussive()
            val speed = status.getSpeed()

            _modulators.value = _modulators.value.map { mod ->
                when (mod.id) {
                    "Bright" -> { setMappingInput(mod.id, brightness); mod }
                    "Warmth" -> { setMappingInput(mod.id, warmth); mod }
                    "Intensity" -> { setMappingInput(mod.id, intensity); mod }
                    "Perc" -> { setMappingInput(mod.id, percussive); mod }
                    "Speed" -> { setMappingInput(mod.id, speed); mod }
                    else -> mod
                }
            }
            if (LOG_DEBUG) Log.d("Status", "Inference: B=$brightness W=$warmth I=$intensity P=$percussive S=$speed")
        }
        StatusType.EXPR -> { setMappingInput("Expr", status.value1) }
    }
}

internal fun MainViewModel.handleRetry(cmd: Command) {
    if (LOG_DEBUG) Log.d("ACK", "Checking ACK for seq=${cmd.seq}: ${pendingCommands[cmd.seq]}")
    val pending = pendingCommands[cmd.seq] ?: return // Only retry if still pending

    if (pending.retryCount >= MAX_ACK_RETRIES) {
        // Fucking give up
        if (LOG_DEBUG) Log.e("ACK", "Command seq ${cmd.seq} failed after $MAX_ACK_RETRIES attempts")
        pendingCommands.remove(cmd.seq)

        if (_syncState.value == SyncState.SYNCED) resync() // Trigger full resync
        return
    }

    pending.retryCount++
    if (LOG_DEBUG) Log.w("ACK", "Retrying seq ${cmd.seq}, attempt ${pending.retryCount}")
    controlQueue.enqueue(cmd.type, cmd.id1, cmd.id2,cmd.value1, cmd.value2, cmd.value3, cmd.seq) // Keep seq
}
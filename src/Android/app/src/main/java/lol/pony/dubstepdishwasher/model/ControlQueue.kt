package lol.pony.dubstepdishwasher.model

import android.util.Log
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import lol.pony.dubstepdishwasher.model.core.ACK_TIMEOUT
import lol.pony.dubstepdishwasher.model.core.CONTROL_RATE
import lol.pony.dubstepdishwasher.model.core.Command
import lol.pony.dubstepdishwasher.model.core.CommandKey
import lol.pony.dubstepdishwasher.model.core.CommandPriority
import lol.pony.dubstepdishwasher.model.core.CommandType
import lol.pony.dubstepdishwasher.model.core.getCommandPriority
import java.util.concurrent.ConcurrentLinkedQueue

class ControlQueue(
    private val scope: CoroutineScope,
    private val rate: Int = CONTROL_RATE,
    private val onFlush: (List<Command>) -> Unit,
    private val onUpdate: (() -> Unit)? = null,
    private val onRetry: ((Command) -> Unit)? = null
) {
    private val queue = ConcurrentLinkedQueue<Command>()
    private val stateCommands = mutableMapOf<CommandKey, Command>()

    init {
        scope.launch {
            while (true) { // Constantly flush queue according to control rate
                delay(1000L / rate)
                flush()
            }
        }
    }

    fun enqueue(cmd: CommandType, id1: Int, id2: Int, value1: Float,
                value2: Float = 0f, value3: Float = 0f, seq: Byte = 0) {

        val command = Command(0xAA55.toShort(), cmd, id1, id2, value1, value2, value3, seq)
        if (getCommandPriority(cmd) == CommandPriority.PRIORITY_STATE) { // Overwrite old values for state commands
            val key = CommandKey(cmd, id1, id2)
            synchronized(stateCommands) { stateCommands[key] = command }
        } else queue.offer(command) // Push structural commands in order

        // Schedule retry
        if (getCommandPriority(cmd) == CommandPriority.PRIORITY_STRUCTURE && seq.toInt() != 0) {
            scope.launch {
                delay(ACK_TIMEOUT)
                onRetry?.invoke(command)
            }
        }
    }

    private fun flush() {
        onUpdate?.invoke()

        val toSend = mutableListOf<Command>()

        synchronized(stateCommands) {
            toSend.addAll(stateCommands.values)
            stateCommands.clear()
        }

        while (true) {
            val c = queue.poll() ?: break
            toSend.add(c)
        }

        if (toSend.isNotEmpty()) onFlush(toSend)
    }

    fun flushNow() { flush() }
}
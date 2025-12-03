package lol.pony.dubstepdishwasher.model

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import lol.pony.dubstepdishwasher.model.core.CONTROL_RATE
import lol.pony.dubstepdishwasher.model.core.Command
import lol.pony.dubstepdishwasher.model.core.CommandKey
import lol.pony.dubstepdishwasher.model.core.CommandType
import lol.pony.dubstepdishwasher.model.core.isStateCommand
import java.util.concurrent.ConcurrentLinkedQueue

class ControlQueue(
    scope: CoroutineScope,
    private val rate: Int = CONTROL_RATE,
    private val onFlush: (List<Command>) -> Unit,
    private val onUpdate: (() -> Unit)? = null
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
                value2: Float = 0f, value3: Float = 0f) {

        val command = Command(cmd, id1, id2, value1, value2, value3)
        if (isStateCommand(cmd)) {
            val key = CommandKey(cmd, id1, id2)
            synchronized(stateCommands) { stateCommands[key] = command } // Overwrite old value
        } else queue.offer(command) // Push structural commands in order
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

    fun flushNow() {
        flush()
    }
}
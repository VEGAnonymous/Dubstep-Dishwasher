package lol.pony.dubstepdishwasher.model

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import lol.pony.dubstepdishwasher.model.core.CONTROL_RATE
import lol.pony.dubstepdishwasher.model.core.CommandType
import java.util.concurrent.ConcurrentLinkedQueue

data class Command(
    val type: CommandType,
    val id1: Int,
    val id2: Int,
    val value: Float
)

class ControlQueue(
    scope: CoroutineScope,
    private val rate: Int = CONTROL_RATE,
    private val onFlush: (Command) -> Unit,
    private val onUpdate: (() -> Unit)? = null
) {
    private val queue = ConcurrentLinkedQueue<Command>()
    private val paramUpdates = mutableMapOf<Pair<Int, Int>, Command>()

    init {
        scope.launch {
            while (true) { // Constantly flush queue according to control rate
                delay(1000L / rate)
                flush()
            }
        }
    }

    fun enqueue(cmd: CommandType, id1: Int, id2: Int, value: Float) {
        val command = Command(cmd, id1, id2, value)
        if (cmd == CommandType.SET_PARAM) { // For param setting, only keep latest
            synchronized(paramUpdates) { paramUpdates[Pair(id1, id2)] = command }
        } else queue.offer(command)
    }

    private fun flush() {
        onUpdate?.invoke()

        // Only keep latest values for each parameter
        val params = synchronized(paramUpdates) {
            val latest = paramUpdates.values.toList()
            paramUpdates.clear()
            latest
        }

        // Send only one param update per flush tick
        val param = params.firstOrNull()
        if (param != null) { onFlush(param); return }

        // Otherwise send one queued command per tick
        val cmd = queue.poll()
        if (cmd != null) onFlush(cmd)
    }
}
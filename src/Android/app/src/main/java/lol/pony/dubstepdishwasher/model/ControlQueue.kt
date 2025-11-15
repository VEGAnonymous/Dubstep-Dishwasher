package lol.pony.dubstepdishwasher.model

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
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
    private val rate: Int = 50, // 50 Hz control rate
    private val onFlush: (Command) -> Unit
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
        val params = synchronized(paramUpdates) { paramUpdates.values.toList().also { paramUpdates.clear() } }
        params.forEach(onFlush)
        while (true) { // Flush + handle all commands in queue
            val cmd = queue.poll() ?: break // Queue empty
            onFlush(cmd)
        }
    }
}
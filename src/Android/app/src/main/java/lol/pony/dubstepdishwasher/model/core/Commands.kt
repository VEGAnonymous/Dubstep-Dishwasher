package lol.pony.dubstepdishwasher.model.core

enum class CommandType(val value: Byte) {
    // Effect chain commands
    EFFECT_ADD(0),
    EFFECT_REMOVE(1),
    EFFECT_REORDER(2),
    EFFECT_SET_PARAMETER(3),
    EFFECT_BYPASS(4),
    EFFECT_CLEAR(5),

    // Modulation commands
    MOD_SET_PARAMETER(6),
    MOD_CLEAR_CURVE(7),
    MOD_SET_CURVE_POINT(8),
    MOD_ASSIGNMENT_ADD(9),
    MOD_ASSIGNMENT_REMOVE(10),
    MOD_ASSIGNMENT_SET(11),
    MOD_MAPPING_SET_INPUT(12),

    // Parallel internal chain command
    PARALLEL_CHAIN_COMMAND(13)
}

enum class CommandPriority {
    PRIORITY_STRUCTURE,
    PRIORITY_STATE
}

data class Command(
    val sync: Short = 0xAA55.toShort(),
    val type: CommandType,
    val id1: Int,
    val id2: Int,
    val value1: Float,
    val value2: Float = 0f,
    val value3: Float = 0f,
    val seq: Byte = 0
)

data class PendingCommand(
    val command: Command,
    var retryCount: Int = 0
)

data class CommandKey(val type: CommandType, val id1: Int, val id2: Int)

fun getCommandPriority(cmd: CommandType): CommandPriority {
    return when (cmd) {
        CommandType.EFFECT_ADD,
        CommandType.EFFECT_REMOVE,
        CommandType.EFFECT_REORDER,
        CommandType.EFFECT_CLEAR,
        CommandType.PARALLEL_CHAIN_COMMAND -> CommandPriority.PRIORITY_STRUCTURE
        else -> CommandPriority.PRIORITY_STATE
    }
}
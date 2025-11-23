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

data class Command(
    val type: CommandType,
    val id1: Int,
    val id2: Int,
    val value1: Float,
    val value2: Float = 0f,
    val value3: Float = 0f
)

data class CommandKey(val type: CommandType, val id1: Int, val id2: Int)

fun isStateCommand(cmd: CommandType): Boolean {
    return when (cmd) {
        CommandType.EFFECT_SET_PARAMETER,
        CommandType.MOD_SET_PARAMETER,
        CommandType.MOD_ASSIGNMENT_SET,
        CommandType.MOD_MAPPING_SET_INPUT -> true
        else -> false
    }
}
package lol.pony.dubstepdishwasher.model.core

import android.util.Log

enum class StatusType(val value: Byte) {
    INFERENCE(0)
}

data class Status(
    val sync: Short, // 0x55AA
    val type: StatusType,
    val id: Byte,
    val flags: Short,
    val value1: Float,
    val value2: Float,
    val value3: Float,
    val checksum: Byte
) {
    companion object {
        const val SIZE = 19 // bytes

        fun fromBytes(bytes: ByteArray): Status? {
            if (bytes.size != SIZE) return null

            val buffer = java.nio.ByteBuffer.wrap(bytes).order(java.nio.ByteOrder.LITTLE_ENDIAN)

            val sync = buffer.short
            if (sync != 0x55AA.toShort()) return null // Invalid sync

            val typeByte = buffer.get()
            val type = StatusType.entries.find { it.value == typeByte } ?: return null

            val id = buffer.get()
            val flags = buffer.short
            val value1 = buffer.float
            val value2 = buffer.float
            val value3 = buffer.float
            val checksum = buffer.get()

            val status = Status(sync, type, id, flags, value1, value2, value3, checksum)

            // Verify checksum
            if (!verifyChecksum(bytes, checksum)) return null

            return status
        }

        private fun verifyChecksum(bytes: ByteArray, expected: Byte): Boolean {
            var checksum: Byte = 0
            for (i in 0 until bytes.size - 1)
                checksum = (checksum.toInt() xor bytes[i].toInt()).toByte()
            return checksum == expected
        }
    }

    // Hacky helpers to extract packed values for INFERENCE
    fun getPercussive(): Float = ((flags.toInt() shr 8) and 0xFF) / 255.0f
    fun getSpeed(): Float = (flags.toInt() and 0xFF) / 255.0f
}
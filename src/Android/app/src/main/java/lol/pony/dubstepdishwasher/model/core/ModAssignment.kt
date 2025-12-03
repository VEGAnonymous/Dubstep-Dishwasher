package lol.pony.dubstepdishwasher.model.core

import kotlinx.serialization.Serializable

enum class ModPolarity { Unipolar, Bipolar }

@Serializable
data class ModAssignment(
    val modId: String,
    val target: ParamKey,
    var amount: Float,
    var polarity: ModPolarity = ModPolarity.Unipolar
)
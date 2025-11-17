package lol.pony.dubstepdishwasher.model.core

enum class ModPolarity { Unipolar, Bipolar }

data class ModAssignment(
    val modId: String,
    val target: ParamKey,
    var amount: Float,
    var polarity: ModPolarity = ModPolarity.Unipolar
)
package lol.pony.dubstepdishwasher.model.core

import kotlinx.serialization.Serializable

@Serializable
data class ParamKey(
    val effectId: Int,
    val paramId: Int
)
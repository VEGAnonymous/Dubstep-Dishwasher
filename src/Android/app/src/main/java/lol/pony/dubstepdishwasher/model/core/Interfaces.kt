package lol.pony.dubstepdishwasher.model.core

import kotlinx.serialization.Serializable

/* INTERFACES */

interface UIEnum { val uiName: String }

// Parameters
interface Parameter {
    val id: Int
    val name: String
    val unit: ParamUnit
    fun formatValue(): String
}

interface NumericParam { val unit: ParamUnit }

interface NormalizableParam {
    fun normalized(): Float // 0.0–1.0
    fun fromNormalized(x: Float)
}

interface ModulatableParam {
    val key: ParamKey
}

// Presets
@Serializable
sealed interface Preset<T> {
    val name: String
    val data: T
    val category: String?
    val favorite: Boolean
}

interface RandomizablePreset<T, A> : Preset<T> {
    fun randomized(args: A? = null): Preset<T>
}

// State
interface PresetContainer<T> {
    var currentPreset: Preset<T>?
}

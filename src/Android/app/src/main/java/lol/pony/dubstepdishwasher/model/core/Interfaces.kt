package lol.pony.dubstepdishwasher.model.core

/* INTERFACES */

interface UIEnum { val uiName: String }

// Parameters
interface Parameter<T> {
    val id: Int
    val name: String
    var value: T
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
interface Preset<T> {
    val name: String
    val data: T
}

// State
interface PresetContainer<T> {
    var currentPreset: Preset<T>
}
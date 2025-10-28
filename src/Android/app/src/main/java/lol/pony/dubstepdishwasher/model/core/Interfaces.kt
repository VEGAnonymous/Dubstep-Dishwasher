package lol.pony.dubstepdishwasher.model.core

/* INTERFACES */

interface NumericParam { val unit: ParamUnit }

interface NormalizableParam {
    fun normalized(): Float // 0.0–1.0
    fun fromNormalized(x: Float)
}
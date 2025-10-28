package lol.pony.dubstepdishwasher.model.core

/* INTERFACES */

public interface NumericParam { val unit: ParamUnit }

public interface NormalizableParam {
    fun normalized(): Float // 0.0–1.0
    fun fromNormalized(x: Float)
}
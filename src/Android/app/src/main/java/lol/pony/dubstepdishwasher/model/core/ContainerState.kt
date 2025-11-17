package lol.pony.dubstepdishwasher.model.core

data class EditorState(
    val snapToGrid: Boolean = false,
    val gridX: Int = 8,
    val gridY: Int = 8,
    val lockEndpoints: Boolean = false,
    override var currentPreset: Preset<List<CurvePoint>> = defaultCurvePresets()[0]
) : PresetContainer<List<CurvePoint>>
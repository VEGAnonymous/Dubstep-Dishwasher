package lol.pony.dubstepdishwasher.model.core

import kotlinx.serialization.Serializable

data class GlobalPresetState(
    override var currentPreset: Preset<GlobalPresetData>? = defaultGlobalPresets()[0] // Init
) : PresetContainer<GlobalPresetData>
@Serializable
data class EditorState(
    val snapToGrid: Boolean = false,
    val gridX: Int = 8,
    val gridY: Int = 8,
    val lockEndpoints: Boolean = false,
    override var currentPreset: Preset<List<CurvePoint>>? = defaultCurvePresets()[1] // Tri UP
) : PresetContainer<List<CurvePoint>>
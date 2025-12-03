package lol.pony.dubstepdishwasher.model.core

import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

/* GLOBAL PRESETS */

@Serializable
@SerialName("GlobalPreset")
data class GlobalPreset(
    override val name: String,
    override val data: GlobalPresetData,
    override val category: String? = null,
    override val favorite: Boolean = false
) : Preset<GlobalPresetData>

@Serializable
data class GlobalPresetData(
    val effects: List<EffectSnapshot>,
    val modulators: List<ModulatorSnapshot>,
    val assignments: List<ModAssignment>,
    val editorStates: Map<String, EditorState>,
    val parallelChains: Map<Int, ParallelChainSnapshot> = emptyMap()
)
@Serializable
data class EffectSnapshot(
    val effectType: EffectType,
    val parameters: MutableList<EffectParameter>,
    val isBypassed: Boolean
)
@Serializable
data class ModulatorSnapshot(
    val id: String,
    val isLFO: Boolean,
    val parameters: MutableList<ModulatorParameter>,
    val curve: List<CurvePoint>
)
@Serializable
data class ParallelChainSnapshot(
    val chainA: List<EffectSnapshot>,
    val chainB: List<EffectSnapshot>
)

fun defaultGlobalPresets(): List<GlobalPreset> {
    // Hack of hacks
    // Hardcode default modulators
    val defaultMods = listOf(
        Modulator.LFO("LFO1"),
        Modulator.LFO("LFO2"),
        Modulator.LFO("LFO3"),
        Modulator.LFO("LFO4"),
        Modulator.LFO("LFO5"),
        Modulator.LFO("LFO6"),
        Modulator.Mapping("Bright"),
        Modulator.Mapping("Warmth"),
        Modulator.Mapping("Intensity"),
        Modulator.Mapping("Perc"),
        Modulator.Mapping("Speed"),
        Modulator.Mapping("Expr")
    )

    val defaultModSnapshots = defaultMods.map { mod ->
        ModulatorSnapshot(
            id = mod.id,
            isLFO = mod is Modulator.LFO,
            parameters = mod.parameters,
            curve = mod.curve.map { it.copy() }
        )
    }

    // Everything else
    val data = GlobalPresetData(
        effects = emptyList(),
        modulators = defaultModSnapshots,
        assignments = emptyList(),
        editorStates = emptyMap(),
        parallelChains = emptyMap()
    )

    return listOf(GlobalPreset(name = "Init", data = data, category = "Factory"))
}

/* Curve presets */

@Serializable
data class CurveRandomArgs(
    val snapToGrid: Boolean,
    val gridX: Int,
    val gridY: Int,
    val lockEndpoints: Boolean
)

@Serializable
@SerialName("CurvePreset")
data class CurvePreset (
    override val name: String,
    override val data: List<CurvePoint>,
    override val category: String? = null,
    override val favorite: Boolean = false
) : Preset<List<CurvePoint>>, RandomizablePreset<List<CurvePoint>, CurveRandomArgs> {
    override fun randomized(args: CurveRandomArgs?): CurvePreset {

        fun r() = Math.random().toFloat()
        fun rc() = ((-100..100).random() / 100f)
        fun sx(x: Float) = snapValue(args?.snapToGrid ?: false, x, args?.gridX ?: 8)
        fun sy(y: Float) = snapValue(args?.snapToGrid ?: false, y, args?.gridY ?: 8)

        val pointCount = (3..12).random()

        // Endpoint y values
        val y0 = sy(r())
        val y1 = if (args?.lockEndpoints ?: false) y0 else sy(r())

        // Build all points
        val points = buildList(pointCount) {
            add(CurvePoint(0f, y0, rc())) // Start

            // Middle points
            repeat(pointCount - 2) {
                val rawX = r().coerceIn(0f, 1f)
                val rawY = r().coerceIn(0f, 1f)
                add(
                    CurvePoint(
                        x = sx(rawX),
                        y = sy(rawY),
                        curve = rc()
                    )
                )
            }
            add(CurvePoint(1f, y1, rc())) // End
        }
        // Sort & ensure unique X
        val sorted = points.sortedBy { it.x }.distinctBy { it.x }
        return CurvePreset(name = "Random Curve", data = sorted)
    }
}

fun defaultCurvePresets() = listOf(
    CurvePreset(name = "Sine", data = sineCurve(), category = "Factory"),
    CurvePreset(name = "Triangle UP", data = triUPCurve(), category = "Factory"),
    CurvePreset(name = "Triangle BP", data = triBPCurve(), category = "Factory"),
    CurvePreset(name = "Ramp", data = rampCurve(), category = "Factory"),
    CurvePreset(name = "Square", data = squareCurve(), category = "Factory")
)

// Default curves
fun sineCurve(): List<CurvePoint> = listOf(
    CurvePoint(0f, 0.5f, -0.5f),
    CurvePoint(0.25f, 1f, 0.5f),
    CurvePoint(0.5f, 0.5f, -0.5f),
    CurvePoint(0.75f, 0f, 0.5f),
    CurvePoint(1f, 0.5f, -0.5f)
)

fun triUPCurve(): List<CurvePoint> = listOf(
    CurvePoint(0f, 0.0f),
    CurvePoint(0.5f, 1.0f),
    CurvePoint(1f, 0.0f)
)

fun triBPCurve(): List<CurvePoint> = listOf(
    CurvePoint(0f, 0.5f),
    CurvePoint(0.25f, 1f),
    CurvePoint(0.5f, 0.5f),
    CurvePoint(0.75f, 0f),
    CurvePoint(1f, 0.5f)
)

fun rampCurve(): List<CurvePoint> = listOf(
    CurvePoint(0f, 0f),
    CurvePoint(1f, 1f)
)

fun squareCurve(): List<CurvePoint> = listOf(
    CurvePoint(0f, 1f),
    CurvePoint(0.5f, 1f),
    CurvePoint(0.5001f, 0f),
    CurvePoint(1f, 0f)
)
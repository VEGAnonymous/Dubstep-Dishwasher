package lol.pony.dubstepdishwasher.model.core

// data class ModResult(val paramKey: ParamKey, val offset: Float)

/* Handles summing modulation offsets for all modulators and targets */
object ModRouter {

    private fun applyPolarity(value: Float, polarity: ModPolarity): Float {
        return when (polarity) {
            ModPolarity.Bipolar -> value - 0.5f // [-0.5, 0.5]
            ModPolarity.Unipolar -> value // [0, 1]
        }
    }

    fun computeModulations(
        modulators: List<Modulator>,
        assignments: List<ModAssignment>,
        dt: Float
    ): Map<ParamKey, Float> {

        val modValues = mutableMapOf<String, Float>()

        // Compute and store current output for each modulator
        modulators.forEach { mod ->
            val value = when (mod) {
                is Modulator.LFO -> ModEngine.lfoValue(mod, dt)
                is Modulator.Mapping -> ModEngine.mappingValue(mod)
            }
            modValues[mod.id] = value
        }

        // Sum per-target offsets
        val offsetMap = mutableMapOf<ParamKey, Float>()

        assignments.forEach { assign ->
            var modValue = modValues[assign.modId] ?: 0f // Not targeted if null
            modValue = applyPolarity(modValue, polarity = assign.polarity) // Apply polarity
            offsetMap[assign.target] = (offsetMap[assign.target] ?: 0f) + (modValue * assign.amount) // Add contribution to total offset
        }

        return offsetMap
    }
}
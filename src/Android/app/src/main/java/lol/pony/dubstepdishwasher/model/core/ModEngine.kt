package lol.pony.dubstepdishwasher.model.core

/* Handles resolving modulation curves / algorithms to concrete values */
object ModEngine {

    /* LFO */
    fun lfoValue(lfo: Modulator.LFO, dt: Float): Float {
        val rateHz = lfo.parameters.find { it.name == "Rate" }?.value as Float
        val mode = lfo.parameters.find { it.name == "Mode" }?.value as LFOMode
        val randomMode = lfo.parameters.find { it.name == "Random" }?.value as RandomMode

        lfo.phase = (lfo.phase + (rateHz * dt)) % 1f

        return when (mode) {
            LFOMode.NORMAL -> {
                val curve = EditableCurve(lfo.curve, loop = true)
                curve.evaluate(lfo.phase)
            }
            LFOMode.RANDOM -> {
                val state = randomStates.getOrPut(lfo.id) { RandomState() }
                when (randomMode) {
                    RandomMode.PERLIN -> perlin(lfo, state)
                    RandomMode.SAMPLE_HOLD -> sampleHold(lfo, state)
                    RandomMode.BINARY -> binary(lfo, state)
                }
            }
        }
    }

    /* RANDOM */
    private val randomStates = mutableMapOf<String, RandomState>() // Store random state per LFO
    private data class RandomState(
        var lastPhase: Float = 0f,
        var currentValue: Float = 0.5f,
        var nextValue: Float = 0.5f,
        var seed: Int = kotlin.random.Random.nextInt()
    )

    private fun perlin(lfo: Modulator.LFO, state: RandomState): Float {
        // Perlin smooth noise algorithm
        if (lfo.phase < state.lastPhase) {
            state.currentValue = state.nextValue
            state.nextValue = kotlin.random.Random.nextFloat()
        }; state.lastPhase = lfo.phase

        // Smooth cosine interpolation to target
        val t = (1f - kotlin.math.cos(lfo.phase * Math.PI.toFloat())) / 2f
        return lerp(state.currentValue, state.nextValue, t)
    }

    private fun sampleHold(lfo: Modulator.LFO, state: RandomState): Float {
        // Sample & hold, update value once per cycle
        if (lfo.phase < state.lastPhase) { state.currentValue = kotlin.random.Random.nextFloat() }
        state.lastPhase = lfo.phase
        return state.currentValue
    }

    private fun binary(lfo: Modulator.LFO, state: RandomState): Float {
        // Random binary switch at each cycle
        if (lfo.phase < state.lastPhase) { state.currentValue = if (kotlin.random.Random.nextBoolean()) 1f else 0f }
        state.lastPhase = lfo.phase
        return state.currentValue
    }

    /* MAPPING */
    fun mappingValue(map: Modulator.Mapping): Float {
        val curve = EditableCurve(map.curve, loop = false)
        return curve.evaluate(map.inputValue).coerceIn(0f, 1f)
    }
}
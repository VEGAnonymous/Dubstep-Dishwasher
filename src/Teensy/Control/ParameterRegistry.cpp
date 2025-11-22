#include "Teensy/Control/ParameterRegistry.h"

ParameterMap createParameterRegistry() {
    ParameterMap registry;

    // Format {paramId, {unit, min, max, exponent, initialValue}
    // Skip discrete or boolean parameters
    
    // Chorus
    registry[EffectName::CHORUS] = {
        {0, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 1.0f}},   // MIX
        {1, {ParamUnit::HZ, 0.0f, 20.0f, 3.0f, 0.08f}},      // RATE
        {2, {ParamUnit::MS, 0.0f, 25.0f, 2.0f, 25.0f}},      // DEPTH
        {3, {ParamUnit::MS, 0.0f, 20.0f, 1.0f, 5.0f}},       // DELAY_TIME
        {4, {ParamUnit::PERCENT, -0.95f, 0.95f, 1.0f, 0.1f}} // FEEDBACK
    };

    // Compressor
    registry[EffectName::COMPRESSOR] = {
        {0, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 1.0f}},         // MIX
        {1, {ParamUnit::DB, -200.0f, 0.0f, 0.25f, -18.0f}},        // THRESHOLD
        {2, {ParamUnit::DIMENSIONLESS, 1.0f, 100.0f, 2.0f, 4.0f}}, // RATIO
        {3, {ParamUnit::DB, 0.0f, 40.0f, 2.0f, 10.0f}},            // KNEE
        {4, {ParamUnit::MS, 0.01f, 250.0f, 2.0f, 100.0f}},         // ATTACK_TIME
        {5, {ParamUnit::MS, 10.0f, 2500.0f, 3.0f, 100.0f}},        // RELEASE_TIME
        {6, {ParamUnit::DB, -72.0f, 36.0f, 1.0f, 0.0f}}            // MAKEUP_GAIN
    };

    // Delay
    registry[EffectName::DELAY] = {
        {0, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 0.5f}},   // MIX
        {1, {ParamUnit::MS, 1.0f, 500.0f, 2.0f, 200.0f}},    // DELAY_TIME
        {2, {ParamUnit::PERCENT, -0.95f, 0.95f, 1.0f, 0.4f}} // FEEDBACK
    };

    // Distortion
    registry[EffectName::DISTORTION] = {
        {0, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 0.5f}}, // MIX
        {2, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 0.25f}} // DRIVE
    };

    // Equalizer
    registry[EffectName::EQUALIZER] = {
        {0, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 1.0f}},           // MIX
        {2, {ParamUnit::HZ, 20.0f, 20000.0f, 3.0f, 200.0f}},         // BAND1_CUTOFF
        {3, {ParamUnit::DIMENSIONLESS, 0.02f, 40.0f, 2.0f, 0.707f}}, // BAND1_Q
        {4, {ParamUnit::DB, -24.0, 24.0f, 1.0f, 0.0f}},              // BAND1_GAIN
        {6, {ParamUnit::HZ, 20.0f, 20000.0f, 3.0f, 2000.0f}},        // BAND2_CUTOFF
        {7, {ParamUnit::DIMENSIONLESS, 0.02f, 40.0f, 2.0f, 0.707f}}, // BAND2_Q
        {8, {ParamUnit::DB, -24.0, 24.0f, 1.0f, 0.0f}}               // BAND2_GAIN
    };
    
    // Flanger
    registry[EffectName::FLANGER] = {
        {0, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 1.0f}},   // MIX
        {1, {ParamUnit::HZ, 0.0f, 20.0f, 3.0f, 0.08f}},      // RATE
        {2, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 1.0f}},   // DEPTH
        {3, {ParamUnit::PERCENT, -0.95f, 0.95f, 1.0f, 0.5f}} // FEEDBACK
    };
    
    // FormantShifter
    registry[EffectName::FORMANT_SHIFTER] = {
        {0, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 1.0f}},      // MIX
        {2, {ParamUnit::SEMITONES, -12.0f, 12.0f, 1.0f, 0.0f}}, // FORMANT_SHIFT
        {3, {ParamUnit::DIMENSIONLESS, 0, 16, 1.0f, 16}}        // ENVELOPE_WIDTH
    };

    // Freezer
    registry[EffectName::FREEZER] = {
        {0, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 1.0f}}, // MIX
        {1, {ParamUnit::HZ, -4.0f, 4.0f, 1.0f, 1.0f}},     // RATE
        {4, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 0.0f}}, // LOOP_START
        {5, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 1.0f}}  // LOOP_END
    };

    // Gain
    registry[EffectName::GAIN] = {
        {0, {ParamUnit::DB, -60.0f, 24.0f, 1.0f, 0.0f}}, // GAIN
    };

    // Gate
    registry[EffectName::GATE] = {
        {0, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 1.0f}},   // MIX
        {1, {ParamUnit::DB, -100.0f, 20.0f, 0.33f, -18.0f}}, // THRESHOLD
        {2, {ParamUnit::MS, 0.01f, 250.0f, 2.0f, 25.0f}},    // ATTACK_TIME
        {3, {ParamUnit::MS, 0.01f, 1500.0f, 3.0f, 25.0f}},   // RELEASE_TIME
        {4, {ParamUnit::MS, 1.0f, 1500.0f, 3.0f, 50.0f}}     // HOLD_TIME
    };

    // Granulator
    registry[EffectName::GRANULATOR] = {
        {0, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 1.0f}},       // MIX
        {1, {ParamUnit::DIMENSIONLESS, 0.0f, 1.0f, 1.0f, 0.5f}}, // POSITION
        {2, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 0.5f}},       // POSITION_RAND
        {3, {ParamUnit::MS, 1.0, 500.0f, 2.0f, 50.0f}},          // RATE
        {4, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 0.0f}},       // RATE_RAND
        {5, {ParamUnit::MS, 5.0f, 500.0f, 2.0f, 200.0f}},        // LENGTH
        {6, {ParamUnit::PERCENT, 0.0, 1.0f, 1.0f, 0.0f}},        // LENGTH_RAND
        {7, {ParamUnit::SEMITONES, -24.0f, 24.0f, 1.0f, 0.0f}},  // TUNE
        {8, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 0.0f}},       // TUNE_RAND
        {9, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 0.8f}},       // LEVEL
        {10, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 0.0f}},      // LEVEL_RAND
        {11, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 0.0f}},      // REVERSE_CHANCE
    };

    // Modulation
    registry[EffectName::MODULATION] = {
        {0, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 1.0f}}, // MIX
        {3, {ParamUnit::HZ, 1.0f, 2000.0f, 4.0f, 5.0f}},   // FREQ
        {4, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 0.5f}}, // DEPTH
        {5, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 0.0f}}, // BIAS
        {6, {ParamUnit::PERCENT, -1.0f, 1.0f, 1.0f, 0.0f}} // RECTIFY
    };

    // Parallel
    registry[EffectName::PARALLEL] = {
        {0, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 1.0f}} // MIX
        // TBD
    };

    // Phaser
    registry[EffectName::PHASER] = {
        {0, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 1.0f}},   // MIX
        {1, {ParamUnit::HZ, 0.0f, 20.0f, 3.0f, 0.08f}},      // RATE
        {2, {ParamUnit::HZ, 50.0f, 8000.0f, 2.0f, 600.0f}},  // CENTER_FREQ
        {3, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 1.0f}},   // SPREAD
        {4, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 0.5f}},   // DEPTH
        {5, {ParamUnit::PERCENT, -0.95f, 0.95f, 1.0f, 0.8f}} // FEEDBACK
    };

    // PitchShifter
    registry[EffectName::PITCH_SHIFTER] = {
        {0, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 1.0f}},      // MIX
        {1, {ParamUnit::SEMITONES, -24.0f, 24.0f, 1.0f, 0.0f}}, // PITCH_SHIFT
        {2, {ParamUnit::MS, 20.0f, 500.0f, 2.0f, 200.0f}},      // GRAIN_SIZE
        {3, {ParamUnit::PERCENT, 0.25f, 0.75f, 1.0f, 0.5f}},    // GRAIN_OVERLAP
        {4, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 0.0f}}       // JITTER
    };
 
    // Reverb
    registry[EffectName::REVERB] = {
        {0, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 1.0f}},     // MIX
        {1, {ParamUnit::MS, 0.0f, 100.0f, 2.0f, 0.0f}},        // PREDELAY_TIME
        {2, {ParamUnit::MS, 100.0f, 10000.0f, 3.0f, 3000.0f}}, // DECAY_TIME
        {3, {ParamUnit::HZ, 0.05f, 5.0f, 2.0f, 0.5f}},         // MOD_RATE
        {4, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 0.2f}}      // MOD_DEPTH
    };

    // Scrubby
    registry[EffectName::SCRUBBY] = {
        {0, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 1.0f}},  // MIX
        {1, {ParamUnit::HZ, 0.3f, 810.0f, 2.5f, 9.0f}},     // SEEK_RATE_LOW
        {2, {ParamUnit::HZ, 0.3f, 810.0f, 2.5f, 9.0f}},     // SEEK_RATE_HIGH
        {3, {ParamUnit::MS, 0.3, 6000.0f, 3.5f, 333.0f}},   // SEEK_RANGE
        {4, {ParamUnit::PERCENT, 0.03f, 1.0f, 1.0f, 1.0f}}, // SEEK_DUR_LOW
        {5, {ParamUnit::PERCENT, 0.03f, 1.0f, 1.0f, 1.0f}}, // SEEK_DUR_HIGH
        {6, {ParamUnit::DIMENSIONLESS, -4, 0, 1.0f, -4}},   // OCTAVES_DOWN
        {7, {ParamUnit::DIMENSIONLESS, 0, 8, 1.0f, 8}}      // OCTAVES_UP
    };

    // SpectralGate
    registry[EffectName::SPECTRAL_GATE] = {
        {0, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 1.0f}},        // MIX
        {2, {ParamUnit::DB, -100.0f, 0.0f, 0.33f, -10.0f}},       // THRESHOLD
        {3, {ParamUnit::DIMENSIONLESS, -1.0f, 1.0f, 1.0f, 0.5f}}, // TILT
    };

    // Vocoder
    registry[EffectName::VOCODER] = {
        {0, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 1.0f}},     // MIX
        {1, {ParamUnit::DIMENSIONLESS, 4, 20, 1.0f, 10}},      // N_BANDS
        {2, {ParamUnit::HZ, 10.0f, 16000.0f, 4.0f, 80.0f}},    // LOW_FREQ
        {3, {ParamUnit::HZ, 10.0f, 16000.0f, 4.0f, 12000.0f}}, // HIGH_FREQ
        {4, {ParamUnit::PERCENT, 0.05f, 4.0f, 1.0f, 0.5f}},    // BANDWIDTH
        {5, {ParamUnit::PERCENT, 0.0f, 2.0f, 1.0f, 1.0f}},     // DEPTH
        {6, {ParamUnit::MS, 10.0f, 2000.0f, 2.5f, 2.0f}},      // ATTACK_TIME
        {7, {ParamUnit::MS, 10.0f, 2000.0f, 2.5f, 35.0f}}      // RELEASE_TIME
    };

    // Wah
    registry[EffectName::WAH] = {
        {0, {ParamUnit::PERCENT, 0.0f, 1.0f, 1.0f, 1.0f}},       // MIX
        {1, {ParamUnit::HZ, 20.0f, 1000.0f, 2.0f, 350.0f}},      // MIN_FREQ
        {2, {ParamUnit::HZ, 1000.0f, 8000.0f, 2.0f, 2500.0f}},   // MAX_FREQ
        {3, {ParamUnit::DIMENSIONLESS, 0.3f, 6.0f, 2.0f, 1.6f}}, // Q
    };

    return registry;
}

static ParameterMap parameterRegistry = createParameterRegistry(); // Global instance

const ParameterRange* getParameterRange(EffectName effect, ParamID param) {
    auto effectIt = parameterRegistry.find(effect);
    if (effectIt == parameterRegistry.end()) return nullptr;
    
    auto paramIt = effectIt->second.find(param);
    if (paramIt == effectIt->second.end()) return nullptr;
    
    return &paramIt->second;
}
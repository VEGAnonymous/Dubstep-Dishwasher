#pragma once

#include "Teensy/Defines.h"

#include "arm_math.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

inline float dbAmp(float dB) { return pow(10.0f, dB / 20.0f); }
inline float ampDB(float amp) { return 20.0f * log10(amp + 1e-12); }

inline float msSamples(float ms) { return ms * SAMPLE_RATE / 1000.0f; }

inline float uniform() { // Random float [-1, 1]
    static thread_local std::mt19937 rng(std::random_device{}());
    static thread_local std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    return dist(rng);
}

inline float getEnvelopeValue(float t, EnvelopeType type) {
    // https://www.desmos.com/calculator/j7vhnwaylq
    switch (type) {
        case EnvelopeType::HANN: return 0.5f * (1.0f - arm_cos_f32(2.0f * static_cast<float>(M_PI) * t));
        case EnvelopeType::HAMMING: return 0.54f - (0.46f * arm_cos_f32(2.0f * static_cast<float>(M_PI) * t));
        case EnvelopeType::SINE: return arm_sin_f32(static_cast<float>(M_PI) * t);
        case EnvelopeType::TRI: return 1.0f - fabsf(2.0f * t - 1.0f);
        case EnvelopeType::PERC: {
            constexpr float attack = 0.03f; constexpr float decay = 6.0f;
            if (t < attack) { return t / attack; // Linear attack
            } else { // Exponential decay
                float t_decay = (t - attack) / (1.0f - attack); 
                return expf(-decay * t_decay);
            }
        }
        case EnvelopeType::SMOOTH_RECT: {
            constexpr float smooth = 0.05f;
            if (t < smooth) { // Fade in
                return 0.5f * (1.0f - arm_cos_f32(static_cast<float>(M_PI) * t / smooth)); 
            } else if (t >= (1.0f - smooth)) { // Fade out
                return 0.5f * (1.0f - arm_cos_f32(static_cast<float>(M_PI) * (1.0f - t) / smooth)); 
            } else { return 1.0f; }
        }
        default: return 1.0f;
    }
}

template <typename T>
inline float lerp(const T& buffer, float index, size_t size) { // Linearly interpolate buffer indices
    if (index < 0) index += size;
    size_t i0 = (static_cast<size_t>(floor(index))) % size;
    size_t i1 = (i0 + 1) % size;
    float frac = index - floor(index);
    return buffer[i0] + (frac * (buffer[i1] - buffer[i0]));
}

inline float lerp(float a, float b, float t) { return a + (t * (b - a)); } // Linearly interpolate scalars

inline float scale(float value, float inLow, float inHigh, float outLow, float outHigh, float exponent = 1.0f) { // Map value in input range to output range
    float t = std::clamp((value - inLow) / (inHigh - inLow), 0.0f, 1.0f); // Normalize to [0, 1]
    if (exponent != 1.0f) t = powf(t, exponent); // Apply exponential
    return lerp(outLow, outHigh, t);
}

inline float dryWetMix(float dry, float wet, float mix, bool lin = true) {
    if (mix == 1.0f) return wet;
    else if (mix == 0.0f) return dry;
    else if (lin) return lerp(dry, wet, mix); // Linear mix
    else return (dry * arm_cos_f32(mix * M_PI_2)) + (wet * arm_sin_f32(mix * M_PI_2)); // Equal power crossfade
}

inline void overlapAdd(std::vector<float>& target, const std::vector<float>& frame, EnvelopeType type, size_t startPos = 0) {
    const size_t N = frame.size();
    for (size_t i = 0; i < N; ++i) {
        size_t pos = (startPos + i) % target.size();
        target[pos] += frame[i] * getEnvelopeValue((float)i / N, type);
    }
}
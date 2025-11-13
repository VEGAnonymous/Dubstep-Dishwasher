#pragma once

#include "Teensy/Modules/Effect.h"
#include "Teensy/Defines.h"

class FIR_Filter : public Effect {
    private:
        enum Params : ParamID { MIX };

        float mix;
        const float* h; // Pointer to filter kernel (stored in PROGMEM)
        size_t M; // Filter order
        std::vector<float> z; // Circular double buffer (filter state)
        size_t z_i = 0; // State pointer
        
    public:
        FIR_Filter(float mix, const float* h, size_t M);
        ~FIR_Filter() = default;

        void setMix(float mix); // [0.0, 1.0]
        void setParam(ParamID param, float value) override;

        void process(const float* in, float* out, size_t n) override;
};
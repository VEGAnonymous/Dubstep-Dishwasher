#pragma once

#include "Teensy/Modules/Biquad.h"

class BPF_Biquad : public Biquad {
    private:
        bool flatGain; 

        void updateCoeffs() override;
    public:
        BPF_Biquad(float cutoff = 1000.0f, float q = 0.707f, float gainDB = 0.0f, bool flatGain = false);
        ~BPF_Biquad() = default;
};
#pragma once

#include "Teensy/Modules/Biquad.h"

class HPF_Biquad : public Biquad {
    private:
        void updateCoeffs() override;
    public:
        HPF_Biquad(float cutoff = 1000.0f, float q = 0.707f, float gainDB = 0.0f);
        ~HPF_Biquad() = default;
};
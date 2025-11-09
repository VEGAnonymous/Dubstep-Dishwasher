#pragma once

#include "Teensy/Modules/Biquad.h"

class Peak_Biquad : public Biquad {
    private:
        void updateCoeffs() override;
    public:
        void setGain(float gainDB);

        Peak_Biquad(float cutoff = 1000.0f, float q = 0.707f, float gainDB = 0.0f);
        ~Peak_Biquad() = default;
};
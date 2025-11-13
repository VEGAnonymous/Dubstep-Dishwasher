#pragma once

#include "Teensy/Modules/Biquad.h"

class Notch_Biquad : public Biquad {
    private:
        void updateCoeffs() override;
    public:

        Notch_Biquad(float cutoff = 1000.0f, float q = 0.707f, float gainDB = 0.0f);
        ~Notch_Biquad() = default;
};
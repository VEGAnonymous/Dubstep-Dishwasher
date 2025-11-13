#pragma once

#include "Teensy/Modules/Biquad.h"

class HighShelf_Biquad : public Biquad {
    private:
        void updateCoeffs() override;
    public:
        void setGain(float gainDB) override;

        HighShelf_Biquad(float cutoff = 1000.0f, float q = 0.707f, float gainDB = 0.0f);
        ~HighShelf_Biquad() = default;
};
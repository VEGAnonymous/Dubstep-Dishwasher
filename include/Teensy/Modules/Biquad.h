#pragma once

#include "Teensy/Modules/IIR_Filter.h"

class Biquad : public IIR_Filter { // Generic SOS form, Direct Form II-Transpose
    protected:
        enum Params : ParamID { CUTOFF = 1, Q, GAIN };

        float cutoff = 1000.0f, q = 0.707f, gainDB = 0.0f;

        // Biquad coefficients
        float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;

        // Filter state
        float z1 = 0.0f, z2 = 0.0f;

        float LCCDE(float x);

        virtual void updateCoeffs() = 0; // Subclasses must implement

    public:
        void setCutoff(float cutoff); // Hz, [20.0, 20000.0]
        void setQ(float q); // [0.025, 40.0]
        virtual void setGain(float gainDB); // dB, [-24.0, 24.0]
        
        void setParam(ParamID param, float value);
        float getParam(ParamID param) const override;
};
#pragma once

#include "Teensy/Modules/Effect.h"
#include "Teensy/Filters/FIR_Filter.h"

class Distortion : public Effect {
    private:
        enum Params : ParamID { MIX, MODE, DRIVE, ENABLE_AAF };

        DistortionMode mode;
        float mix, drive;
        bool enableAAF;

        float (Distortion::*algorithm)(float, float) = nullptr; // Function pointer for distortion algorithm to use
        FIR_Filter antiAlias;

        // Distortion algorithms
        float tube(float in, float drive);
        float softClip(float in, float drive);
        float hardClip(float in, float drive);
        float diode(float in, float drive);
        float bitCrush(float in, float drive);
        float rectify(float in, float drive);
        float saturate(float in, float drive);

    public:
        Distortion(float mix = 1.0f, DistortionMode mode = DistortionMode::TUBE, float drive = 0.25f, bool enableAAF = false);

        void setMix(float mix); // [0.0, 1.0]
        void setMode(DistortionMode mode);
        void setDrive(float drive); // [0.0, 1.0]
        void setAAF(bool enableAAF); 
        void setParam(ParamID param, float value) override;

        void process(const float* in, float* out, size_t n) override;
};
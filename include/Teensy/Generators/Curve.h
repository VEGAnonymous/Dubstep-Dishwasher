#pragma once

#include "Teensy/Modules/Generator.h"
#include "Teensy/Defines.h"

class Curve : public Generator {
    private:
        std::vector<CurvePoint> points;
        float freq, phase; bool loop;
        
        inline float applyCurve(float t, float curveAmount) const; // Apply exponential curve to interpolation parameter
        float evaluateCurve(float x) const; // Evaluate curve at given x position

    public:
        friend class Modulator;

        Curve(float freq = 1.0f, bool loop = true);
        
        void setFreq(float freq);
        float getFreq() const;
        void setPhase(float phase);
        float getPhase() const;
        void setCurve(const std::vector<CurvePoint>& points);
        void setCurvePoint(float x, float y, float curve);
        void clearCurve();
        const std::vector<CurvePoint>& getCurve() const;
        
        float evaluate();
        float next() override;

        void printCurve() const;
};
#pragma once

#include "Teensy/Defines.h"
#include "Teensy/Generators/Curve.h"
#include "Teensy/Generators/Random.h"

#include <memory>
#include <map>

class Modulator {
    private:
        enum ModulatorParams : ParamID { RATE, MODE, RANDOM_MODE };

        ModulatorID id;
        ModulatorType type;
        std::map<ParamID, float> parameters;

        // Instances
        std::unique_ptr<Curve> lfoCurve;
        std::unique_ptr<Random> lfoRandom;
        
        // Mapping
        float mappingInput;
        std::unique_ptr<Curve> mappingCurve;
        
        // Cached
        float output;

    public:
        friend class ModulationEngine;

        Modulator(ModulatorID id, ModulatorType type);
        
        ModulatorID getID() const;
        ModulatorType getType() const;
        
        void setParam(ParamID param, float value);
        float getParam(ParamID param) const;
        
        void setCurve(const std::vector<CurvePoint>& points);
        void setCurvePoint(size_t index, float x, float y, float curve);
        void clearCurve();
        
        void setPhase(float phase);
        float getPhase() const;
        
        void setMappingInput(float input);
        
        float compute(float dt);
        
        float getOutput() const;
};
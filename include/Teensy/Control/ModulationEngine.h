#pragma once

#include "Teensy/Control/AudioChain.h"
#include "Teensy/Modules/Modulator.h"
#include "Teensy/Defines.h"

#include <array>
#include <vector>
#include <map>

class ModulationEngine {
    private:
        std::array<std::unique_ptr<Modulator>, 12> modulators; // Fixed 12 modulators
        std::vector<ModAssignment> assignments;
        AudioChain& audioChain;
        
        std::map<ParamKey, float> baseValues;
        
        float lastUpdateTime; // dt

    public:
        ModulationEngine(AudioChain& chain);
        
        Modulator* getModulator(ModulatorID id);

        void setMode(ModulatorID modId, ModulatorType mode);
        void addAssignment(const ModAssignment& assignment);
        void removeAssignment(ModulatorID modId, EffectID effectId, ParamID paramId);
        void setAssignment(ModulatorID modId, EffectID effectId, ParamID paramId, float amount, ModPolarity polarity);
        void clearAssignments();
        void setBaseValue(EffectID effectId, ParamID paramId, float value);
        void setMappingInput(ModulatorID id, float input);
        
        void update(float dt);
};
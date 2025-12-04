#include "Teensy/Control/ModulationEngine.h"
#include "Teensy/Control/ParameterRegistry.h"

/* PRIVATE */

/*

std::array<std::unique_ptr<Modulator>, 12> modulators;
std::vector<ModAssignment> assignments;
AudioChain& audioChain;

std::map<ParamKey, float> baseValues;

float lastUpdateTime; // dt

*/

/* PUBLIC */

ModulationEngine::ModulationEngine(AudioChain& chain) : audioChain(chain), lastUpdateTime(0.0f) {
    // Initialize 12 modulators: 6 LFO/Random, 6 Mapping
    for (size_t i = 0; i < 6; ++i) modulators[i] = std::make_unique<Modulator>(i, ModulatorType::LFO_CURVE);
    for (size_t i = 6; i < 12; ++i) modulators[i] = std::make_unique<Modulator>(i, ModulatorType::MAPPING);
}

Modulator* ModulationEngine::getModulator(ModulatorID id) { return modulators[id].get(); }

void ModulationEngine::addAssignment(const ModAssignment& assignment) {
    assignments.push_back(assignment);
    
    ParamKey key{assignment.effectId, assignment.paramId};
    if (baseValues.find(key) == baseValues.end()) {
        Effect* effect = audioChain.getEffect(assignment.effectId);
        if (effect) {
            // Only store base value if parameter is modulatable
            const ParameterRange* range = getParameterRange(effect->getEffectName(), assignment.paramId);
            if (range) baseValues[key] = effect->getNormalized(assignment.paramId);
        }
    }
}

void ModulationEngine::removeAssignment(ModulatorID modId, EffectID effectId, ParamID paramId) {
    assignments.erase(
        std::remove_if(assignments.begin(), assignments.end(),
            [modId, effectId, paramId](const ModAssignment& a) {
                return a.modId == modId && a.effectId == effectId && a.paramId == paramId;
            }),
        assignments.end()
    );
    
    // Check if parameter has other assignments
    ParamKey key{effectId, paramId};
    bool modulated = std::any_of(assignments.begin(), assignments.end(),
        [&key](const ModAssignment& a) { return a.effectId == key.effectId && a.paramId == key.paramId; });
    
    // If not, restore base value
    if (!modulated) {
        Effect* effect = audioChain.getEffect(effectId);
        if (effect && baseValues.count(key)) { effect->setNormalized(paramId, baseValues[key]); }
    }
}

void ModulationEngine::removeEffect(EffectID effectId) { // Remove all assignments / base values for effect
    assignments.erase(
        std::remove_if(assignments.begin(), assignments.end(),
            [effectId](const ModAssignment& a) {
                return a.effectId == effectId;
            }),
        assignments.end()
    );

    for (auto it = baseValues.begin(); it != baseValues.end(); ) {
        if (it->first.effectId == effectId) it = baseValues.erase(it);
        else ++it;
    }
}

void ModulationEngine::setAssignment(ModulatorID modId, EffectID effectId, ParamID paramId, float amount, ModPolarity polarity) {
    for (auto& assignment : assignments) {
        if (assignment.modId == modId && assignment.effectId == effectId && assignment.paramId == paramId) {
            assignment.amount = amount; 
            assignment.polarity = polarity;
            return;
        }
    }
    addAssignment(ModAssignment(modId, effectId, paramId, amount, polarity)); // Not found, add new
}

void ModulationEngine::clearAssignments() { 
    assignments.clear(); 
    for (auto& modulator : modulators) if (modulator) modulator->output = 0.0f;
}

void ModulationEngine::setBaseValue(EffectID effectId, ParamID paramId, float normalized) {
    Effect* effect = audioChain.getEffect(effectId);
    if (!effect) return;

    const ParameterRange* range = getParameterRange(effect->getEffectName(), paramId);
    if (!range) return; // Skip discrete/boolean parameters

    ParamKey key{effectId, paramId};
    baseValues[key] = normalized;
    if (effect) effect->setNormalized(paramId, normalized);
}

void ModulationEngine::setMappingInput(ModulatorID id, float input) {
    if (id >= 12) return;
    Modulator* mod = modulators[id].get();
    if (mod && (mod->getType() == ModulatorType::MAPPING)) mod->setMappingInput(input);
}

void ModulationEngine::update(float dt) { // Update for LFOs
    if (assignments.empty()) return;
    
    // Compute all modulator outputs
    for (auto& modulator : modulators) if (modulator) modulator->compute(dt);
    
    // Sum modulation offsets per target parameter
    std::map<ParamKey, float> offsets;
    for (const auto& assignment : assignments) {
        Modulator* mod = getModulator(assignment.modId);
        if (!mod) continue;
        
        float modValue = mod->getOutput(); // [0, 1]
        
        // Apply polarity
        float offset = 0.0f;
        switch (assignment.polarity) {
            case ModPolarity::UNIPOLAR: offset = modValue; break; // [0, 1]
            case ModPolarity::BIPOLAR: offset = modValue - 0.5f; break; // [-0.5, 0.5]
        }
        offset *= assignment.amount; // Scale by amount
        
        // Accumulate
        ParamKey key{assignment.effectId, assignment.paramId};
        offsets[key] += offset;
    }
    
    // Apply offsets to effect parameters (normalized)
    for (const auto& [key, offset] : offsets) {
        Effect* effect = audioChain.getEffect(key.effectId);
        if (!effect) continue;
        
        float base = baseValues[key];
        float effective = std::clamp(base + offset, 0.0f, 1.0f); // [0, 1]
        
        effect->setNormalized(key.paramId, effective);
    }
}
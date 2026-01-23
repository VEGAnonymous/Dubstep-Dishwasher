#include "Teensy/Modules/Modulator.h"

/* PRIVATE */

/*

ModulatorID id;
ModulatorType type;
std::map<ParamID, float> parameters;

// Instances
std::unique_ptr<Curve> lfoCurve;
std::unique_ptr<Random> lfoRandom;

// Mapping
float mappingInput, targetMappingInput;
std::unique_ptr<Curve> mappingCurve;

// Cached
float output;

*/

/* PUBLIC */

Modulator::Modulator(ModulatorID id, ModulatorType type) : id(id), type(type), mappingInput(0.0f), output(0.0f) {
    switch (type) {
        case ModulatorType::LFO_CURVE:
        case ModulatorType::LFO_RANDOM:
            // Defaults
            parameters[RATE] = 0.621f;
            parameters[MODE] = static_cast<float>(ModulatorType::LFO_CURVE);
            parameters[RANDOM_MODE] = static_cast<float>(RandomMode::PERLIN);
            // Instantiate both modes
            lfoCurve = std::make_unique<Curve>(parameters[RATE], true);
            lfoRandom = std::make_unique<Random>(parameters[RATE], static_cast<RandomMode>(parameters[RANDOM_MODE]));
            break; 
        case ModulatorType::MAPPING:
            mappingCurve = std::make_unique<Curve>(0.621f, false);
            mappingCurve->clearCurve();
            mappingCurve->setCurvePoint(1.0f, 1.0f, 0.0f); // Init to ramp
            mappingInput = 0.0f; targetMappingInput = 0.0f;
            break;
    }
}

ModulatorID Modulator::getID() const { return id; }
ModulatorType Modulator::getType() const { return type; }

void Modulator::setParam(ParamID param, float value) {
    parameters[param] = value;
    switch (type) {
        case ModulatorType::LFO_CURVE:
        case ModulatorType::LFO_RANDOM:
            switch (param) {
                case RATE:
                    if (lfoCurve) { lfoCurve->setFreq(value); } 
                    if (lfoRandom) { lfoRandom->setFreq(value); }
                    break;
                case ModulatorParams::MODE: type = static_cast<ModulatorType>(value); break;
                case RANDOM_MODE: if (lfoRandom) { lfoRandom->setMode(static_cast<RandomMode>(value)); } break;
                default: break;
            } break;
        case ModulatorType::MAPPING: break;
    }
}

float Modulator::getParam(ParamID param) const {
    auto it = parameters.find(param);
    return (it != parameters.end()) ? it->second : 0.0f;
}

void Modulator::setCurve(const std::vector<CurvePoint>& points) {
    switch (type) {
        case ModulatorType::LFO_CURVE: if (lfoCurve) { lfoCurve->setCurve(points); } break;
        case ModulatorType::MAPPING: if (mappingCurve) { mappingCurve->setCurve(points); } break;
        default: break;
    }
}

void Modulator::setCurvePoint(float x, float y, float curve) {
    switch (type) {
        case ModulatorType::LFO_CURVE: if (lfoCurve) { lfoCurve->setCurvePoint(x, y, curve); } break;
        case ModulatorType::MAPPING: if (mappingCurve) { mappingCurve->setCurvePoint(x, y, curve); } break;
        default: break;
    }
}

void Modulator::clearCurve() {
    switch (type) {
        case ModulatorType::LFO_CURVE: if (lfoCurve) { lfoCurve->clearCurve(); } break;
        case ModulatorType::MAPPING: if (mappingCurve) { mappingCurve->clearCurve(); } break;
        default: break;
    }
}

void Modulator::setPhase(float phase) {
    if (lfoCurve) lfoCurve->setPhase(phase);
    if (lfoRandom) lfoRandom->setPhase(phase);
}

float Modulator::getPhase() const {
    if (lfoCurve) return lfoCurve->getPhase();
    if (lfoRandom) return lfoRandom->getPhase();
    return 0.0f;
}

void Modulator::setMappingInput(float input) { targetMappingInput = std::clamp(input, 0.0f, 1.0f); }

float Modulator::compute(float dt) { // Compute current output value
    switch (type) {
        case ModulatorType::LFO_CURVE: {
            if (lfoCurve) { /* LFO */
                // Evaluate at current phase
                auto& points = lfoCurve->getCurve();
                if (points.size() >= 2) {
                    output = std::clamp(lfoCurve->next(), 0.0f, 1.0f);
                
                    float freq = lfoCurve->getFreq();
                    float phase = lfoCurve->getPhase();
                    
                    output = lfoCurve->evaluateCurve(phase);
                    phase += freq * dt; // Advance by dt
                    if (phase >= 1.0f) phase -= 1.0f;
                    lfoCurve->setPhase(phase);
                }
            } break;
        }
        case ModulatorType::LFO_RANDOM: {
            if (lfoRandom) {
                output = lfoRandom->tick(dt); 
                output = std::clamp((output + 1.0f) * 0.5f, 0.0f, 1.0f);
            } break;
        }
        case ModulatorType::MAPPING: {
            if (mappingCurve) {
                // Smooth mapping input
                constexpr float smoothTime = 0.5f;
                const float alpha = 1.0f - expf(-dt / smoothTime); // 500ms time constant
                if (fabsf(mappingInput - targetMappingInput) > 1e-4f) {
                     mappingInput = (alpha * targetMappingInput) + ((1.0f - alpha) * mappingInput); // 1st order LPF
                } else mappingInput = targetMappingInput;

                // Evaluate curve at mapping input
                mappingCurve->setPhase(mappingInput);
                output = std::clamp(mappingCurve->evaluate(), 0.0f, 1.0f);
            } break;
        }
    } return output;
}

float Modulator::getOutput() const { return output; }


/* // Debug
void Modulator::printState() const {
    Serial.printf("Modulator %d [Type: %d]\n", id, static_cast<int>(type));
    Serial.printf("  Output: %.3f\n", output);
    
    for (const auto& [paramId, value] : parameters) {
        Serial.printf("  Param %d: %.3f\n", paramId, value);
    }
    
    if (type == ModulatorType::LFO_CURVE && lfoCurve) {
        Serial.println("  LFO Curve:");
        lfoCurve->printCurve();
    }
    
    if (type == ModulatorType::LFO_RANDOM && lfoRandom) {
        Serial.printf("  Random: freq=%.3f, phase=%.3f\n", 
                        lfoRandom->getFreq(), lfoRandom->getPhase());
    }
    
    if (type == ModulatorType::MAPPING && mappingCurve) {
        Serial.println("  Mapping Curve:");
        mappingCurve->printCurve();
        Serial.printf("  Input: %.3f\n", mappingInput);
    }
        
}
*/
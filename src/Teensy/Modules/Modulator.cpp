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
float mappingInput;
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
                    else if (lfoRandom) { lfoRandom->setFreq(value); }
                    break;
                case ModulatorParams::MODE: break; // Already set parameters[MODE]
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

void Modulator::setCurvePoint(size_t index, float x, float y, float curve) {
    switch (type) {
        case ModulatorType::LFO_CURVE: if (lfoCurve) { lfoCurve->setCurvePoint(index, x, y, curve); } break;
        case ModulatorType::MAPPING: if (mappingCurve) { mappingCurve->setCurvePoint(index, x, y, curve); } break;
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

void Modulator::setMappingInput(float input) { mappingInput = std::clamp(input, 0.0f, 1.0f); }

float Modulator::compute(float dt) { // Compute current output value
    switch (type) {
        case ModulatorType::LFO_CURVE:
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
        case ModulatorType::LFO_RANDOM:   
            if (lfoRandom) { /* RANDOM */
                output = lfoRandom->next();
                // Normalize to [0.0, 1.0]
                output = (output + 1.0f) * 0.5f;
                output = std::clamp(output, 0.0f, 1.0f);
                
                // Adjust phase for dt
                float phase = lfoRandom->getPhase();
                float freq = getParam(RATE);

                phase = phase - (1.0f / SAMPLE_RATE) + (freq * dt);
                while (phase >= 1.0f) phase -= 1.0f; // Wrap
                while (phase < 0.0f) phase += 1.0f;
                lfoRandom->setPhase(phase);
            } break;
        case ModulatorType::MAPPING:
            if (mappingCurve) {
                // Evaluate curve at mapping input
                mappingCurve->setPhase(mappingInput);
                output = std::clamp(mappingCurve->next(), 0.0f, 1.0f);
            } break;
    } return output;
}

float Modulator::getOutput() const { return output; }
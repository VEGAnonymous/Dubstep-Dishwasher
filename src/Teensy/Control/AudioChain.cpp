#include "Teensy/Control/AudioChain.h"

#include "Teensy/Effects/Gain.h"
#include "Teensy/Effects/Parallel.h"
#include "Teensy/Effects/Modulation.h"
#include "Teensy/Effects/Wah.h"
#include "Teensy/Effects/Distortion.h"
#include "Teensy/Effects/Delay.h"
#include "Teensy/Effects/Flanger.h"
#include "Teensy/Effects/Phaser.h"
#include "Teensy/Effects/Chorus.h"
#include "Teensy/Effects/Reverb.h"
#include "Teensy/Effects/Gate.h"
#include "Teensy/Effects/Compressor.h"
#include "Teensy/Effects/Equalizer.h"
#include "Teensy/Effects/Granulator.h"
#include "Teensy/Effects/Freezer.h"
#include "Teensy/Effects/Scrubby.h"
#include "Teensy/Effects/PitchShifter.h"
#include "Teensy/Effects/Vocoder.h"
#include "Teensy/Effects/SpectralGate.h"
#include "Teensy/Effects/FormantShifter.h"

#include <Audio.h>

/* PRIVATE */

/*

std::vector<std::unique_ptr<Effect>> effects; // Effect chain
std::map<EffectID, size_t> fxMap;
EffectID nextID = 0; std::queue<EffectID> freeIDs;

std::map<EffectName, std::function<std::unique_ptr<Effect>()>> effectInits; // Function pointers (factories) to instantiate effects

*/

/* PUBLIC */

AudioChain::AudioChain() { 
    effectInits = { // Factories
        {EffectName::CHORUS, [](){ return std::make_unique<Chorus>(); }},
        {EffectName::COMPRESSOR, [](){ return std::make_unique<Compressor>(); }},
        {EffectName::DELAY, [](){ return std::make_unique<Delay>(); }},
        {EffectName::DISTORTION, [](){ return std::make_unique<Distortion>(); }},
        {EffectName::EQUALIZER, [](){ return std::make_unique<Equalizer>(); }},
        {EffectName::FLANGER, [](){ return std::make_unique<Flanger>(); }},
        {EffectName::FORMANT_SHIFTER, [](){ return std::make_unique<FormantShifter>(); }},
        {EffectName::FREEZER, [](){ return std::make_unique<Freezer>(); }},
        {EffectName::GAIN, [](){ return std::make_unique<Gain>(); }},
        {EffectName::GATE, [](){ return std::make_unique<Gate>(); }},
        {EffectName::GRANULATOR, [](){ return std::make_unique<Granulator>(); }},
        {EffectName::MODULATION, [](){ return std::make_unique<Modulation>(); }},
        {EffectName::PARALLEL, [](){ return std::make_unique<Parallel>(); }},
        {EffectName::PHASER, [](){ return std::make_unique<Phaser>(); }},
        {EffectName::PITCH_SHIFTER, [](){ return std::make_unique<PitchShifter>(); }},
        {EffectName::REVERB, [](){ return std::make_unique<Reverb>(); }},
        {EffectName::SCRUBBY, [](){ return std::make_unique<Scrubby>(); }},
        {EffectName::SPECTRAL_GATE, [](){ return std::make_unique<SpectralGate>(); }},
        {EffectName::VOCODER, [](){ return std::make_unique<Vocoder>(); }},
        {EffectName::WAH, [](){ return std::make_unique<Wah>(); }},
        
        {EffectName::LIMITER, [](){ return std::make_unique<Compressor>(1.0f, dbAmp(-0.6f), 100.0f, 0.0f, 1.0f, 50.0f, dbAmp(-0.3f), false); }}
    };
}

Effect* AudioChain::addEffect(EffectName name) {
    // Search for effect initializer via EffectName mapping
    auto it = effectInits.find(name);
    if (it == effectInits.end()) return nullptr; // Not found

    auto effect = it->second();
    effect->setEffectName(name);

    // Assign next free ID
    EffectID assignedID;
    if (!freeIDs.empty()) { // Use any recently freed IDs first
        assignedID = freeIDs.front();
        freeIDs.pop();
    } else assignedID = nextID++;

    effect->setID(assignedID);
    
    effects.push_back(std::move(effect));
    fxMap[assignedID] = effects.size() - 1;
    return effects.back().get();
}

void AudioChain::removeEffect(EffectID id) {
    AudioNoInterrupts();
    // Search for effect by ID
    auto it = fxMap.find(id);
    if (it == fxMap.end()) return;

    size_t idx = it->second;
    // Delete that shit
    fxMap.erase(it);
    freeIDs.push(id); // Free the ID
    effects.erase(effects.begin() + idx);

    // Update all indices
    for (auto &p : fxMap) if (p.second > idx) p.second--;
    AudioInterrupts();
}

Effect* AudioChain::getEffect(EffectID id) {
    auto it = fxMap.find(id);
    if (it == fxMap.end()) return nullptr;

    size_t idx = it->second;
    if (idx >= effects.size()) return nullptr;

    return effects[idx].get();
}

void AudioChain::reorderEffect(EffectID id, size_t pos) {
    // Find current index
    auto it = fxMap.find(id);
    if (it == fxMap.end()) return;

    Serial.printf("Reordering effect %d to position %d", id, pos);

    size_t oldPos = it->second;
    Serial.println(oldPos);
    if (pos > effects.size()) pos = effects.size();

    // Extract the effect from the vector
    auto effectPtr = std::move(effects[oldPos]);
    effects.erase(effects.begin() + oldPos);

    // Insert at new position
    effects.insert(effects.begin() + pos, std::move(effectPtr));

    // Reindex everything
    for (size_t i = 0; i < effects.size(); i++)
        fxMap[effects[i]->getID()] = i;
}

void AudioChain::clear() {
    fxMap.clear();
    effects.clear();
    nextID = 0;
    std::queue<EffectID> empty; std::swap(freeIDs, empty);
}

void AudioChain::processChain(const float* input, float* output, size_t n = BUFFER_SIZE) { // Process effect chain serially
    // Mark last active effect
    int lastActive = -1;
    for (size_t i = 0; i < effects.size(); ++i) {
        if (!effects[i]->isBypassed()) lastActive = i; }
    if (lastActive == -1) { memcpy(output, input, n * sizeof(float)); return; } // All bypass, just pass output

    const float* in_ptr = input;
    float* out_ptr = nullptr;
    float stageBuf[BUFFER_SIZE];
    for (size_t i = 0; i < effects.size(); ++i) { // Run that shit
        if (effects[i]->isBypassed()) continue;

        out_ptr = (i == (size_t)lastActive) ? output : stageBuf;
        effects[i]->process(in_ptr, out_ptr, n);
        in_ptr = out_ptr;
    }
}
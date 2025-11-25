#include "Teensy/Modules/Effect.h"
#include "Teensy/Control/ParameterRegistry.h"

/* PROTECTED */

/*

EffectID id;
EffectName effectName;
bool bypass = false;

*/

/* PUBLIC */

void Effect::setBypass(bool state) { bypass = state; }
bool Effect::isBypassed() const { return bypass; }
bool Effect::isParallel() const { return false; }
void Effect::setID(EffectID id) { this->id = id; }
EffectID Effect::getID() const { return id; }

void Effect::setEffectName(EffectName name) { effectName = name; }
EffectName Effect::getEffectName() const { return effectName; }

void Effect::setNormalized(ParamID param, float normalizedValue) {
    const ParameterRange* range = getParameterRange(effectName, param);
    if (!range) { setParam(param, normalizedValue); return; }
    setParam(param, range->fromNormalized(normalizedValue));
}

float Effect::getNormalized(ParamID param) const {
    const ParameterRange* range = getParameterRange(effectName, param);
    if (!range) return 0.5f;
    return range->toNormalized(getParam(param));
}
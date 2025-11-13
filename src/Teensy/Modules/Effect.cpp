#include "Teensy/Modules/Effect.h"

/* PROTECTED */

/*

bool bypass = false;
EffectID id;

*/

/* PUBLIC */

void Effect::setBypass(bool state) { bypass = state; }
bool Effect::isBypassed() const { return bypass; }
void Effect::setID(EffectID id) { this->id = id; }
EffectID Effect::getID() const { return id; }
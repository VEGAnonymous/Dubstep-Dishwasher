#include "Teensy/Filters/APF.h"

/* PRIVATE */

/*

enum Params : ParamID { CUTOFF = 1, Q };

const size_t maxDelaySamples;
const bool useDFII;
const bool usePSRAM; // This shitty flag shouldn't even be necessary but here we are

float cutoff, g;
size_t N;
bool invert;

std::unique_ptr<DelayLineVector> bufferXv, bufferYv; // DFI vector
std::unique_ptr<DelayLineVector> bufferv; // DFII vector
std::unique_ptr<DelayLine> bufferX, bufferY; // DFI PSRAM
std::unique_ptr<DelayLine> buffer; // DFII PSRAM

float g_s, g_sq; // Cached

*/

void APF::updateSign() {
    auto s = invert ? -1 : 1; // Invert sign as needed
    g_s = s * g;
}

void APF::setBufferDelays(size_t N) { // This is fucking disgusting
    if (useDFII) {
        if (usePSRAM) buffer->setDelaySamples(N); 
        else bufferv->setDelaySamples(N);
    } else { 
        if (usePSRAM) { 
            bufferX->setDelaySamples(N); 
            bufferY->setDelaySamples(N); 
        } else { 
            bufferXv->setDelaySamples(N); 
            bufferYv->setDelaySamples(N); 
        } 
    }
}

/* PUBLIC */

APF::APF(float mix, float cutoff, float q, bool invert, float maxDelayTime, bool useDFII, bool usePSRAM)
    : maxDelaySamples((size_t)(maxDelayTime * SAMPLE_RATE / 1000.0f)), useDFII(useDFII), usePSRAM(usePSRAM) {

    // Allocate buffers
    if (useDFII) {
        if (usePSRAM) buffer = std::make_unique<DelayLine>(500.0f / cutoff, maxDelayTime);
        else bufferv = std::make_unique<DelayLineVector>(500.0f / cutoff, maxDelayTime);
    } else {
        if (usePSRAM) { // This is also disgusting
            bufferX = std::make_unique<DelayLine>(500.0f / cutoff, maxDelayTime); 
            bufferY = std::make_unique<DelayLine>(500.0f / cutoff, maxDelayTime); 
        } else {
            bufferXv = std::make_unique<DelayLineVector>(500.0f / cutoff, maxDelayTime); 
            bufferYv = std::make_unique<DelayLineVector>(500.0f / cutoff, maxDelayTime); 
        }
    }

    IIR_Filter::mix = mix; setCutoff(cutoff); setQ(q); setInvert(invert);
}

float APF::readTap(float offset) { // Tap the output delay line
    if (useDFII) return 0.0f;
    else if (usePSRAM) return bufferY->read(offset);
    else return bufferYv->read(offset);
} 
void APF::setInvert(bool invert) { this->invert = invert; updateSign(); }
void APF::setDelay(size_t N) { 
    this->N = std::clamp(N, (size_t)1, maxDelaySamples); 
    cutoff = SAMPLE_RATE / (2.0f * (float)N);
    setBufferDelays(N);
}
void APF::setCutoff(float cutoff) { // Hz
    N = std::clamp(SAMPLE_RATE / (2.0f * cutoff), 1.0f, (float)maxDelaySamples); // Cutoff translates to delay N
    this->cutoff = cutoff;
    setBufferDelays(N);
}
void APF::setQ(float q) { // Q translates to coefficient g
    g = std::clamp(1.0f - (1.0f / q), -0.999f, 0.999f);
    g_sq = 1.0f - g*g;
    updateSign();
} 
void APF::setParam(ParamID param, float value) { 
    switch (param) {
        case CUTOFF: setCutoff(value); break;
        case Q: setQ(value); break;
        default: IIR_Filter::setParam(param, value);
    }
}

float APF::LCCDE(float x) {
    if (!useDFII) { // Direct Form I
        // y[n] = -gy[n-N] + gx[n] + x[n-N]
        float y = 0.0f;
        if (usePSRAM) { y = (-g_s * bufferY->read()) + (g_s * x) + bufferX->read(); bufferX->write(x); bufferY->write(y); }
        else { y = (-g_s * bufferYv->read()) + (g_s * x) + bufferXv->read(); bufferXv->write(x); bufferYv->write(y); }
        return y;
    } else { // Direct Form II
        // v[n] = (1-g^2)x[n] - gw[n-N]
        // y[n] = gx[n] + w[n-N]
        float v_D = usePSRAM ? buffer->read() : bufferv->read();
        float y = (g_s * x) + v_D;
        float v = (g_sq * x) - (g_s * v_D);
        if (usePSRAM) buffer->write(v); else bufferv->write(v);
        return y;
    }
}
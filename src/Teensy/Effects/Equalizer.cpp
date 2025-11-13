#include "Teensy/Effects/Equalizer.h"

/* PRIVATE */

/*

enum Params : ParamID { MIX, BAND1_TYPE, BAND1_CUTOFF, BAND1_Q, BAND1_GAIN, BAND2_TYPE, BAND2_CUTOFF, BAND2_Q, BAND2_GAIN };

std::array<std::unique_ptr<Biquad>, 2> bands;

BiquadType band1Type, band2Type;
float mix, band1Cutoff, band1Q, band1Gain, band2Cutoff, band2Q, band2Gain;

*/

std::unique_ptr<Biquad> Equalizer::createBiquad(BiquadType type, float cutoff, float q, float gainDB) {
    switch (type) {
        case BiquadType::LOW_PASS: return std::make_unique<LPF_Biquad>(cutoff, q, gainDB);
        case BiquadType::HIGH_PASS: return std::make_unique<HPF_Biquad>(cutoff, q, gainDB);
        case BiquadType::LOW_SHELF: return std::make_unique<LowShelf_Biquad>(cutoff, q, gainDB);
        case BiquadType::HIGH_SHELF: return std::make_unique<HighShelf_Biquad>(cutoff, q, gainDB);
        case BiquadType::PEAK: return std::make_unique<Peak_Biquad>(cutoff, q, gainDB);
        case BiquadType::NOTCH: return std::make_unique<Notch_Biquad>(cutoff, q, gainDB);
        default: return std::make_unique<Peak_Biquad>(cutoff, q, gainDB);
    }
}

/* PUBLIC */

Equalizer::Equalizer(float mix,
                     BiquadType band1Type, float band1Cutoff, float band1Q, float band1Gain,
                     BiquadType band2Type, float band2Cutoff, float band2Q, float band2Gain) {
    bands[0] = createBiquad(band1Type, band1Cutoff, band1Q, band1Gain);
    bands[1] = createBiquad(band2Type, band2Cutoff, band2Q, band2Gain);

    setMix(mix);
    setBand1Type(band1Type); setBand1Cutoff(band1Cutoff); setBand1Q(band1Q); setBand1Gain(band1Gain);
    setBand2Type(band2Type); setBand2Cutoff(band2Cutoff); setBand2Q(band2Q); setBand2Gain(band2Gain);
}

void Equalizer::setMix(float mix) { this->mix = std::clamp(mix, 0.0f, 1.0f); } // [0.0, 1.0]
void Equalizer::setBand1Type(BiquadType type) {
    if (type == band1Type) return;
    band1Type = type;
    bands[0] = createBiquad(band1Type, band1Cutoff, band1Q, band1Gain);
}
void Equalizer::setBand1Cutoff(float cutoff) { band1Cutoff = std::clamp(cutoff, 20.0f, 20000.0f); bands[0]->setCutoff(band1Cutoff); } // Hz, [20.0, 20000.0]
void Equalizer::setBand1Q(float q) { band1Q = std::clamp(q, 0.02f, 40.0f); bands[0]->setQ(band1Q); } // [0.02, 40.0]
void Equalizer::setBand1Gain(float gainDB) { band1Gain = std::clamp(gainDB, -24.0f, 24.0f); bands[0]->setGain(band1Gain); } // dB, [-24.0, 24.0]
void Equalizer::setBand2Type(BiquadType type) {
    if (type == band2Type) return; 
    band2Type = type;
    bands[1] = createBiquad(band2Type, band2Cutoff, band2Q, band2Gain);
}
void Equalizer::setBand2Cutoff(float cutoff) { band2Cutoff = std::clamp(cutoff, 20.0f, 20000.0f); bands[1]->setCutoff(band2Cutoff); } // Hz, [20.0, 20000.0]
void Equalizer::setBand2Q(float q) { band2Q = std::clamp(q, 0.02f, 40.0f); bands[1]->setQ(band2Q); } // [0.02, 40.0]
void Equalizer::setBand2Gain(float gainDB) { band2Gain = std::clamp(gainDB, -24.0f, 24.0f); bands[1]->setGain(band2Gain); } // dB, [-24.0, 24.0]
void Equalizer::setParam(ParamID param, float value) { 
    switch (param) {
        case MIX: setMix(value); break;
        case BAND1_TYPE: setBand1Type((BiquadType)value); break; 
        case BAND1_CUTOFF: setBand1Cutoff(value); break;
        case BAND1_Q: setBand1Q(value); break;
        case BAND1_GAIN: setBand1Gain(value); break;
        case BAND2_TYPE: setBand2Type((BiquadType)value); break;
        case BAND2_CUTOFF: setBand2Cutoff(value); break;
        case BAND2_Q: setBand2Q(value); break;
        case BAND2_GAIN: setBand2Gain(value); break;
    }
}

void Equalizer::process(const float* in, float* out, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        float y = in[i];
        for (auto& band : bands) if (!band->isBypassed()) y = band->processSample(y);
        out[i] = dryWetMix(in[i], y, mix);
    }
}
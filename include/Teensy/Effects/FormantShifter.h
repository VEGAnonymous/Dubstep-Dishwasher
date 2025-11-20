#include "Teensy/Modules/Phase_Vocoder.h"

class FormantShifter : public Phase_Vocoder {
    // Alexander Panos' GOATed formant shifter device, faithfully ported from Max/MSP (Gen) to C++!
    // https://alexanderpanos.com/software
    private:
        enum Params : ParamID { FORMANT_SHIFT = 2, ENVELOPE_WIDTH };

        float formantShift; size_t envelopeWidth;
        size_t width; // internal value after env_compensation
        
        std::vector<float> buf;
        std::vector<float> interp; // interpolated peaks
        std::vector<std::pair<float, size_t>> pk_info; // ch.0 = value of peak, ch.1 = index of peak in "buf"
        std::vector<float> formants;
        
    public:
        FormantShifter(float mix = 1.0f, float formantShift = 0.0f, size_t envelopeWidth = 16, size_t fftSize = 1024, size_t hopFactor = 4);
        
        void setFormantShift(float formantShift); // semitones, [-12.0, 12.0]
        void setEnvelopeWidth(size_t envelopeWidth);
        void setParam(ParamID param, float value) override;
        float getParam(ParamID param) const override;

    protected:
        void processSpectrum(STFT::FFTFrame& frame) override;
};
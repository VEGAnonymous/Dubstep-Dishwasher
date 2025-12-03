#include "Teensy/Modules/Phase_Vocoder.h"

class SpectralGate : public Phase_Vocoder {
    private:
        enum Params : ParamID { THRESHOLD = 2, TILT, INVERT };

        float thresholdDB, threshold, tilt; bool invert;
        
    public:
        SpectralGate(float mix = 1.0f, float thresholdDB = -10.0f, float tilt = 0.5f, bool invert = 0,
            size_t fftSize = 512, size_t hopFactor = 4);
        
        void setThreshold(float thresholdDB); // dB, [-100.0, 0.0]
        void setTilt(float tilt); // [-1.0, 1.0]
        void setInvert(bool invert);
        void setParam(ParamID param, float value) override;
        float getParam(ParamID param) const override;

    protected:
        void processSpectrum(STFT::FFTFrame& frame) override;
};
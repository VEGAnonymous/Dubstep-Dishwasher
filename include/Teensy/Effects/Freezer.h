#pragma once

#include "Teensy/Modules/Effect.h"
#include "Teensy/Utilities/STFT.h"

#include <memory>

class Freezer : public Effect {
    private:
        enum Params : ParamID { MIX, RATE, SPECTRAL_MODE, FFT_SIZE, LOOP_START, LOOP_END };

        static constexpr size_t bufSize = (size_t)3 * (size_t)SAMPLE_RATE; // 3s running buffer
        static constexpr float smooth = 0.005f; // Smoothing factor at loop boundaries
        
        float mix, rate; bool spectralMode;
        float loopStart, loopEnd;
        
        // Time domain
        float* inBuf = nullptr; size_t writePos = 0; float readPos = 0.0f;
        
        // Spectral resythesis
        size_t fftSize; 
        static constexpr size_t hopFactor = 4;
        std::unique_ptr<STFT> stft;
        std::vector<float> spectBuf, spectFrame;
        size_t spectPos = 0, spectHopCounter = 0;

        void allocateSTFT();
        void freeSTFT();

    public:
        Freezer(float mix = 1.0f, float rate = 1.0f, bool spectralMode = false, size_t fftSize = 1024, 
                float loopStart = 0.0f, float loopEnd = 1.0f);
        ~Freezer();
        
        void setMix(float mix); // [0.0, 1.0]
        void setRate(float rate); // [-4.0, 4.0]
        void setSpectralMode(bool mode);
        void setFFTSize(size_t N); // [128, FFT_MAX_SIZE], MUST BE POWER OF 2 (I'm not going to ask you again)
        void setLoopRegion(float start, float end); // [0.0, 1.0] for both
        void setParam(ParamID param, float value) override;
        
        void process(const float* in, float* out, size_t n) override;
};
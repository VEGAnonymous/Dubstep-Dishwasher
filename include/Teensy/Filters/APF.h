#pragma once

#include "Teensy/Modules/IIR_Filter.h"
#include "Teensy/Utilities/DelayLine.h"
#include "Teensy/Utilities/DelayLineVector.h"
#include "Teensy/Defines.h"

#include <memory>

class APF : public IIR_Filter { // 1st order, Direct Form I/II
    private:
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

        void updateSign();

        void setBufferDelays(size_t N);

    public:
        APF(float mix = 1.0f, float cutoff = 1000.0f, float q = 0.8f, bool invert = false, 
            float maxDelayTime = 1.0f, bool useDFII = true, bool usePSRAM = true);

        float readTap(float offset);
        void setInvert(bool invert);
        void setDelay(size_t N);
        void setCutoff(float cutoff);
        void setQ(float q);
        void setParam(ParamID param, float value) override;
        float getParam(ParamID param) const override;

        float LCCDE(float x) override;
};
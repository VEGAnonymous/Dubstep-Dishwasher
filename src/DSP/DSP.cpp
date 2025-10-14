/* CHANGELOG

10-9-25:
- Initial commit

10-10-25:
- Implemented Chorus, FIR_Filter effects
- Implemented Random generator with three modes: perlin (smooth) noise, sample and hold, binary
- Refactored Flanger to use DelayLine instead of Delay
- Added (optional) anti-aliasing FIR filter to Distortion
- New Distortion algorithms: Tube, diode, rectifier

10-13-25:
- Split DSP codebase into modular files
- "Implemented" FFT via kissfft lib
- Implemented Spectral_Effect base class
- Implemented Spectral Gate effect
- Rescaled Distortion drive ranges
- New Distortion algorithm: Saturate (probably the last)

*/

/* TESTBENCH */

#include "Defines.h"
#include "Utilities.h"
#include "Modules.h"
#include "Generators.h"
#include "Filters.h"
#include "Effects.h"
#include "Control.h"
#include "LUTs.h"

// For testing; irrelevant to embedded implementation
#include <iostream>
#include <cstring>
#include <chrono>
#include <sndfile.h>

using namespace std;

void setTestParams(AudioChain& chain) { // Set DSP testing parameters
    chain.getEffect("Gain")->setBypass(true);
    chain.getEffect("Gain")->setParam("Gain", dbAmp(3.0f));

    chain.getEffect("LPF")->setBypass(true);
    chain.getEffect("LPF")->setParam("Mix", 1.0f);
    chain.getEffect("LPF")->setParam("Cutoff", 300.0f);

    chain.getEffect("APF")->setBypass(true);
    chain.getEffect("APF")->setParam("Mix", 1.0f);
    chain.getEffect("APF")->setParam("Cutoff", 1000.0f);
    chain.getEffect("APF")->setParam("Q", 0.5f);

    chain.getEffect("FIR")->setBypass(true);
    chain.getEffect("FIR")->setParam("Mix", 1.0f);

    chain.getEffect("Distortion")->setBypass(true);
    chain.getEffect("Distortion")->setParam("Mix", 1.0f);
    chain.getEffect("Distortion")->setParam("Mode", TUBE);
    chain.getEffect("Distortion")->setParam("Drive", 1.0f);
    chain.getEffect("Distortion")->setParam("AAF", false);

    chain.getEffect("Delay")->setBypass(true);
    chain.getEffect("Delay")->setParam("Mix", 0.5f);
    chain.getEffect("Delay")->setParam("Time", 500.0f);
    chain.getEffect("Delay")->setParam("Feedback", 0.5f);
    
    chain.getEffect("Flanger")->setBypass(true);
    chain.getEffect("Flanger")->setParam("Mix", 1.0f);
    chain.getEffect("Flanger")->setParam("Rate", 0.2f);
    chain.getEffect("Flanger")->setParam("Depth", 1.0f);
    chain.getEffect("Flanger")->setParam("Feedback", 0.7f);

    chain.getEffect("Phaser")->setBypass(true);
    chain.getEffect("Phaser")->setParam("Mix", 1.0f);
    chain.getEffect("Phaser")->setParam("Rate", 0.2f);
    chain.getEffect("Phaser")->setParam("Center", 500.0f);
    chain.getEffect("Phaser")->setParam("Spread", 0.3f);
    chain.getEffect("Phaser")->setParam("Depth", 1.0f);
    chain.getEffect("Phaser")->setParam("Feedback", 0.5f);

    chain.getEffect("Chorus")->setBypass(true);
    chain.getEffect("Chorus")->setParam("Mix", 0.5f);
    chain.getEffect("Chorus")->setParam("Rate", 0.2f);
    chain.getEffect("Chorus")->setParam("Depth", 1.0f);
    chain.getEffect("Chorus")->setParam("Delay", 50.0f);
    chain.getEffect("Chorus")->setParam("Feedback", 0.6f);

    chain.getEffect("Reverb")->setBypass(true);
    chain.getEffect("Reverb")->setParam("Mix", 0.5f);
    chain.getEffect("Reverb")->setParam("Predelay", 50.0f);
    chain.getEffect("Reverb")->setParam("Decay", 5000.0f);
    chain.getEffect("Reverb")->setParam("Mod Rate", 0.2f);
    chain.getEffect("Reverb")->setParam("Mod Depth", 0.5f);
    chain.getEffect("Reverb")->setParam("Damping", 0.0005f);

    chain.getEffect("Spectral Gate")->setBypass(false);
    chain.getEffect("Spectral Gate")->setParam("Mix", 1.0f);
    chain.getEffect("Spectral Gate")->setParam("Threshold", -10.0f);
    chain.getEffect("Spectral Gate")->setParam("FFT Size", 1024);
};

int main() {
    SF_INFO sfInfo;
    memset(&sfInfo, 0.0f, sizeof(sfInfo));

    // Open audio in/out files
    SNDFILE* inFile = sf_open("test.wav", SFM_READ, &sfInfo);
    SNDFILE* outFile = sf_open("result.wav", SFM_WRITE, &sfInfo);
    printf("Input: %d Hz, %d channels\n", sfInfo.samplerate, sfInfo.channels);

    // Init effects chain
    AudioChain chain;
    float input[BUFFER_SIZE], output[BUFFER_SIZE];
    sf_count_t readCount;

    setTestParams(chain);

    auto start = chrono::high_resolution_clock::now();
    // Process buffers sequentially and write to output
    while ((readCount = sf_read_float(inFile, input, BUFFER_SIZE)) > 0) {
        if (readCount < BUFFER_SIZE) { memset(input + readCount, 0, (BUFFER_SIZE - readCount) * sizeof(float)); } // Zero-pad ending
        chain.processChain(input, output, BUFFER_SIZE);
        // cout << *input << endl;
        // cout << *output << endl;
        sf_write_float(outFile, output, readCount);
    }

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end - start;

    sf_close(inFile); sf_close(outFile);
    printf("Successfully processed chain in %fs\n", elapsed.count());
    return 0;
}
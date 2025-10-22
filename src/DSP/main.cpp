#include <Arduino.h>
#include <Audio.h>

#include "Defines.h"
#include "Utilities.h"
#include "Modules.h"
#include "Generators.h"
#include "Filters.h"
#include "Effects.h"
#include "Control.h"
#include "LUTs.h"

using namespace std;

AudioInputI2S i2sInput; // ADC input
AudioOutputI2S i2sOutput; // DAC output
AudioConnection* patch1 = nullptr; AudioConnection* patch2 = nullptr; // Connections

AudioChain chain; // FX chain
AudioChainStream *stream = nullptr; // DSP stream

void setTestParams(AudioChain& chain) { // Set DSP testing parameters
    chain.getEffect("Gain")->setBypass(true);
    chain.getEffect("Gain")->setParam("Gain", dbAmp(3.0f));

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

    chain.getEffect("Spectral Gate")->setBypass(true);
    chain.getEffect("Spectral Gate")->setParam("Mix", 1.0f);
    chain.getEffect("Spectral Gate")->setParam("Threshold", -10.0f);
    chain.getEffect("Spectral Gate")->setParam("Tilt", 1.0f);
    chain.getEffect("Spectral Gate")->setParam("FFT Size", 1024);

    chain.getEffect("Compressor")->setBypass(true);
    chain.getEffect("Compressor")->setParam("Mix", 1.0f);
    chain.getEffect("Compressor")->setParam("Threshold", -200.0f);
    chain.getEffect("Compressor")->setParam("Ratio", 20.0f);
    chain.getEffect("Compressor")->setParam("Knee", 10.0f);
    chain.getEffect("Compressor")->setParam("Attack", 50.0f);
    chain.getEffect("Compressor")->setParam("Release", 500.0f);
    chain.getEffect("Compressor")->setParam("Makeup", 0.0f);
    chain.getEffect("Compressor")->setParam("Auto Makeup", true);

    chain.getEffect("Granulator")->setBypass(true);
    chain.getEffect("Granulator")->setParam("Mix", 1.0f);
    chain.getEffect("Granulator")->setParam("Position", 0.5f);
    chain.getEffect("Granulator")->setParam("Position Random", 1.0f);
    chain.getEffect("Granulator")->setParam("Time", 50.0f);
    chain.getEffect("Granulator")->setParam("Time Random", 0.5f);
    chain.getEffect("Granulator")->setParam("Length", 500.0f);
    chain.getEffect("Granulator")->setParam("Length Random", 0.5f);
    chain.getEffect("Granulator")->setParam("Level", 0.8f);
    chain.getEffect("Granulator")->setParam("Level Random", 1.0f);
    chain.getEffect("Granulator")->setParam("Reverse Chance", 0.0f);
    chain.getEffect("Granulator")->setParam("Envelope Type", PERC);

    chain.getEffect("Freezer")->setBypass(false);
    chain.getEffect("Freezer")->setParam("Mix", 1.0f);
    chain.getEffect("Freezer")->setParam("Rate", -2.0f);
    chain.getEffect("Freezer")->setParam("Spectral Mode", false);
    chain.getEffect("Freezer")->setParam("FFT Size", 1024);
    chain.getEffect("Freezer")->setParam("Hop Size", 4);
};

void setup() {
    /* Allocate memory */
    AudioMemory(12);

    /* Setup DSP stream connections */
    // Input -> DSP -> Output
    stream = new AudioChainStream(chain);
    patch1 = new AudioConnection(i2sInput, 0, *stream, 0);
    patch2 = new AudioConnection(*stream, 0, i2sOutput, 0);

    setTestParams(chain); // Set DSP params

    /* Other stuff */
    delay(500);
}

void loop() { }
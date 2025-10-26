#ifndef DEFINES
#define DEFINES

/* DEFINES */

#include <Audio.h>

#define SAMPLE_RATE AUDIO_SAMPLE_RATE_EXACT
#define BUFFER_SIZE AUDIO_BLOCK_SAMPLES

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// Datatypes
using EffectID = uint8_t;
using ParamID = uint8_t;

/* ENUMS */

enum envelopeType { HANN, HAMMING, SINE, TRI, PERC, SMOOTH_RECT };
enum wavetable { SINE_TABLE, TRI_TABLE, SAW_TABLE, SQUARE_TABLE };
enum randomMode { PERLIN, SAMPLE_HOLD, BINARY };
enum distortionMode { TUBE, SOFT_CLIP, HARD_CLIP, DIODE, BITCRUSH, RECTIFY, SATURATE };
enum biquadType { LOW_PASS, HIGH_PASS, LOW_SHELF, HIGH_SHELF, PEAK, NOTCH };

#endif // DEFINES
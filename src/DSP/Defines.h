#ifndef DEFINES
#define DEFINES

/* DEFINES */

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 256
#define CONTROL_RATE 64 // # of samples between control (e.g., LFO) updates; unused at the moment

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

/* ENUMS */

enum envelopeType { HANN, HAMMING, SINE, TRI, PERC, SMOOTH_RECT };
enum wavetable { SINE_TABLE, TRI_TABLE, SAW_TABLE, SQUARE_TABLE };
enum randomMode { PERLIN, SAMPLE_HOLD, BINARY };
enum distortionMode { TUBE, SOFT_CLIP, HARD_CLIP, DIODE, BITCRUSH, RECTIFY, SATURATE };

#endif // DEFINES
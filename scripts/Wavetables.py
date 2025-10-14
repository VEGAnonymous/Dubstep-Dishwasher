# Shitty wavetable array generator (from function)

import math

N = 512
wave = "square" # "sine", "tri", "saw", "square" 

samples = []
for i in range(N):
    phase = i / N
    if wave == "sine":
        s = math.sin(2 * math.pi * phase)
    elif wave == "tri":
        s = 4.0 * abs(phase - 0.5) - 1.0
    elif wave == "saw":
        s = 2.0 * phase - 1.0
    elif wave == "square":
        s = 1.0 if phase < 0.5 else -1.0
    # Add more here!
    samples.append(s)

print([round(val, 4) for val in samples])
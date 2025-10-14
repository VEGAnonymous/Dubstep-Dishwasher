# Script to generate FIR filter kernels using Remez exchange

import numpy as np
from scipy import signal as sig
from matplotlib import pyplot as plt

# Spec
fs = 44100
f_p = 19000
f_s = 21000
R_p = 0.25 # Maximum passband ripple (dB)
A_s = 60 # Stopband attenuation (dB)

w_p = 2 * np.pi * f_p / fs
w_s = 2 * np.pi * f_s / fs

d1 = (10**(R_p/20)-1) / (10**(R_p/20)+1)
d2 = (1+d1)*(10**(-A_s/20))

M, _ = sig.kaiserord(A_s, (f_s - f_p) / fs) # Use optimal Kaiser order
M = 64 # Or manually override
print(M)

f = [0, f_p/fs, f_s/fs, 0.5]
m = [1, 0]
weights = [d2/d1, 1]

h = sig.remez(numtaps=M, bands=f, desired=m, weight=weights)

# Plot h[n] and |H(f)|
FFT_N = 1024
n = np.linspace(0, fs/2, FFT_N//2)
H_k = np.abs(np.fft.fft(h, FFT_N))

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(9, 9))
ax1.stem(h)
ax1.set_title('h[n]')

ax2.plot(n, H_k[:FFT_N//2])
ax2.set_title('|H(f)|')
ax2.set_xlabel('Frequency (Hz)')
ax2.set_ylabel('Magnitude')
ax2.set_ylim(-0.10, 1.10)
ax2.set_yticks(np.arange(-0.1, 1.1, 0.05))

plt.tight_layout()
plt.show()

# PRINT ARRAY
print("float h[%d] = {%s};" % (len(h), ", ".join([f"{v:.8f}" for v in h])))
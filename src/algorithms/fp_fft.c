// fp_fft.c
//
// Fast Fourier Transform (FFT) - Cooley-Tukey Algorithm
// Demonstrates divide-and-conquer and signal processing
//
// This showcases:
// - FFT (Cooley-Tukey radix-2 decimation-in-time)
// - Inverse FFT (IFFT)
// - Fast convolution (O(n log n) vs O(n²))
// - Frequency domain filtering
// - Spectral analysis
//
// FP Primitives Used:
// - Recursive divide-and-conquer
// - Complex number operations
// - Bit-reversal permutation
//
// Complexity: O(n log n) vs O(n²) for DFT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// Complex Number Operations
// ============================================================================

typedef struct {
    double real;
    double imag;
} Complex;

// Complex addition: a + b
static inline Complex complex_add(Complex a, Complex b) {
    Complex result;
    result.real = a.real + b.real;
    result.imag = a.imag + b.imag;
    return result;
}

// Complex subtraction: a - b
static inline Complex complex_sub(Complex a, Complex b) {
    Complex result;
    result.real = a.real - b.real;
    result.imag = a.imag - b.imag;
    return result;
}

// Complex multiplication: a * b
static inline Complex complex_mul(Complex a, Complex b) {
    Complex result;
    result.real = a.real * b.real - a.imag * b.imag;
    result.imag = a.real * b.imag + a.imag * b.real;
    return result;
}

// Complex magnitude: |a|
static inline double complex_magnitude(Complex a) {
    return sqrt(a.real * a.real + a.imag * a.imag);
}

// Complex phase: arg(a)
static inline double complex_phase(Complex a) {
    return atan2(a.imag, a.real);
}

// Create complex from polar: r * e^(iθ)
static inline Complex complex_from_polar(double r, double theta) {
    Complex result;
    result.real = r * cos(theta);
    result.imag = r * sin(theta);
    return result;
}

// ============================================================================
// Bit Reversal Permutation
// ============================================================================

// Reverse bits of integer (for bit-reversal permutation)
static unsigned int reverse_bits(unsigned int x, int log2n) {
    unsigned int result = 0;
    for (int i = 0; i < log2n; i++) {
        result = (result << 1) | (x & 1);
        x >>= 1;
    }
    return result;
}

// Bit-reversal permutation (in-place)
static void bit_reverse_permutation(Complex* data, int n) {
    int log2n = 0;
    int temp_n = n;
    while (temp_n > 1) {
        log2n++;
        temp_n >>= 1;
    }

    for (unsigned int i = 0; i < (unsigned int)n; i++) {
        unsigned int j = reverse_bits(i, log2n);
        if (j > i) {
            // Swap data[i] and data[j]
            Complex temp = data[i];
            data[i] = data[j];
            data[j] = temp;
        }
    }
}

// ============================================================================
// FFT (Cooley-Tukey Radix-2 Decimation-in-Time)
// ============================================================================

// FFT - Forward transform
// Input: data (length n, must be power of 2)
// Output: frequency domain (in-place)
void fp_fft(Complex* data, int n) {
    if (n <= 1) return;

    // Bit-reversal permutation
    bit_reverse_permutation(data, n);

    // Cooley-Tukey FFT (iterative, in-place)
    for (int s = 1; s <= (int)(log2(n)); s++) {
        int m = 1 << s;  // 2^s
        int m2 = m >> 1; // m/2

        // Twiddle factor: W_m = e^(-2πi/m)
        Complex wm = complex_from_polar(1.0, -2.0 * M_PI / m);

        for (int k = 0; k < n; k += m) {
            Complex w = {1.0, 0.0};  // w^0 = 1

            for (int j = 0; j < m2; j++) {
                // Butterfly operation
                Complex t = complex_mul(w, data[k + j + m2]);
                Complex u = data[k + j];

                data[k + j] = complex_add(u, t);
                data[k + j + m2] = complex_sub(u, t);

                w = complex_mul(w, wm);
            }
        }
    }
}

// IFFT - Inverse transform
// Input: frequency domain (length n, must be power of 2)
// Output: time domain (in-place)
void fp_ifft(Complex* data, int n) {
    if (n <= 1) return;

    // Conjugate input
    for (int i = 0; i < n; i++) {
        data[i].imag = -data[i].imag;
    }

    // Forward FFT
    fp_fft(data, n);

    // Conjugate output and scale
    for (int i = 0; i < n; i++) {
        data[i].real /= n;
        data[i].imag = -data[i].imag / n;
    }
}

// ============================================================================
// Real FFT (optimized for real-valued signals)
// ============================================================================

// Real FFT - Forward transform for real signals
// Input: real signal (length n)
// Output: complex spectrum (length n/2 + 1, exploits Hermitian symmetry)
void fp_rfft(const double* real_data, Complex* freq_data, int n) {
    // Convert real to complex
    Complex* temp = (Complex*)malloc(n * sizeof(Complex));
    for (int i = 0; i < n; i++) {
        temp[i].real = real_data[i];
        temp[i].imag = 0.0;
    }

    // Perform FFT
    fp_fft(temp, n);

    // Copy positive frequencies (exploit Hermitian symmetry)
    int half = n / 2 + 1;
    for (int i = 0; i < half; i++) {
        freq_data[i] = temp[i];
    }

    free(temp);
}

// Real IFFT - Inverse transform to real signal
// Input: complex spectrum (length n/2 + 1)
// Output: real signal (length n)
void fp_irfft(const Complex* freq_data, double* real_data, int n) {
    Complex* temp = (Complex*)malloc(n * sizeof(Complex));

    int half = n / 2 + 1;

    // Copy positive frequencies
    for (int i = 0; i < half; i++) {
        temp[i] = freq_data[i];
    }

    // Mirror negative frequencies (Hermitian symmetry)
    for (int i = half; i < n; i++) {
        int mirror = n - i;
        temp[i].real = temp[mirror].real;
        temp[i].imag = -temp[mirror].imag;
    }

    // Perform IFFT
    fp_ifft(temp, n);

    // Extract real part
    for (int i = 0; i < n; i++) {
        real_data[i] = temp[i].real;
    }

    free(temp);
}

// ============================================================================
// Fast Convolution (using FFT)
// ============================================================================

// Convolution via FFT: y = x ⊛ h
// Complexity: O(n log n) vs O(n²) for direct convolution
// Input: signal x (length n), filter h (length m)
// Output: convolution y (length n+m-1)
void fp_fft_convolve(const double* x, int n, const double* h, int m, double* y) {
    // Find next power of 2 >= n + m - 1
    int output_len = n + m - 1;
    int fft_size = 1;
    while (fft_size < output_len) {
        fft_size <<= 1;
    }

    // Zero-pad inputs
    Complex* X = (Complex*)calloc(fft_size, sizeof(Complex));
    Complex* H = (Complex*)calloc(fft_size, sizeof(Complex));
    Complex* Y = (Complex*)calloc(fft_size, sizeof(Complex));

    for (int i = 0; i < n; i++) {
        X[i].real = x[i];
        X[i].imag = 0.0;
    }

    for (int i = 0; i < m; i++) {
        H[i].real = h[i];
        H[i].imag = 0.0;
    }

    // FFT both signals
    fp_fft(X, fft_size);
    fp_fft(H, fft_size);

    // Multiply in frequency domain
    for (int i = 0; i < fft_size; i++) {
        Y[i] = complex_mul(X[i], H[i]);
    }

    // IFFT to get convolution
    fp_ifft(Y, fft_size);

    // Extract real part
    for (int i = 0; i < output_len; i++) {
        y[i] = Y[i].real;
    }

    free(X);
    free(H);
    free(Y);
}

// ============================================================================
// Signal Analysis Utilities
// ============================================================================

// Compute power spectrum: |X(f)|²
void fp_fft_power_spectrum(const Complex* freq_data, double* power, int n) {
    for (int i = 0; i < n; i++) {
        power[i] = freq_data[i].real * freq_data[i].real +
                   freq_data[i].imag * freq_data[i].imag;
    }
}

// Compute magnitude spectrum: |X(f)|
void fp_fft_magnitude_spectrum(const Complex* freq_data, double* magnitude, int n) {
    for (int i = 0; i < n; i++) {
        magnitude[i] = complex_magnitude(freq_data[i]);
    }
}

// Compute phase spectrum: arg(X(f))
void fp_fft_phase_spectrum(const Complex* freq_data, double* phase, int n) {
    for (int i = 0; i < n; i++) {
        phase[i] = complex_phase(freq_data[i]);
    }
}

// Apply frequency domain filter (element-wise multiplication)
void fp_fft_filter(Complex* freq_data, const double* filter, int n) {
    for (int i = 0; i < n; i++) {
        freq_data[i].real *= filter[i];
        freq_data[i].imag *= filter[i];
    }
}

// ============================================================================
// Signal Generation (for testing)
// ============================================================================

// Generate sine wave: A * sin(2πft + φ)
void fp_generate_sine(double* signal, int n, double frequency, double amplitude, double phase, double sample_rate) {
    for (int i = 0; i < n; i++) {
        double t = i / sample_rate;
        signal[i] = amplitude * sin(2.0 * M_PI * frequency * t + phase);
    }
}

// Generate cosine wave: A * cos(2πft + φ)
void fp_generate_cosine(double* signal, int n, double frequency, double amplitude, double phase, double sample_rate) {
    for (int i = 0; i < n; i++) {
        double t = i / sample_rate;
        signal[i] = amplitude * cos(2.0 * M_PI * frequency * t + phase);
    }
}

// Generate square wave
void fp_generate_square(double* signal, int n, double frequency, double amplitude, double sample_rate) {
    for (int i = 0; i < n; i++) {
        double t = i / sample_rate;
        double phase = fmod(frequency * t, 1.0);
        signal[i] = (phase < 0.5) ? amplitude : -amplitude;
    }
}

// Add Gaussian white noise
void fp_add_noise(double* signal, int n, double noise_level) {
    for (int i = 0; i < n; i++) {
        // Box-Muller transform for Gaussian noise
        double u1 = (double)rand() / RAND_MAX;
        double u2 = (double)rand() / RAND_MAX;
        double noise = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
        signal[i] += noise_level * noise;
    }
}

// ============================================================================
// Verification and Utilities
// ============================================================================

// Verify Parseval's theorem: energy is preserved
// ∑|x[n]|² = (1/N)∑|X[k]|²
double fp_fft_parseval_check(const double* time_signal, const Complex* freq_signal, int n) {
    double time_energy = 0.0;
    double freq_energy = 0.0;

    for (int i = 0; i < n; i++) {
        time_energy += time_signal[i] * time_signal[i];
        freq_energy += complex_magnitude(freq_signal[i]) * complex_magnitude(freq_signal[i]);
    }

    freq_energy /= n;
    return fabs(time_energy - freq_energy) / time_energy;  // Relative error
}

// Check if n is power of 2
int fp_fft_is_power_of_2(int n) {
    return (n > 0) && ((n & (n - 1)) == 0);
}

// Find next power of 2 >= n
int fp_fft_next_power_of_2(int n) {
    int result = 1;
    while (result < n) {
        result <<= 1;
    }
    return result;
}

// ============================================================================
// Printing and Visualization
// ============================================================================

void fp_fft_print_complex_array(const char* name, const Complex* data, int n, int max_print) {
    printf("%s (showing first %d of %d):\n", name, (max_print < n) ? max_print : n, n);
    for (int i = 0; i < n && i < max_print; i++) {
        printf("  [%3d] %10.6f %+10.6fi  (mag: %10.6f, phase: %7.3f°)\n",
               i, data[i].real, data[i].imag,
               complex_magnitude(data[i]),
               complex_phase(data[i]) * 180.0 / M_PI);
    }
    if (n > max_print) {
        printf("  ... (%d more entries)\n", n - max_print);
    }
}

void fp_fft_print_real_array(const char* name, const double* data, int n, int max_print) {
    printf("%s (showing first %d of %d):\n", name, (max_print < n) ? max_print : n, n);
    for (int i = 0; i < n && i < max_print; i++) {
        printf("  [%3d] %12.6f\n", i, data[i]);
    }
    if (n > max_print) {
        printf("  ... (%d more entries)\n", n - max_print);
    }
}

void fp_fft_print_spectrum_summary(const Complex* freq_data, int n, double sample_rate) {
    printf("\nSpectrum Summary:\n");
    printf("  Sample rate: %.1f Hz\n", sample_rate);
    printf("  FFT size: %d\n", n);
    printf("  Frequency resolution: %.3f Hz\n", sample_rate / n);
    printf("  Nyquist frequency: %.1f Hz\n", sample_rate / 2.0);

    // Find dominant frequencies (top 5 peaks)
    printf("\nTop 5 Frequency Components:\n");
    printf("  Freq (Hz) | Magnitude | Power (dB)\n");
    printf("  ----------|-----------|----------\n");

    int half = n / 2;  // Only positive frequencies
    for (int peak = 0; peak < 5 && peak < half; peak++) {
        int max_idx = 0;
        double max_mag = 0.0;

        // Find next peak (excluding DC and already found)
        for (int i = 1; i < half; i++) {
            double mag = complex_magnitude(freq_data[i]);
            if (mag > max_mag) {
                int already_found = 0;
                // Check if already printed (simple approach)
                if (peak == 0 || i != max_idx) {
                    max_mag = mag;
                    max_idx = i;
                }
            }
        }

        if (max_mag > 0.0) {
            double freq = max_idx * sample_rate / n;
            double power_db = 20.0 * log10(max_mag);
            printf("  %8.2f  | %9.2f | %8.2f\n", freq, max_mag, power_db);

            // Zero out this peak for next iteration
            Complex temp = freq_data[max_idx];
            ((Complex*)freq_data)[max_idx].real = 0.0;
            ((Complex*)freq_data)[max_idx].imag = 0.0;
        }
    }
}

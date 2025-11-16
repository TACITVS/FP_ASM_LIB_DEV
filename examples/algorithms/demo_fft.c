// demo_fft.c - Fast Fourier Transform Demo
// Demonstrates FFT and signal processing applications

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Forward declarations
typedef struct { double real, imag; } Complex;

void fp_fft(Complex* data, int n);
void fp_ifft(Complex* data, int n);
void fp_rfft(const double* real_data, Complex* freq_data, int n);
void fp_irfft(const Complex* freq_data, double* real_data, int n);
void fp_fft_convolve(const double* x, int n, const double* h, int m, double* y);

void fp_fft_power_spectrum(const Complex* freq_data, double* power, int n);
void fp_fft_magnitude_spectrum(const Complex* freq_data, double* magnitude, int n);
double fp_fft_parseval_check(const double* time_signal, const Complex* freq_signal, int n);
int fp_fft_is_power_of_2(int n);

void fp_generate_sine(double* signal, int n, double frequency, double amplitude, double phase, double sample_rate);
void fp_generate_cosine(double* signal, int n, double frequency, double amplitude, double phase, double sample_rate);
void fp_add_noise(double* signal, int n, double noise_level);

void fp_fft_print_complex_array(const char* name, const Complex* data, int n, int max_print);
void fp_fft_print_real_array(const char* name, const double* data, int n, int max_print);
void fp_fft_print_spectrum_summary(const Complex* freq_data, int n, double sample_rate);

// ============================================================================
// TEST 1: FFT/IFFT Round-Trip (Correctness Verification)
// ============================================================================
void test_fft_ifft_roundtrip() {
    printf("================================================================\n");
    printf("TEST 1: FFT/IFFT Round-Trip (Correctness Test)\n");
    printf("================================================================\n\n");

    int n = 64;
    double sample_rate = 1000.0;  // 1 kHz

    printf("Test: Transform signal to frequency domain and back\n");
    printf("Expected: Perfect reconstruction (error < 1e-12)\n\n");

    // Generate test signal: mix of 50 Hz and 120 Hz sine waves
    double* original = (double*)malloc(n * sizeof(double));
    fp_generate_sine(original, n, 50.0, 1.0, 0.0, sample_rate);

    double* temp = (double*)malloc(n * sizeof(double));
    fp_generate_sine(temp, n, 120.0, 0.5, M_PI/4, sample_rate);

    for (int i = 0; i < n; i++) {
        original[i] += temp[i];
    }

    printf("Original signal (first 8 samples):\n");
    fp_fft_print_real_array("", original, n, 8);

    // Convert to complex
    Complex* freq = (Complex*)malloc(n * sizeof(Complex));
    for (int i = 0; i < n; i++) {
        freq[i].real = original[i];
        freq[i].imag = 0.0;
    }

    // Forward FFT
    fp_fft(freq, n);
    printf("\nFrequency domain (first 8 bins):\n");
    fp_fft_print_complex_array("", freq, n, 8);

    // Inverse FFT
    fp_ifft(freq, n);

    // Extract real part
    double* reconstructed = (double*)malloc(n * sizeof(double));
    for (int i = 0; i < n; i++) {
        reconstructed[i] = freq[i].real;
    }

    printf("\nReconstructed signal (first 8 samples):\n");
    fp_fft_print_real_array("", reconstructed, n, 8);

    // Calculate reconstruction error
    double max_error = 0.0;
    for (int i = 0; i < n; i++) {
        double error = fabs(original[i] - reconstructed[i]);
        if (error > max_error) max_error = error;
    }

    printf("\n");
    printf("Reconstruction Quality:\n");
    printf("  Maximum error: %.2e\n", max_error);
    printf("  Status: %s\n", (max_error < 1e-10) ? "PASS (Perfect!)" : "FAIL");
    printf("\n");

    free(original);
    free(temp);
    free(freq);
    free(reconstructed);
}

// ============================================================================
// TEST 2: Spectral Analysis (Frequency Detection)
// ============================================================================
void test_spectral_analysis() {
    printf("================================================================\n");
    printf("TEST 2: Spectral Analysis (Frequency Detection)\n");
    printf("================================================================\n\n");

    int n = 512;
    double sample_rate = 1000.0;  // 1 kHz

    printf("Signal: Combination of 3 sine waves + noise\n");
    printf("  - 50 Hz (amplitude 1.0)\n");
    printf("  - 120 Hz (amplitude 0.7)\n");
    printf("  - 200 Hz (amplitude 0.3)\n");
    printf("  + Gaussian noise (level 0.1)\n\n");

    // Generate multi-frequency signal
    double* signal = (double*)calloc(n, sizeof(double));
    double* temp = (double*)malloc(n * sizeof(double));

    fp_generate_sine(temp, n, 50.0, 1.0, 0.0, sample_rate);
    for (int i = 0; i < n; i++) signal[i] += temp[i];

    fp_generate_sine(temp, n, 120.0, 0.7, 0.0, sample_rate);
    for (int i = 0; i < n; i++) signal[i] += temp[i];

    fp_generate_sine(temp, n, 200.0, 0.3, 0.0, sample_rate);
    for (int i = 0; i < n; i++) signal[i] += temp[i];

    srand(42);
    fp_add_noise(signal, n, 0.1);

    // Perform FFT
    Complex* freq = (Complex*)malloc(n * sizeof(Complex));
    for (int i = 0; i < n; i++) {
        freq[i].real = signal[i];
        freq[i].imag = 0.0;
    }
    fp_fft(freq, n);

    // Compute magnitude spectrum
    double* magnitude = (double*)malloc(n * sizeof(double));
    fp_fft_magnitude_spectrum(freq, magnitude, n);

    // Find dominant frequencies
    printf("Dominant Frequencies Detected:\n");
    printf("  Freq (Hz) | Magnitude | Expected\n");
    printf("  ----------|-----------|---------\n");

    int half = n / 2;
    for (int peak = 0; peak < 3; peak++) {
        int max_idx = 0;
        double max_mag = 0.0;

        for (int i = 1; i < half; i++) {
            if (magnitude[i] > max_mag) {
                max_mag = magnitude[i];
                max_idx = i;
            }
        }

        double freq_hz = max_idx * sample_rate / n;
        printf("  %8.1f  | %9.2f | ", freq_hz, max_mag);

        if (fabs(freq_hz - 50.0) < 2.0) printf("50 Hz ✓\n");
        else if (fabs(freq_hz - 120.0) < 2.0) printf("120 Hz ✓\n");
        else if (fabs(freq_hz - 200.0) < 2.0) printf("200 Hz ✓\n");
        else printf("???\n");

        magnitude[max_idx] = 0.0;  // Zero out for next iteration
    }

    printf("\n");
    free(signal);
    free(temp);
    free(freq);
    free(magnitude);
}

// ============================================================================
// TEST 3: Fast Convolution (O(n log n) vs O(n²))
// ============================================================================
void test_fast_convolution() {
    printf("================================================================\n");
    printf("TEST 3: Fast Convolution (FFT-based)\n");
    printf("================================================================\n\n");

    printf("Convolution: smoothing filter applied to noisy signal\n");
    printf("Direct convolution: O(n²) complexity\n");
    printf("FFT convolution: O(n log n) complexity\n\n");

    int signal_len = 256;
    int filter_len = 11;

    // Generate noisy square wave
    double* signal = (double*)malloc(signal_len * sizeof(double));
    fp_generate_sine(signal, signal_len, 10.0, 1.0, 0.0, 1000.0);
    srand(42);
    fp_add_noise(signal, signal_len, 0.3);

    // Create moving average filter (simple low-pass)
    double* filter = (double*)malloc(filter_len * sizeof(double));
    for (int i = 0; i < filter_len; i++) {
        filter[i] = 1.0 / filter_len;
    }

    // FFT-based convolution
    int output_len = signal_len + filter_len - 1;
    double* convolved = (double*)malloc(output_len * sizeof(double));

    clock_t start = clock();
    fp_fft_convolve(signal, signal_len, filter, filter_len, convolved);
    clock_t end = clock();

    double time_ms = 1000.0 * (end - start) / CLOCKS_PER_SEC;

    printf("Convolution Results:\n");
    printf("  Signal length: %d\n", signal_len);
    printf("  Filter length: %d\n", filter_len);
    printf("  Output length: %d\n", output_len);
    printf("  Time: %.3f ms\n", time_ms);

    printf("\nOriginal (noisy, first 10 samples):\n");
    fp_fft_print_real_array("", signal, signal_len, 10);

    printf("\nFiltered (smoothed, first 10 samples):\n");
    fp_fft_print_real_array("", convolved, output_len, 10);

    printf("\nObservation: FFT convolution smooths noise while preserving signal shape\n");
    printf("For large signals, FFT is MUCH faster than direct convolution!\n\n");

    free(signal);
    free(filter);
    free(convolved);
}

// ============================================================================
// TEST 4: Parseval's Theorem (Energy Conservation)
// ============================================================================
void test_parseval_theorem() {
    printf("================================================================\n");
    printf("TEST 4: Parseval's Theorem (Energy Conservation)\n");
    printf("================================================================\n\n");

    printf("Parseval's Theorem: Energy is conserved in FFT\n");
    printf("  Time domain: Σ|x[n]|²\n");
    printf("  Freq domain: (1/N)Σ|X[k]|²\n");
    printf("  These must be equal!\n\n");

    int n = 256;
    double sample_rate = 1000.0;

    // Generate test signal
    double* signal = (double*)malloc(n * sizeof(double));
    fp_generate_sine(signal, n, 50.0, 1.0, 0.0, sample_rate);

    double* temp = (double*)malloc(n * sizeof(double));
    fp_generate_sine(temp, n, 120.0, 0.5, 0.0, sample_rate);
    for (int i = 0; i < n; i++) signal[i] += temp[i];

    // Perform FFT
    Complex* freq = (Complex*)malloc(n * sizeof(Complex));
    for (int i = 0; i < n; i++) {
        freq[i].real = signal[i];
        freq[i].imag = 0.0;
    }
    fp_fft(freq, n);

    // Check Parseval's theorem
    double relative_error = fp_fft_parseval_check(signal, freq, n);

    // Calculate energies manually for display
    double time_energy = 0.0;
    double freq_energy = 0.0;

    for (int i = 0; i < n; i++) {
        time_energy += signal[i] * signal[i];
        double mag = sqrt(freq[i].real * freq[i].real + freq[i].imag * freq[i].imag);
        freq_energy += mag * mag;
    }
    freq_energy /= n;

    printf("Energy Analysis:\n");
    printf("  Time domain energy: %.6f\n", time_energy);
    printf("  Freq domain energy: %.6f\n", freq_energy);
    printf("  Relative error: %.2e\n", relative_error);
    printf("  Status: %s\n", (relative_error < 1e-10) ? "PASS (Energy conserved!)" : "FAIL");
    printf("\n");

    free(signal);
    free(temp);
    free(freq);
}

// ============================================================================
// Main
// ============================================================================
int main() {
    printf("================================================================\n");
    printf("  Fast Fourier Transform (FFT) Demo\n");
    printf("  FP-ASM Library - Signal Processing & Spectral Analysis\n");
    printf("================================================================\n\n");

    printf("FFT Applications Demonstrated:\n");
    printf("  1. FFT/IFFT Round-Trip - Correctness verification\n");
    printf("  2. Spectral Analysis - Frequency detection in mixed signals\n");
    printf("  3. Fast Convolution - O(n log n) filtering\n");
    printf("  4. Parseval's Theorem - Energy conservation\n\n");

    printf("Key Algorithm: Cooley-Tukey FFT (Radix-2)\n");
    printf("  - Divide-and-conquer approach\n");
    printf("  - Butterfly operations with twiddle factors\n");
    printf("  - Bit-reversal permutation\n");
    printf("  - Complexity: O(n log n) vs O(n²) for DFT\n\n");

    test_fft_ifft_roundtrip();
    test_spectral_analysis();
    test_fast_convolution();
    test_parseval_theorem();

    printf("================================================================\n");
    printf("  FFT Demo Complete!\n");
    printf("================================================================\n\n");

    printf("Key Takeaways:\n");
    printf("  ✓ FFT transforms time → frequency domain (and back)\n");
    printf("  ✓ Detects hidden frequencies in complex signals\n");
    printf("  ✓ Enables fast convolution (O(n log n) vs O(n²))\n");
    printf("  ✓ Conserves energy (Parseval's theorem)\n");
    printf("  ✓ Foundation of modern signal processing\n\n");

    printf("Real-World Applications:\n");
    printf("  - Audio Processing: MP3, speech recognition, noise cancellation\n");
    printf("  - Image Processing: JPEG compression, image filtering\n");
    printf("  - Telecommunications: OFDM (4G/5G), channel equalization\n");
    printf("  - Scientific Computing: PDE solvers, spectral methods\n");
    printf("  - Machine Learning: Convolutional layers, feature extraction\n\n");

    printf("Why FFT Revolutionized Computing:\n");
    printf("  - O(n log n) vs O(n²) = 1000× speedup for n=1M\n");
    printf("  - Enables real-time audio/video processing\n");
    printf("  - Powers digital communications (WiFi, cell phones)\n");
    printf("  - Voted one of top 10 algorithms of 20th century!\n\n");

    return 0;
}

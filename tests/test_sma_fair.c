#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../include/fp_core.h"

// Fair C baseline - same sliding window algorithm
void sma_c_optimized(const double* data, size_t n, size_t window, double* output) {
    if (n == 0 || window == 0 || window > n) return;

    double sum = 0.0;
    for (size_t i = 0; i < window; i++) {
        sum += data[i];
    }
    output[0] = sum / window;

    size_t out_size = n - window + 1;
    for (size_t i = 1; i < out_size; i++) {
        sum = sum - data[i - 1] + data[i + window - 1];
        output[i] = sum / window;
    }
}

int main(void) {
    size_t n = 10000;
    size_t window = 1000;
    int iterations = 1000;

    double* data = malloc(n * sizeof(double));
    double* output = malloc((n - window + 1) * sizeof(double));

    for (size_t i = 0; i < n; i++) {
        data[i] = (double)(i % 100);
    }

    // Warmup
    fp_sma_f64(data, n, window, output);
    sma_c_optimized(data, n, window, output);

    // Benchmark ASM
    clock_t start = clock();
    for (int i = 0; i < iterations; i++) {
        fp_sma_f64(data, n, window, output);
    }
    clock_t end = clock();
    double time_asm = (double)(end - start) / CLOCKS_PER_SEC;

    // Benchmark C optimized
    start = clock();
    for (int i = 0; i < iterations; i++) {
        sma_c_optimized(data, n, window, output);
    }
    end = clock();
    double time_c = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Fair Comparison (both using sliding window):\n");
    printf("==============================================\n");
    printf("Assembly: %.6f seconds\n", time_asm);
    printf("C:        %.6f seconds\n", time_c);
    printf("Speedup:  %.2fx (implementation only)\n", time_c / time_asm);

    free(data);
    free(output);
    return 0;
}
